//////////////////////////////////////////////////////////////////////////
// LuaCwrap - lua <-> C 
// Copyright (C) 2011-2021 Klaus Oberhofer. See Copyright Notice in luacwrap.h
//
//////////////////////////////////////////////////////////////////////////
/**

  Wraps reference members within 
  wrapped structs and implements set/get methods for them

*/////////////////////////////////////////////////////////////////////////

#include "wrapreference.h"
#include "luaaux.h"
#include "string.h"

//////////////////////////////////////////////////////////////////////////
/**

  access a reference object on the stack

  @param[in]  L       lua state
  @param[in]  index   stack index

*////////////////////////////////////////////////////////////////////////
static int* luacwrap_toreference(lua_State* L, int index)
{
  const char *msg;

  int* p = (int*)lua_touserdata(L, index);
  if (p != NULL)
  {
    if (lua_getmetatable(L, index))
    {
      lua_pushlightuserdata(L, g_mtReferences);
      lua_rawget(L, LUA_REGISTRYINDEX);
      if (lua_rawequal(L, -1, -2))
      {
        lua_pop(L, 2);
        return p;
      }
    }
  }

  msg = lua_pushfstring(L, "luacwrap: reference expected, got %s", luaL_typename(L, index));
  luaL_argerror(L, index, msg);

  return NULL;
}


//////////////////////////////////////////////////////////////////////////
/**

  creates a reference in the reference table

  @param[in]  L       lua state
  @param[in]  index   stack index

*////////////////////////////////////////////////////////////////////////
int luacwrap_createreference(lua_State* L, int index)
{
  int ref;
  int validx = abs_index(L, index);

  LUASTACK_SET(L);

  // get access to _M.reftable
  getmoduletable(L);
  lua_getfield(L, -1, g_keyRefTable);
  assert(lua_istable(L, -1));

  lua_pushvalue(L, validx);
  ref = luaL_ref(L, -2);

  // pop _M and _M.reftable
  lua_pop(L, 2);

  LUASTACK_CLEAN(L, 0);
  return ref;
}

//////////////////////////////////////////////////////////////////////////
/**

  release a reference from the reference table

  @param[in]  L       lua state

*////////////////////////////////////////////////////////////////////////
int luacwrap_release_reference(lua_State *L)
{
  int* pref = luacwrap_toreference(L, 1);

  LUASTACK_SET(L);

  if (pref && (*pref))
  {
    // get access to _M.reftable
    getmoduletable(L);
    lua_getfield(L, -1, g_keyRefTable);
    assert(lua_istable(L, -1));

    lua_pushnil(L);
    lua_rawseti(L, -2, *pref);

    // remove _M and _M.reftable
    lua_pop(L, 2);
  }

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  implements mt.__index for references

  @param[in]  L       lua state

*////////////////////////////////////////////////////////////////////////
int luacwrap_reference_index(lua_State *L)
{
  int* pref = luacwrap_toreference(L, 1);

  LUASTACK_SET(L);

  if (pref)
  {
    const char* stridx = lua_tostring(L, 2);
    if (0 == strcmp(stridx, "value"))
    {
      // get access to _M.reftable
      getmoduletable(L);
      lua_getfield(L, -1, g_keyRefTable);
      assert(lua_istable(L, -1));

      lua_rawgeti(L, -1, *pref);

      // remove _M and _M.reftable
      lua_replace(L, -3);
      lua_pop(L, 1);
    }
    else if (0 == strcmp(stridx, "release"))
    {
      // return release function
      lua_pushcfunction(L, luacwrap_release_reference);
    }
    else if (0 == strcmp(stridx, "ref"))
    {
      lua_pushinteger(L, *pref);
    }
    else
    {
      luaL_error(L, "try to access unknown field <%s>", stridx);
      LUASTACK_CLEAN(L, 0);
      return 0;
    }

    LUASTACK_CLEAN(L, 1);
    return 1;
  }
  return 0;
}

// metatable for references
luaL_Reg g_mtReferences[ ] = {
  { "__index", luacwrap_reference_index },
  { NULL, NULL }
};

//////////////////////////////////////////////////////////////////////////
/**

  pushes a reference object onto the stack

  @param[in]  L         lua state
  @param[in]  reference reference index

*////////////////////////////////////////////////////////////////////////
int luacwrap_pushreference(lua_State* L, int reference)
{
  int* ud;

  LUASTACK_SET(L);

  ud = lua_newuserdata(L, sizeof(int*));
  *ud = reference;
  lua_pushlightuserdata(L, g_mtReferences);
  lua_rawget(L, LUA_REGISTRYINDEX);
  lua_setmetatable(L, -2);

  LUASTACK_CLEAN(L, 1);
  return 1;
}


//////////////////////////////////////////////////////////////////////////
/**

  Implements set method for a special reference type which stores
  references to lua objects via the lua reference system.

*////////////////////////////////////////////////////////////////////////
static int luacwrap_reference_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  int* v = (int*)pData;

  LUASTACK_SET(L);

  *v = luacwrap_createreference(L, -1);

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements get method for for a special reference type which stores
  references to lua objects via the lua reference system.

*////////////////////////////////////////////////////////////////////////
static int luacwrap_reference_get(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  int* v = (int*)pData;
  return luacwrap_pushreference(L, *v);
}

//////////////////////////////////////////////////////////////////////////
/**

  !!! Attention !!!
  We do not control the lifetime of the lua object via __gc. So be sure
  to use this type only if the content of a pointer type is returned
  to the lua state.

*////////////////////////////////////////////////////////////////////////
luacwrap_BasicType regType_Reference =
{
  {
    LUACWRAP_TC_BASIC,
    "$ref"
  },
  sizeof(int),
  luacwrap_reference_get,
  luacwrap_reference_set
};