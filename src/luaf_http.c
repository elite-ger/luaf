#include "luaf_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    
    static int winsock_initialized = 0;
    
    static void init_winsock(void) {
        if (!winsock_initialized) {
            WSADATA wsa;
            WSAStartup(MAKEWORD(2, 2), &wsa);
            winsock_initialized = 1;
        }
    }
    
    #define close_socket(s) closesocket(s)
    typedef HANDLE thread_t;
    typedef CRITICAL_SECTION mutex_t;
    
    static void mutex_init(mutex_t* m) { InitializeCriticalSection(m); }
    static void mutex_lock(mutex_t* m) { EnterCriticalSection(m); }
    static void mutex_unlock(mutex_t* m) { LeaveCriticalSection(m); }
    static void mutex_destroy(mutex_t* m) { DeleteCriticalSection(m); }
    static thread_t thread_create(LPTHREAD_START_ROUTINE func, void* arg) {
        return CreateThread(NULL, 0, func, arg, 0, NULL);
    }
    static void thread_close(thread_t t) { CloseHandle(t); }
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <pthread.h>
    
    #define close_socket(s) close(s)
    typedef int SOCKET;
    #define INVALID_SOCKET (-1)
    typedef pthread_t thread_t;
    typedef pthread_mutex_t mutex_t;
    
    static void mutex_init(mutex_t* m) { pthread_mutex_init(m, NULL); }
    static void mutex_lock(mutex_t* m) { pthread_mutex_lock(m); }
    static void mutex_unlock(mutex_t* m) { pthread_mutex_unlock(m); }
    static void mutex_destroy(mutex_t* m) { pthread_mutex_destroy(m); }
    static thread_t thread_create(void*(*func)(void*), void* arg) {
        pthread_t t;
        pthread_create(&t, NULL, func, arg);
        return t;
    }
    static void thread_close(thread_t t) { pthread_detach(t); }
#endif


typedef struct AsyncRequest {
    char* url;
    char* method;
    char* body;
    char* response;
    int callback_ref;
    int done;
    int error;
    thread_t thread;
    struct AsyncRequest* next;
} AsyncRequest;

static AsyncRequest* pending_requests = NULL;
static mutex_t requests_mutex;

static int parse_url(const char* url, char* host, size_t host_size,
                     char* path, size_t path_size, int* port) {
    const char* p = url;
    
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
        *port = 80;
    } else if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        *port = 443;
        return -1;
    } else {
        *port = 80;
    }
    
    const char* host_start = p;
    while (*p && *p != '/' && *p != ':' && *p != '?') p++;
    size_t host_len = p - host_start;
    if (host_len >= host_size) host_len = host_size - 1;
    memcpy(host, host_start, host_len);
    host[host_len] = '\0';
    
    if (*p == ':') {
        p++;
        *port = atoi(p);
        while (*p && *p != '/' && *p != '?') p++;
    }
    
    if (*p == '\0' || *p == '?') {
        strcpy(path, "/");
    } else {
        const char* path_start = p;
        while (*p && *p != '?') p++;
        size_t path_len = p - path_start;
        if (path_len >= path_size) path_len = path_size - 1;
        memcpy(path, path_start, path_len);
        path[path_len] = '\0';
    }
    
    return 0;
}

static char* http_request(const char* host, int port, const char* path,
                          const char* method, const char* body) {
#ifdef _WIN32
    init_winsock();
#endif
    
    struct addrinfo hints, *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    if (getaddrinfo(host, port_str, &hints, &result) != 0) {
        return NULL;
    }
    
    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(result);
        return NULL;
    }
    
    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) != 0) {
        close_socket(sock);
        freeaddrinfo(result);
        return NULL;
    }
    
    freeaddrinfo(result);
    
    char request[8192];
    int req_len;
    
    if (body) {
        req_len = snprintf(request, sizeof(request),
            "%s %s HTTP/1.0\r\n"
            "Host: %s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            method, path, host, strlen(body), body);
    } else {
        req_len = snprintf(request, sizeof(request),
            "%s %s HTTP/1.0\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n"
            "\r\n",
            method, path, host);
    }
    
    if (send(sock, request, req_len, 0) < 0) {
        close_socket(sock);
        return NULL;
    }
    
    char* response = NULL;
    size_t response_size = 0;
    size_t response_capacity = 4096;
    
    response = (char*)malloc(response_capacity);
    if (!response) {
        close_socket(sock);
        return NULL;
    }
    
    char buf[1024];
    int received;
    while ((received = recv(sock, buf, sizeof(buf), 0)) > 0) {
        if (response_size + received >= response_capacity) {
            response_capacity *= 2;
            char* new_response = (char*)realloc(response, response_capacity);
            if (!new_response) {
                free(response);
                close_socket(sock);
                return NULL;
            }
            response = new_response;
        }
        memcpy(response + response_size, buf, received);
        response_size += received;
    }
    
    response[response_size] = '\0';
    close_socket(sock);
    
    char* body_start = strstr(response, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        size_t body_len = strlen(body_start);
        char* final_body = (char*)malloc(body_len + 1);
        if (final_body) {
            memcpy(final_body, body_start, body_len + 1);
            free(response);
            return final_body;
        }
    }
    
    return response;
}

#ifdef _WIN32
static DWORD WINAPI async_thread(LPVOID arg)
#else
static void* async_thread(void* arg)
#endif
{
    AsyncRequest* req = (AsyncRequest*)arg;
    
    char host[256];
    char path[1024];
    int port;
    
    if (parse_url(req->url, host, sizeof(host), path, sizeof(path), &port) == 0) {
        req->response = http_request(host, port, path, req->method, req->body);
        req->error = (req->response == NULL);
    } else {
        req->error = 1;
        req->response = strdup("HTTPS not supported");
    }
    
    req->done = 1;
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static int l_http_get_async(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    
    AsyncRequest* req = (AsyncRequest*)malloc(sizeof(AsyncRequest));
    memset(req, 0, sizeof(AsyncRequest));
    req->url = strdup(url);
    req->method = "GET";
    req->body = NULL;
    req->done = 0;
    req->error = 0;
    
    lua_pushvalue(L, 2);
    req->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    mutex_lock(&requests_mutex);
    req->next = pending_requests;
    pending_requests = req;
    mutex_unlock(&requests_mutex);
    
    req->thread = thread_create(async_thread, req);
    
    return 0;
}

void luaf_http_poll(lua_State* mainL) {
    mutex_lock(&requests_mutex);
    
    AsyncRequest** prev = &pending_requests;
    AsyncRequest* req = pending_requests;
    
    while (req) {
        if (req->done) {
            *prev = req->next;
            
            lua_rawgeti(mainL, LUA_REGISTRYINDEX, req->callback_ref);
            
            if (lua_isfunction(mainL, -1)) {
                if (req->error) {
                    lua_pushnil(mainL);
                    lua_pushstring(mainL, req->response ? req->response : "Unknown error");
                    lua_pcall(mainL, 2, 0, 0);
                } else {
                    lua_pushstring(mainL, req->response ? req->response : "");
                    lua_pcall(mainL, 1, 0, 0);
                }
            } else {
                lua_pop(mainL, 1);
            }
            
            luaL_unref(mainL, LUA_REGISTRYINDEX, req->callback_ref);
            
            free(req->url);
            free(req->response);
            thread_close(req->thread);
            
            AsyncRequest* to_free = req;
            req = req->next;
            free(to_free);
        } else {
            prev = &req->next;
            req = req->next;
        }
    }
    
    mutex_unlock(&requests_mutex);
}

int luaf_http_has_pending(void) {
    int has = 0;
    mutex_lock(&requests_mutex);
    has = (pending_requests != NULL);
    mutex_unlock(&requests_mutex);
    return has;
}

static int l_http_get(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    
    char host[256];
    char path[1024];
    int port;
    
    if (parse_url(url, host, sizeof(host), path, sizeof(path), &port) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, "HTTPS not supported");
        return 2;
    }
    
    char* response = http_request(host, port, path, "GET", NULL);
    
    if (response) {
        lua_pushstring(L, response);
        free(response);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "HTTP request failed");
        return 2;
    }
}

static int l_http_post(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    const char* body = luaL_checkstring(L, 2);
    
    char host[256];
    char path[1024];
    int port;
    
    if (parse_url(url, host, sizeof(host), path, sizeof(path), &port) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, "HTTPS not supported");
        return 2;
    }
    
    char* response = http_request(host, port, path, "POST", body);
    
    if (response) {
        lua_pushstring(L, response);
        free(response);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "HTTP request failed");
        return 2;
    }
}

static const luaL_Reg http_lib[] = {
    {"get",       l_http_get},
    {"post",      l_http_post},
    {"get_async", l_http_get_async},
    {NULL, NULL}
};

void luaf_open_http(lua_State* L) {
    static int initialized = 0;
    if (!initialized) {
        mutex_init(&requests_mutex);
        initialized = 1;
    }
    
    luaL_newlib(L, http_lib);
    lua_setglobal(L, "http");
}