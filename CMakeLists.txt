cmake_minimum_required(VERSION 3.16)
project(Luaf VERSION 0.1.0 LANGUAGES C)

set(LUA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/lua-5.5.0/src)

file(GLOB LUA_SOURCES
    ${LUA_DIR}/lapi.c
    ${LUA_DIR}/lcode.c
    ${LUA_DIR}/lctype.c
    ${LUA_DIR}/ldebug.c
    ${LUA_DIR}/ldo.c
    ${LUA_DIR}/ldump.c
    ${LUA_DIR}/lfunc.c
    ${LUA_DIR}/lgc.c
    ${LUA_DIR}/llex.c
    ${LUA_DIR}/lmem.c
    ${LUA_DIR}/lobject.c
    ${LUA_DIR}/lopcodes.c
    ${LUA_DIR}/lparser.c
    ${LUA_DIR}/lstate.c
    ${LUA_DIR}/lstring.c
    ${LUA_DIR}/ltable.c
    ${LUA_DIR}/ltm.c
    ${LUA_DIR}/lundump.c
    ${LUA_DIR}/lvm.c
    ${LUA_DIR}/lzio.c
    ${LUA_DIR}/lauxlib.c
    ${LUA_DIR}/lbaselib.c
    ${LUA_DIR}/lcorolib.c
    ${LUA_DIR}/ldblib.c
    ${LUA_DIR}/liolib.c
    ${LUA_DIR}/lmathlib.c
    ${LUA_DIR}/loslib.c
    ${LUA_DIR}/lstrlib.c
    ${LUA_DIR}/ltablib.c
    ${LUA_DIR}/lutf8lib.c
)

add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${LUA_DIR})
target_link_libraries(lua PUBLIC m)

set(LUAF_SOURCES
    src/luaf_state.c
    src/luaf_sandbox.c
    src/luaf_dofile.c
    src/luaf_runtime.c
    src/luaf_http.c
    src/luaf_vector.c
)

add_library(luaf_lib STATIC ${LUAF_SOURCES})
target_include_directories(luaf_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${LUA_DIR})
target_link_libraries(luaf_lib PUBLIC lua)

add_executable(luaf host/main.c)
target_link_libraries(luaf PRIVATE luaf_lib lua m)

if(WIN32)
    target_link_libraries(luaf PRIVATE ws2_32)
endif()

file(COPY examples/ DESTINATION ${CMAKE_BINARY_DIR}/examples)
file(COPY modules/ DESTINATION ${CMAKE_BINARY_DIR}/modules)