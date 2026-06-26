#ifndef LUAF_HTTP_H
#define LUAF_HTTP_H

#include "luaf.h"

void luaf_open_http(lua_State* L);

void luaf_http_poll(lua_State* mainL);

int luaf_http_has_pending(void);

#endif