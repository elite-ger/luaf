#include "luaf_preprocess.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static const char* skip_spaces(const char* p) {
    while (*p && isspace(*p)) p++;
    return p;
}

static int starts_with(const char* str, const char* prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static const char* skip_ident(const char* p) {
    while (*p && (isalnum(*p) || *p == '_' || *p == '.')) p++;
    return p;
}

static const char* find_block_end(const char* p) {
    int depth = 0;
    while (*p) {
        if (*p == '(' || *p == '{' || (*p == 'd' && starts_with(p, "do"))) depth++;
        else if (*p == ')' || *p == '}' || (*p == 'e' && starts_with(p, "end"))) {
            if (depth == 0) return p;
            depth--;
        }
        p++;
    }
    return p;
}

static char* copy_string(const char* start, const char* end) {
    size_t len = end - start;
    char* result = (char*)malloc(len + 1);
    if (result) {
        memcpy(result, start, len);
        result[len] = '\0';
    }
    return result;
}

char* luaf_preprocess(const char* source) {
    size_t src_len = strlen(source);
    size_t buf_size = src_len * 2 + 1024;
    char* output = (char*)malloc(buf_size);
    if (!output) return NULL;
    
    const char* src = source;
    char* dst = output;
    size_t remaining = buf_size - 1;
    
    while (*src) {
        if (starts_with(src, "solid ") && (src == source || *(src - 1) == '\n' || isspace(*(src - 1)))) {
            src = skip_spaces(src + 5);
            
            const char* name_start = src;
            const char* name_end = skip_ident(src);
            char* name = copy_string(name_start, name_end);
            src = skip_spaces(name_end);
            
            if (*src == '=') {
                src = skip_spaces(src + 1);
                const char* val_start = src;
                while (*src && *src != '\n' && *src != ',' && *src != ';') src++;
                char* value = copy_string(val_start, src);
                
                int written = snprintf(dst, remaining,
                    "%s = __solid_register(\"%s\", %s)\n",
                    name, name, value);
                dst += written;
                remaining -= written;
                
                free(name);
                free(value);
            } else {
                int written = snprintf(dst, remaining,
                    "%s = __solid_register(\"%s\", nil)\n",
                    name, name);
                dst += written;
                remaining -= written;
                free(name);
            }
            continue;
        }
        
        if (starts_with(src, "on ")) {
            src = skip_spaces(src + 2);
            
            const char* event_start = src;
            const char* event_end = skip_ident(src);
            char* event = copy_string(event_start, event_end);
            src = skip_spaces(event_end);
            
            if (starts_with(src, "do")) {
                src = skip_spaces(src + 2);
                const char* body_start = src;
                const char* body_end = find_block_end(src);
                char* body = copy_string(body_start, body_end);
                src = body_end;
                
                if (starts_with(src, "end")) {
                    src += 3;
                }
                
                int written = snprintf(dst, remaining,
                    "%s:Connect(function()\n%s\nend)\n",
                    event, body);
                dst += written;
                remaining -= written;
                
                free(event);
                free(body);
            } else {
                int written = snprintf(dst, remaining, "on %s", event);
                dst += written;
                remaining -= written;
                free(event);
            }
            continue;
        }

        if (starts_with(src, "integer ") && (src == source || *(src - 1) == '\n' || isspace(*(src - 1)))) {
            src = skip_spaces(src + 7);
            
            const char* name_start = src;
            const char* name_end = skip_ident(src);
            char* name = copy_string(name_start, name_end);
            src = skip_spaces(name_end);
            
            if (*src == '=') {
                src = skip_spaces(src + 1);
                const char* val_start = src;
                while (*src && *src != '\n' && *src != ',' && *src != ';') src++;
                char* value = copy_string(val_start, src);
                
                int written = snprintf(dst, remaining,
                    "%s = __luaf_check_int(%s)\n",
                    name, value);
                dst += written;
                remaining -= written;
                
                free(name);
                free(value);
            }
            continue;
        }
        
        if (starts_with(src, "number ") && (src == source || *(src - 1) == '\n' || isspace(*(src - 1)))) {
            src = skip_spaces(src + 6);
            
            const char* name_start = src;
            const char* name_end = skip_ident(src);
            char* name = copy_string(name_start, name_end);
            src = skip_spaces(name_end);
            
            if (*src == '=') {
                src = skip_spaces(src + 1);
                const char* val_start = src;
                while (*src && *src != '\n' && *src != ',' && *src != ';') src++;
                char* value = copy_string(val_start, src);
                
                int written = snprintf(dst, remaining,
                    "%s = __luaf_check_number(%s)\n",
                    name, value);
                dst += written;
                remaining -= written;
                
                free(name);
                free(value);
            }
            continue;
        }
        
        *dst++ = *src++;
        remaining--;
    }
    
    *dst = '\0';
    return output;
}