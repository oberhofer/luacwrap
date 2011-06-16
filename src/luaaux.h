//////////////////////////////////////////////////////////////////////////
// LuaCwrap - lua <-> C 
// Copyright (C) 2011 Klaus Oberhofer. See Copyright Notice in luacwrap.h
//
//////////////////////////////////////////////////////////////////////////
/**

  Auxiliary helper functions for debugging
  
  The origin of this code is the LuaCOM project.
  LuaCOM is also under the same license as LuaCwrap (MIT license).

*/////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <assert.h>
#ifdef __cplusplus
}
#endif

typedef int stkIndex;

void luaaux_printLuaTable(lua_State *L, stkIndex index);
void luaaux_printLuaStack(lua_State *L);
void luaaux_printPreDump(int expected);
const char* luaaux_makeLuaErrorMessage(int return_value, const char* msg);

#include <assert.h>

#ifdef _DEBUG
#define LUASTACK_SET(L) const int __LuaAux_luastack_top_index = lua_gettop(L)
#else
#define LUASTACK_SET(L)
#endif

#ifdef _DEBUG
#define LUASTACK_CLEAN(L, n) if((__LuaAux_luastack_top_index + n) != lua_gettop(L)) { luaaux_printPreDump(__LuaAux_luastack_top_index + n); luaaux_printLuaStack(L); assert(0); }
#else
#define LUASTACK_CLEAN(L, n)
#endif

