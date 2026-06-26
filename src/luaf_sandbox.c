#include "luaf.h"
#include <string.h>

static void instruction_hook(lua_State* L, lua_Debug* ar) {
    (void)ar;
    lua_getfield(L, LUA_REGISTRYINDEX, "__luaf_state");
    luaf_State* ls = (luaf_State*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    
    if (!ls) return;
    ls->instruction_count++;
    if (ls->max_instructions > 0 && ls->instruction_count > ls->max_instructions) {
        ls->status = LUAF_ERR_INSTR;
        ls->error_msg = "instruction limit exceeded";
        luaL_error(L, "Luaf: instruction limit exceeded");
    }
}

void luaf_install_instruction_limit(luaf_State* ls) {
    lua_pushlightuserdata(ls->L, ls);
    lua_setfield(ls->L, LUA_REGISTRYINDEX, "__luaf_state");
    lua_sethook(ls->L, instruction_hook, LUA_MASKCOUNT, 1);
}

void luaf_block_dangerous(luaf_State* ls) {
    lua_State* L = ls->L;
    
    static const char* blocked[] = {
        "dofile", "load", "collectgarbage",
        "rawequal", "rawlen",
        NULL
    };
    
    for (int i = 0; blocked[i] != NULL; i++) {
        lua_pushnil(L);
        lua_setglobal(L, blocked[i]);
    }
    
    lua_getglobal(L, "os");
    if (lua_istable(L, -1)) {
        lua_pushnil(L); lua_setfield(L, -2, "execute");
        lua_pushnil(L); lua_setfield(L, -2, "remove");
        lua_pushnil(L); lua_setfield(L, -2, "rename");
        lua_pushnil(L); lua_setfield(L, -2, "tmpname");
        lua_pushnil(L); lua_setfield(L, -2, "setlocale");
    }
    lua_pop(L, 1);
}

static int luaf_print(lua_State* L) {
    int n = lua_gettop(L);
    lua_getglobal(L, "tostring");
    for (int i = 1; i <= n; i++) {
        lua_pushvalue(L, -1);
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        const char* s = lua_tostring(L, -1);
        if (s == NULL) return luaL_error(L, "tostring returned nil");
        if (i > 1) fputs("\t", stdout);
        fputs(s, stdout);
        lua_pop(L, 1);
    }
    fputs("\n", stdout);
    fflush(stdout);
    return 0;
}

void luaf_setup_sandbox(luaf_State* ls) {
    luaf_block_dangerous(ls);
    luaf_install_instruction_limit(ls);
    ls->status = LUAF_OK;
}