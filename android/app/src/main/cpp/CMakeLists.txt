cmake_minimum_required(VERSION 3.22.1)
project(lua_jni)

# Path to Lua source directory from cpp folder (5 levels up)
set(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../")

# Define Lua core and library source files
set(LUA_SOURCES
    "${LUA_DIR}/lapi.c"
    "${LUA_DIR}/lcode.c"
    "${LUA_DIR}/lctype.c"
    "${LUA_DIR}/ldebug.c"
    "${LUA_DIR}/ldo.c"
    "${LUA_DIR}/ldump.c"
    "${LUA_DIR}/lfunc.c"
    "${LUA_DIR}/lgc.c"
    "${LUA_DIR}/llex.c"
    "${LUA_DIR}/lmem.c"
    "${LUA_DIR}/lobject.c"
    "${LUA_DIR}/lopcodes.c"
    "${LUA_DIR}/lparser.c"
    "${LUA_DIR}/lstate.c"
    "${LUA_DIR}/lstring.c"
    "${LUA_DIR}/ltable.c"
    "${LUA_DIR}/ltm.c"
    "${LUA_DIR}/lundump.c"
    "${LUA_DIR}/lvm.c"
    "${LUA_DIR}/lzio.c"
    "${LUA_DIR}/lauxlib.c"
    "${LUA_DIR}/lbaselib.c"
    "${LUA_DIR}/ldblib.c"
    "${LUA_DIR}/liolib.c"
    "${LUA_DIR}/lmathlib.c"
    "${LUA_DIR}/loslib.c"
    "${LUA_DIR}/ltablib.c"
    "${LUA_DIR}/lstrlib.c"
    "${LUA_DIR}/lutf8lib.c"
    "${LUA_DIR}/loadlib.c"
    "${LUA_DIR}/lcorolib.c"
    "${LUA_DIR}/linit.c"
)

# Add the shared library target
add_library(lua_jni SHARED
    lua_jni.c
    ${LUA_SOURCES}
)

# Specify include directories
target_include_directories(lua_jni PRIVATE
    "${LUA_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Compile flags
target_compile_definitions(lua_jni PRIVATE
    LUA_USE_LINUX
)

# Link standard libraries (Android logging, math, dynamic loading)
target_link_libraries(lua_jni
    log
    m
    dl
)
