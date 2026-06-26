#include "luaf_runtime.h"
#include "luaf_http.h"
#include "luaf_vector.h"
#include "luaf_preprocess.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int luaf_wait(lua_State *L);
static int luaf_spawn(lua_State *L);
static int luaf_delay(lua_State *L);

static int l_solid_register(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    (void)name;
    lua_pushvalue(L, 2);
    return 1;
}

static int l_solid_sync(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    (void)name;
    return 0;
}

static int l_check_int(lua_State *L)
{
    if (!lua_isinteger(L, 1))
    {
        luaL_error(L, "type check failed: expected integer");
    }
    lua_pushvalue(L, 1);
    return 1;
}

static int l_check_number(lua_State *L)
{
    if (!lua_isnumber(L, 1))
    {
        luaL_error(L, "type check failed: expected number");
    }
    lua_pushvalue(L, 1);
    return 1;
}

static int luaf_wait(lua_State *L)
{
    double seconds = luaL_optnumber(L, 1, 0.03);

    lua_getfield(L, LUA_REGISTRYINDEX, "__luaf_runtime");
    LuafRuntime *rt = (LuafRuntime *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (rt)
    {
        LuafTask *task = (LuafTask *)malloc(sizeof(LuafTask));
        task->coroutine = L;
        task->wake_time = rt->global_time + seconds;

        if (!rt->tasks || rt->tasks->wake_time > task->wake_time)
        {
            task->next = rt->tasks;
            rt->tasks = task;
        }
        else
        {
            LuafTask *curr = rt->tasks;
            while (curr->next && curr->next->wake_time <= task->wake_time)
            {
                curr = curr->next;
            }
            task->next = curr->next;
            curr->next = task;
        }
    }

    return lua_yield(L, 0);
}

static int luaf_spawn(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);

    lua_getfield(L, LUA_REGISTRYINDEX, "__luaf_runtime");
    LuafRuntime *rt = (LuafRuntime *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!rt)
    {
        lua_pushstring(L, "spawn: runtime not found");
        lua_error(L);
        return 0;
    }

    lua_State *co = lua_newthread(rt->mainL);

    lua_pushvalue(L, 1);
    lua_xmove(L, co, 1);

    lua_pushcfunction(co, luaf_wait);
    lua_setglobal(co, "wait");
    lua_pushcfunction(co, luaf_spawn);
    lua_setglobal(co, "spawn");
    lua_pushcfunction(co, luaf_delay);
    lua_setglobal(co, "delay");
    luaL_requiref(co, "_G", luaopen_base, 1);
    lua_pop(co, 1);
    luaf_open_http(co);
    luaf_open_vector(co);

    lua_pushcfunction(co, l_solid_register);
    lua_setglobal(co, "__solid_register");
    lua_pushcfunction(co, l_solid_sync);
    lua_setglobal(co, "__solid_sync");
    lua_pushcfunction(co, l_check_int);
    lua_setglobal(co, "__luaf_check_int");
    lua_pushcfunction(co, l_check_number);
    lua_setglobal(co, "__luaf_check_number");

    lua_pushlightuserdata(co, rt);
    lua_setfield(co, LUA_REGISTRYINDEX, "__luaf_runtime");

    LuafTask *task = (LuafTask *)malloc(sizeof(LuafTask));
    task->coroutine = co;
    task->wake_time = rt->global_time;

    if (!rt->tasks || rt->tasks->wake_time > task->wake_time)
    {
        task->next = rt->tasks;
        rt->tasks = task;
    }
    else
    {
        LuafTask *curr = rt->tasks;
        while (curr->next && curr->next->wake_time <= task->wake_time)
        {
            curr = curr->next;
        }
        task->next = curr->next;
        curr->next = task;
    }

    return 0;
}

static int luaf_delay(lua_State *L)
{
    double seconds = luaL_checknumber(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_getfield(L, LUA_REGISTRYINDEX, "__luaf_runtime");
    LuafRuntime *rt = (LuafRuntime *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!rt)
    {
        lua_pushstring(L, "delay: runtime not found");
        lua_error(L);
        return 0;
    }

    lua_State *co = lua_newthread(rt->mainL);

    lua_pushcfunction(co, luaf_wait);
    lua_setglobal(co, "wait");
    lua_pushcfunction(co, luaf_spawn);
    lua_setglobal(co, "spawn");
    lua_pushcfunction(co, luaf_delay);
    lua_setglobal(co, "delay");
    luaL_requiref(co, "_G", luaopen_base, 1);
    lua_pop(co, 1);
    luaf_open_http(co);
    luaf_open_vector(co);

    lua_pushcfunction(co, l_solid_register);
    lua_setglobal(co, "__solid_register");
    lua_pushcfunction(co, l_solid_sync);
    lua_setglobal(co, "__solid_sync");
    lua_pushcfunction(co, l_check_int);
    lua_setglobal(co, "__luaf_check_int");
    lua_pushcfunction(co, l_check_number);
    lua_setglobal(co, "__luaf_check_number");

    lua_pushlightuserdata(co, rt);
    lua_setfield(co, LUA_REGISTRYINDEX, "__luaf_runtime");

    luaL_loadstring(co,
                    "local t, f = ...\n"
                    "wait(t)\n"
                    "f()\n");

    lua_pushnumber(co, seconds);
    lua_pushvalue(L, 2);
    lua_xmove(L, co, 1);

    LuafTask *task = (LuafTask *)malloc(sizeof(LuafTask));
    task->coroutine = co;
    task->wake_time = rt->global_time;

    if (!rt->tasks || rt->tasks->wake_time > task->wake_time)
    {
        task->next = rt->tasks;
        rt->tasks = task;
    }
    else
    {
        LuafTask *curr = rt->tasks;
        while (curr->next && curr->next->wake_time <= task->wake_time)
        {
            curr = curr->next;
        }
        task->next = curr->next;
        curr->next = task;
    }

    return 0;
}

LuafRuntime *luaf_runtime_new(void)
{
    LuafRuntime *rt = (LuafRuntime *)malloc(sizeof(LuafRuntime));
    if (!rt)
        return NULL;

    memset(rt, 0, sizeof(LuafRuntime));

    luaf_State *ls = luaf_newstate(LUAF_PERM_SCRIPTING);
    if (!ls)
    {
        free(rt);
        return NULL;
    }
    luaf_setup_sandbox(ls);

    rt->mainL = ls->L;
    rt->tasks = NULL;
    rt->global_time = 0.0;
    rt->running = 1;

    lua_pushlightuserdata(rt->mainL, rt);
    lua_setfield(rt->mainL, LUA_REGISTRYINDEX, "__luaf_runtime");

    return rt;
}

void luaf_runtime_free(LuafRuntime *rt)
{
    if (!rt)
        return;

    LuafTask *task = rt->tasks;
    while (task)
    {
        LuafTask *next = task->next;
        free(task);
        task = next;
    }

    if (rt->mainL)
    {
        lua_close(rt->mainL);
    }

    free(rt);
}

static void runtime_add_function(LuafRuntime *rt, lua_State *co)
{
    lua_pushcfunction(co, luaf_wait);
    lua_setglobal(co, "wait");
    lua_pushcfunction(co, luaf_spawn);
    lua_setglobal(co, "spawn");
    lua_pushcfunction(co, luaf_delay);
    lua_setglobal(co, "delay");
    luaL_requiref(co, "_G", luaopen_base, 1);
    lua_pop(co, 1);
    luaL_dostring(co, "package.path = package.path .. ';modules/?.lua'");
    luaf_open_http(co);
    luaf_open_vector(co);

    luaL_dostring(co,
        "function solid(name, value)\n"
        "    local data = { _value = value, _name = name }\n"
        "    local mt = {\n"
        "        __index = function(t, k)\n"
        "            if k == 'value' then return rawget(t, '_value') end\n"
        "            return rawget(t, k)\n"
        "        end,\n"
        "        __newindex = function(t, k, v)\n"
        "            if k == 'value' then\n"
        "                rawset(t, '_value', v)\n"
        "                local Net = require('net')\n"
        "                Net:Broadcast('__solid_' .. rawget(t, '_name'), v)\n"
        "            else\n"
        "                rawset(t, k, v)\n"
        "            end\n"
        "        end,\n"
        "        __tostring = function(t) return tostring(rawget(t, '_value')) end,\n"
        "        __add = function(a, b) return rawget(a, '_value') + (type(b) == 'table' and rawget(b, '_value') or b) end,\n"
        "        __sub = function(a, b) return rawget(a, '_value') - (type(b) == 'table' and rawget(b, '_value') or b) end,\n"
        "        __mul = function(a, b) return rawget(a, '_value') * (type(b) == 'table' and rawget(b, '_value') or b) end,\n"
        "        __div = function(a, b) return rawget(a, '_value') / (type(b) == 'table' and rawget(b, '_value') or b) end,\n"
        "        __unm = function(a) return -rawget(a, '_value') end,\n"
        "        __eq = function(a, b) return rawget(a, '_value') == (type(b) == 'table' and rawget(b, '_value') or b) end,\n"
        "        __lt = function(a, b) return rawget(a, '_value') < (type(b) == 'table' and rawget(b, '_value') or b) end,\n"
        "        __le = function(a, b) return rawget(a, '_value') <= (type(b) == 'table' and rawget(b, '_value') or b) end,\n"
        "    }\n"
        "    local var = setmetatable(data, mt)\n"
        "    _G[name] = var\n"
        "    return var\n"
        "end\n"
    );

    lua_pushcfunction(co, l_solid_register);
    lua_setglobal(co, "__solid_register");
    lua_pushcfunction(co, l_solid_sync);
    lua_setglobal(co, "__solid_sync");
    lua_pushcfunction(co, l_check_int);
    lua_setglobal(co, "__luaf_check_int");
    lua_pushcfunction(co, l_check_number);
    lua_setglobal(co, "__luaf_check_number");

    lua_pushlightuserdata(co, rt);
    lua_setfield(co, LUA_REGISTRYINDEX, "__luaf_runtime");

    LuafTask *task = (LuafTask *)malloc(sizeof(LuafTask));
    task->coroutine = co;
    task->wake_time = rt->global_time;

    if (!rt->tasks || rt->tasks->wake_time > task->wake_time)
    {
        task->next = rt->tasks;
        rt->tasks = task;
    }
    else
    {
        LuafTask *curr = rt->tasks;
        while (curr->next && curr->next->wake_time <= task->wake_time)
        {
            curr = curr->next;
        }
        task->next = curr->next;
        curr->next = task;
    }
}

int luaf_runtime_add_file(LuafRuntime* rt, const char* filename) {
    if (!rt || !rt->mainL) return -1;
    
    lua_State* co = lua_newthread(rt->mainL);
    
    if (luaL_loadfile(co, filename) != LUA_OK) {
        fprintf(stderr, "Luaf load error: %s\n", lua_tostring(co, -1));
        lua_pop(co, 1);
        return -1;
    }
    
    runtime_add_function(rt, co);
    return 0;
}

int luaf_runtime_add_string(LuafRuntime *rt, const char *code)
{
    if (!rt || !rt->mainL)
        return -1;

    lua_State *co = lua_newthread(rt->mainL);

    if (luaL_loadstring(co, code) != LUA_OK)
    {
        fprintf(stderr, "Luaf load error: %s\n", lua_tostring(co, -1));
        lua_pop(co, 1);
        return -1;
    }

    runtime_add_function(rt, co);
    return 0;
}

void luaf_runtime_tick(LuafRuntime *rt, double dt)
{
    rt->global_time += dt;

    luaf_http_poll(rt->mainL);

    while (rt->tasks && rt->tasks->wake_time <= rt->global_time)
    {
        LuafTask *task = rt->tasks;
        rt->tasks = task->next;

        int status = lua_status(task->coroutine);
        int nresults;
        int ret;

        if (status == LUA_OK)
        {
            int nargs = lua_gettop(task->coroutine) - 1;
            if (nargs < 0)
                nargs = 0;
            ret = lua_resume(task->coroutine, rt->mainL, nargs, &nresults);
        }
        else if (status == LUA_YIELD)
        {
            ret = lua_resume(task->coroutine, rt->mainL, 0, &nresults);
        }
        else
        {
            free(task);
            continue;
        }

        if (ret == LUA_YIELD)
        {
            free(task);
        }
        else if (ret != LUA_OK)
        {
            fprintf(stderr, "Runtime error: %s\n",
                    lua_tostring(task->coroutine, -1));
            lua_pop(task->coroutine, 1);
            free(task);
        }
        else
        {
            free(task);
        }
    }
}

int luaf_runtime_has_tasks(LuafRuntime *rt)
{
    return (rt && rt->tasks != NULL) || luaf_http_has_pending();
}