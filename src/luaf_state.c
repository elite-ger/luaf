#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "luaf.h"

typedef struct LuafAllocState {
    size_t allocated;
    size_t limit;
} LuafAllocState;

static void* luaf_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    LuafAllocState* state = (LuafAllocState*)ud;
    
    if (nsize == 0) {
        if (ptr != NULL) {
            state->allocated -= osize;
            free(ptr);
        }
        return NULL;
    }
    
    if (ptr == NULL) {
        if (state->limit > 0 && state->allocated + nsize > state->limit) {
            return NULL;
        }
        void* newptr = malloc(nsize);
        if (newptr) state->allocated += nsize;
        return newptr;
    }
    
    if (state->limit > 0 && (state->allocated - osize + nsize) > state->limit) {
        return NULL;
    }
    
    void* newptr = realloc(ptr, nsize);
    if (newptr) state->allocated = state->allocated - osize + nsize;
    return newptr;
}

size_t luaf_get_allocated(luaf_State* ls) {
    LuafAllocState* state = (LuafAllocState*)ls->alloc_ud;
    return state ? state->allocated : 0;
}

static void luaf_open_base_libs(lua_State* L) {
    luaL_requiref(L, "_G", luaopen_base, 1);
    lua_pop(L, 1);
    
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(L, 1);
    
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(L, 1);
    
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(L, 1);
    
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1);
    lua_pop(L, 1);
    
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
    lua_pop(L, 1);
    
    luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1);
    lua_pop(L, 1);
    
    lua_newtable(L);
    lua_newtable(L);
    lua_setfield(L, -2, "loaded");
    lua_newtable(L);
    lua_setfield(L, -2, "preload");
    lua_setglobal(L, "package");
    
    luaL_dostring(L,
        "function require(name)\n"
        "    if package.loaded[name] then return package.loaded[name] end\n"
        "    local loader = package.preload[name]\n"
        "    if loader then\n"
        "        local result = loader()\n"
        "        package.loaded[name] = result\n"
        "        return result\n"
        "    end\n"
        "    -- поиск в modules/\n"
        "    local f, err = loadfile('modules/' .. name .. '.lua')\n"
        "    if f then\n"
        "        local result = f()\n"
        "        package.loaded[name] = result\n"
        "        return result\n"
        "    end\n"
        "    error('module ' .. name .. ' not found')\n"
        "end\n"
    );
}

luaf_State* luaf_newstate(int permissions) {
    luaf_State* ls = (luaf_State*)malloc(sizeof(luaf_State));
    if (!ls) return NULL;
    
    memset(ls, 0, sizeof(luaf_State));
    ls->permissions = permissions;
    ls->max_instructions = 1000000;
    ls->max_memory = 16 * 1024 * 1024;
    ls->status = LUAF_OK;
    
    LuafAllocState* alloc_state = (LuafAllocState*)malloc(sizeof(LuafAllocState));
    if (!alloc_state) {
        free(ls);
        return NULL;
    }
    alloc_state->allocated = 0;
    alloc_state->limit = ls->max_memory;
    ls->alloc_ud = alloc_state;
    
    unsigned seed = (unsigned)time(NULL) ^ (unsigned)(uintptr_t)ls;
    ls->L = lua_newstate(luaf_alloc, alloc_state, seed);
    
    if (!ls->L) {
        free(alloc_state);
        free(ls);
        return NULL;
    }
    
    luaf_open_base_libs(ls->L);
    
    return ls;
}

void luaf_close(luaf_State* ls) {
    if (!ls) return;
    if (ls->L) lua_close(ls->L);
    if (ls->alloc_ud) free(ls->alloc_ud);
    free(ls);
}

void luaf_set_max_instructions(luaf_State* ls, int max) {
    ls->max_instructions = max;
}

void luaf_set_max_memory(luaf_State* ls, size_t max) {
    ls->max_memory = max;
    if (ls->alloc_ud) {
        LuafAllocState* alloc_state = (LuafAllocState*)ls->alloc_ud;
        alloc_state->limit = max;
    }
}

const char* luaf_get_error(luaf_State* ls) {
    return ls->error_msg;
}