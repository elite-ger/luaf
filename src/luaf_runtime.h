#ifndef LUAF_RUNTIME_H
#define LUAF_RUNTIME_H

#include "luaf.h"

typedef struct LuafTask {
    lua_State* coroutine;
    double wake_time;
    struct LuafTask* next;
} LuafTask;

typedef struct LuafRuntime {
    lua_State* mainL;
    LuafTask* tasks;
    double global_time;
    int running;
} LuafRuntime;

LuafRuntime* luaf_runtime_new(void);

void luaf_runtime_free(LuafRuntime* rt);

int luaf_runtime_add_file(LuafRuntime* rt, const char* filename);

int luaf_runtime_add_string(LuafRuntime* rt, const char* code);

void luaf_runtime_tick(LuafRuntime* rt, double dt);

int luaf_runtime_has_tasks(LuafRuntime* rt);

#endif