#include "luaf.h"
#include "luaf_preprocess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int luaf_dofile(luaf_State* ls, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        ls->status = LUAF_ERR_RUNTIME;
        ls->error_msg = "cannot open file";
        return LUAF_ERR_RUNTIME;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* source = (char*)malloc(size + 1);
    if (!source) {
        fclose(f);
        ls->status = LUAF_ERR_MEMORY;
        ls->error_msg = "out of memory";
        return LUAF_ERR_MEMORY;
    }
    
    size_t read_size = fread(source, 1, size, f);
    source[read_size] = '\0';
    fclose(f);
    
    char* processed = NULL;
    const char* ext = strrchr(filename, '.');
    if (ext && strcmp(ext, ".luaf") == 0) {
        processed = luaf_preprocess(source);
        free(source);
        source = processed;
    }
    
    int load_result = luaL_loadstring(ls->L, source);
    free(source);
    
    if (load_result != LUA_OK) {
        ls->status = LUAF_ERR_RUNTIME;
        ls->error_msg = lua_tostring(ls->L, -1);
        lua_pop(ls->L, 1);
        return LUAF_ERR_RUNTIME;
    }
    
    int call_result = lua_pcall(ls->L, 0, LUA_MULTRET, 0);
    if (call_result != LUA_OK) {
        ls->status = LUAF_ERR_RUNTIME;
        ls->error_msg = lua_tostring(ls->L, -1);
        lua_pop(ls->L, 1);
        return LUAF_ERR_RUNTIME;
    }
    
    return LUAF_OK;
}

int luaf_dostring(luaf_State* ls, const char* code) {
    ls->instruction_count = 0;
    ls->status = LUAF_OK;
    ls->error_msg = NULL;
    
    lua_State* L = ls->L;
    
    int load_result = luaL_loadstring(L, code);
    if (load_result != LUA_OK) {
        ls->status = LUAF_ERR_RUNTIME;
        ls->error_msg = lua_tostring(L, -1);
        lua_pop(L, 1);
        return ls->status;
    }
    
    int call_result = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (call_result != LUA_OK) {
        ls->status = LUAF_ERR_RUNTIME;
        ls->error_msg = lua_tostring(L, -1);
        lua_pop(L, 1);
        return ls->status;
    }
    
    return LUAF_OK;
}