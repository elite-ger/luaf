#ifndef LUAF_H
#define LUAF_H

#include "../lib/lua-5.5.0/src/lua.h"
#include "../lib/lua-5.5.0/src/lualib.h"
#include "../lib/lua-5.5.0/src/lauxlib.h"

#define LUAF_OK            0
#define LUAF_ERR_MEMORY    1
#define LUAF_ERR_INSTR     2
#define LUAF_ERR_SANDBOX   3
#define LUAF_ERR_RUNTIME   4

#define LUAF_PERM_NONE         0x00
#define LUAF_PERM_READ_WORLD   0x01
#define LUAF_PERM_CREATE_PART  0x02
#define LUAF_PERM_SCRIPTING    0x04
#define LUAF_PERM_PLUGIN       0x08
#define LUAF_PERM_HOST         0xFF

typedef struct luaf_State {
    lua_State* L;
    void*      alloc_ud;
    int        permissions;
    int        max_instructions;
    size_t     max_memory;
    int        instruction_count;
    int        status;
    const char* error_msg;
} luaf_State;

luaf_State* luaf_newstate(int permissions);
void        luaf_close(luaf_State* ls);
int         luaf_dofile(luaf_State* ls, const char* filename);
int         luaf_dostring(luaf_State* ls, const char* code);
void        luaf_set_max_instructions(luaf_State* ls, int max);
void        luaf_set_max_memory(luaf_State* ls, size_t max);
const char* luaf_get_error(luaf_State* ls);
size_t      luaf_get_allocated(luaf_State* ls);

void luaf_setup_sandbox(luaf_State* ls);
void luaf_block_dangerous(luaf_State* ls);

void luaf_install_instruction_limit(luaf_State* ls);

#endif