//////////////////////////////////////////////////////////////////////////
//
// LuaCwrap - Lua <-> C 
// Copyright (C) 2011-2021 Klaus Oberhofer. See Copyright Notice in luacwrap.h
//
//////////////////////////////////////////////////////////////////////////
/**

  Wraps pointer members within 
  wrapped structs and implements set/get methods for them

*/////////////////////////////////////////////////////////////////////////

#include "wrappointer.h"

//////////////////////////////////////////////////////////////////////////
/**

  Implements set method for Pointer types. The reference
  system allows to assign lua strings and userdata and keeps
  them alive during the lifetime of the outer object.

*////////////////////////////////////////////////////////////////////////
static int luacwrap_pointer_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  PBYTE* v = (PBYTE*)pData;

  switch (lua_type(L, -1))
  {
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
      {
        *v = (PBYTE)lua_touserdata(L, -1);
        
        if (*v)
        {
          // store reference in outer value
          luacwrap_mobj_set_reference(L, 1, abs_index(L, -1), offset);
        }
        else
        {
          // remove a possible reference value (from a previously assigned value)
          luacwrap_mobj_remove_reference(L, 1, offset);
        }
      }
      break;
    case LUA_TSTRING:
      {
        *v = (PBYTE)lua_tostring(L, -1);

        // store reference in outer value
        luacwrap_mobj_set_reference(L, 1, abs_index(L, -1), offset);
      }
      break;
    case LUA_TNUMBER:
    case LUA_TNIL:
      {
        *v = (PBYTE)lua_tointeger(L, -1);
        
        // remove a possible reference value (from a previously assigned value)
        luacwrap_mobj_remove_reference(L, 1, offset);
      }
      break;
    default:
      {
        luaL_error(L, "userdata, string or number expected, got %s", luaL_typename(L, 4));
      }
      break;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements get method for Pointer types. This checks first
  the reference system to get the lua type behind the pointer.
  Otherwise it returns the pointer as a lightuserdata.

*////////////////////////////////////////////////////////////////////////
static int luacwrap_pointer_get(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // try to get referenced lua value from outer struct
  if (!luacwrap_mobj_get_reference(L, 1, offset))
  {
    // otherwise return raw pointer as light userdata
    PBYTE* v = (PBYTE*)pData;
    if (*v)
    {
      lua_pushlightuserdata(L, *v);
    }
    else
    {
      lua_pushnil(L);
    }
  }

  return 1;
}

luacwrap_BasicType regType_Pointer =
{
  {
    LUACWRAP_TC_BASIC,
    "$ptr"
  },
  sizeof(PBYTE),
  luacwrap_pointer_get,
  luacwrap_pointer_set
};
