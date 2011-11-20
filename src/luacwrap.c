//////////////////////////////////////////////////////////////////////////
// LuaCwrap - lua <-> C 
// Copyright (C) 2011 Klaus Oberhofer. See Copyright Notice in luacwrap.h
//
//////////////////////////////////////////////////////////////////////////
/**

  Lua C wrapper library

*/////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>

#include "luaaux.h"
#include "luacwrap.h"
#include "wrapnumeric.h"


// address of this string is used as key to register module table _M
const char* g_keyLibraryTable = "luacwrap";


// convert a stack index to an absolute (positive) index
#define abs_index(L, i)   ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : \
                          lua_gettop(L) + (i) + 1) 

// forward declarations
int getEmbedded(lua_State* L, int ud, int offset, const char* typname);
int setEmbedded(lua_State* L, int ud, int val, int offset, const char* typname);

//////////////////////////////////////////////////////////////////////////
/**

  Find a member within a members array by name. 
  Uses slow linear search.

*////////////////////////////////////////////////////////////////////////
luacwrap_RecordMember* findMember(luacwrap_RecordMember* members, const char* name)
{
  luacwrap_RecordMember* result = members;
  while (result->name)
  {
    if (0 == strcmp(result->name, name))
    {
      return result;
    }
    ++result;
  };
  return NULL;
}

//////////////////////////////////////////////////////////////////////////
/**

  Get reference from environment of managed object to get a 
  pointer referenced value.
  The offset of the pointer is used as index in the reference table.

*////////////////////////////////////////////////////////////////////////
int luacwrap_type_get_reference(lua_State *L, int ud, int offset)
{
  LUASTACK_SET(L);

  // get environment
  lua_getfenv(L, ud);
  if (!lua_isnil(L, -1))
  {
    PBYTE pUdata;

    // result = env[offset]
    lua_rawgeti(L, -1, offset);
    pUdata = (PBYTE)lua_touserdata(L, ud);

    // check if reference is present
    if (!lua_isnil(L, -1))
    {
      // remove environment table
      lua_remove(L, -2);

      LUASTACK_CLEAN(L, 1);
      return 1;
    }
    else
    {
      // printf("not found: %i\n", offset);
    }

    lua_pop(L, 2);
  }
  
  LUASTACK_CLEAN(L, 0);
  return 0;
}


//////////////////////////////////////////////////////////////////////////
/**

  Set reference within environment of managed object to a 
  pointer referenced value.
  The offset of the pointer is used as index in the reference table.

*////////////////////////////////////////////////////////////////////////
int luacwrap_type_set_reference(lua_State *L, int ud, int value, int offset)
{
  LUASTACK_SET(L);

  // create environment if not already present
  lua_getfenv(L, ud);
  if (lua_isnil(L, -1))
  {
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfenv(L, ud);
  }

  // env[offset] = value
  lua_pushvalue(L, value);
  lua_rawseti(L, -2, offset);
  
  // pop environment table
  lua_pop(L, 1);  
  
  LUASTACK_CLEAN(L, 0);

  return 0;
}


//////////////////////////////////////////////////////////////////////////
/**

  Determine the size of a type in [bytes].

*////////////////////////////////////////////////////////////////////////
int luacwrap_type_size(luacwrap_Type* desc)
{
  int size = 0;
  switch(desc->typeclass)
  {
    case LUACWRAP_TC_BASIC : size = ((luacwrap_BasicType*)desc)->size;  break;
    case LUACWRAP_TC_RECORD: size = ((luacwrap_RecordType*)desc)->size;  break;
    case LUACWRAP_TC_ARRAY :
      {
        luacwrap_ArrayType* arrdesc = (luacwrap_ArrayType*)desc;
        size = arrdesc->elemcount * arrdesc->elemsize;
      }
      break;
    case LUACWRAP_TC_BUFFER: size = ((luacwrap_BufferType*)desc)->size; break;
    default:
      {
        assert(0);
      }
  }
  return size;
}


//////////////////////////////////////////////////////////////////////////
/**

  Returns the type dependant length (for the # operator).

*////////////////////////////////////////////////////////////////////////
int luacwrap_type_len(luacwrap_Type* desc)
{
  int size = 0;
  switch(desc->typeclass)
  {
    case LUACWRAP_TC_BASIC : size = ((luacwrap_BasicType*)desc)->size;  break;
    case LUACWRAP_TC_RECORD: size = ((luacwrap_RecordType*)desc)->size;  break;
    case LUACWRAP_TC_ARRAY :
      {
        luacwrap_ArrayType* arrdesc = (luacwrap_ArrayType*)desc;
        size = arrdesc->elemcount;
      }
      break;
    case LUACWRAP_TC_BUFFER: size = ((luacwrap_BufferType*)desc)->size; break;
    default:
      {
        assert(0);
      }
  }
  return size;
}


//////////////////////////////////////////////////////////////////////////
/**

  Closure to implement get() on buffers and basic types

*////////////////////////////////////////////////////////////////////////
int luacwrap_get_closure(lua_State* L)
{
  int offset;
  const char* typname;

  offset = lua_tointeger(L, lua_upvalueindex(1));
  typname = lua_tostring(L, lua_upvalueindex(2));

  return getEmbedded(L, 1, offset, typname);
}

//////////////////////////////////////////////////////////////////////////
/**

  Closure to implement set() on buffers and basic types

*////////////////////////////////////////////////////////////////////////
int luacwrap_set_closure(lua_State* L)
{
  int offset;
  const char* typname;

  offset = lua_tointeger(L, lua_upvalueindex(1));
  typname = lua_tostring(L, lua_upvalueindex(2));

  return setEmbedded(L, 1, 2, offset, typname);
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements type dependant __index method. 
  The object pointer is determined from the pointer to 
  the outer object and the given offset.

*////////////////////////////////////////////////////////////////////////
int luacwrap_type_index(lua_State* L, int ud, int offset, luacwrap_Type* desc)
{
  const char* stridx;

  LUASTACK_SET(L);

  ud = abs_index(L, ud);

  switch(desc->typeclass)
  {
    case LUACWRAP_TC_RECORD:
      {
        luacwrap_RecordMember* member;
        luacwrap_RecordType* recdesc = (luacwrap_RecordType*)desc;

        stridx = lua_tostring(L, 2);

        member = findMember(recdesc->members, stridx);
        if (member)
        {
          return getEmbedded(L, ud, offset+member->offset, member->typname);
        }
        else
        {
          // try to return methods from method table
          lua_getfenv(L, ud);
          if (!lua_isnil(L, -1))
          {
            lua_getfield(L, -1, "$methods");
            lua_getfield(L, -1, stridx);
            lua_replace(L, -3);
            lua_pop(L, 1);
            if (!lua_isnil(L, -1))
            {
              LUASTACK_CLEAN(L, 1);
              return 1;
            }
          }
          lua_pop(L, 1);
        }
      }
      break;
    case LUACWRAP_TC_ARRAY :
      {
        // determine offset and return inner wrapper
        luacwrap_ArrayType* arrdesc = (luacwrap_ArrayType*)desc;
        int idx = lua_tointeger(L, 2);

        if ((idx>0) && (idx <= arrdesc->elemcount))
        {
          int arroffs = (idx - 1) * arrdesc->elemsize;

          return getEmbedded(L, ud, offset+arroffs, arrdesc->elemtype);
        }
        // else return nil
      }
      break;
    case LUACWRAP_TC_BASIC :
    case LUACWRAP_TC_BUFFER:
      {
        // special handling for get/set
        stridx = lua_tostring(L, 2);
        if (0 == strcmp("get", stridx))
        {
          lua_pushinteger(L, offset);
          lua_pushstring(L, desc->name);
          lua_pushcclosure(L, luacwrap_get_closure, 2);

          LUASTACK_CLEAN(L, 1);
          return 1;
        }
        else if (0 == strcmp("set", stridx))
        {
          lua_pushinteger(L, offset);
          lua_pushstring(L, desc->name);
          lua_pushcclosure(L, luacwrap_set_closure, 2);

          LUASTACK_CLEAN(L, 1);
          return 1;
        }
      }
      break;
    default:
      {
        assert(0);
      }
  }

  // special handling for __ptr
  stridx = lua_tostring(L, 2);
  if (0 == strcmp("__ptr", stridx))
  {
    PBYTE pobj;
    pobj = (PBYTE)lua_touserdata(L, ud) + offset;
    lua_pushlightuserdata(L, pobj);
    LUASTACK_CLEAN(L, 1);
    return 1;
  }

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements type dependant __newindex method. 
  The object pointer is determined from the pointer to 
  the outer object and the given offset.

*////////////////////////////////////////////////////////////////////////
int luacwrap_type_newindex(lua_State* L, int ud, int offset, luacwrap_Type* desc)
{
  LUASTACK_SET(L);

  switch(desc->typeclass)
  {
    case LUACWRAP_TC_BASIC :
      {
        assert(0);
      }
      break;
    case LUACWRAP_TC_RECORD:
      {
        luacwrap_RecordMember* member;
        luacwrap_RecordType* recdesc = (luacwrap_RecordType*)desc;

        const char* stridx = lua_tostring(L, 2);

        member = findMember(recdesc->members, stridx);
        if (member)
        {
          return setEmbedded(L, ud, 3, offset+member->offset, member->typname);
        }
        else
        {
          luaL_error(L, "try to set unknown member <%s>", stridx);
        }
      }
      break;
    case LUACWRAP_TC_ARRAY :
      {
        // determine offset and return inner wrapper
        luacwrap_ArrayType* arrdesc = (luacwrap_ArrayType*)desc;
        int idx = lua_tointeger(L, 2);

        if ((idx>0) && (idx <= arrdesc->elemcount))
        {
          int arroffs = (idx - 1) * arrdesc->elemsize;

          return setEmbedded(L, ud, 3, offset+arroffs, arrdesc->elemtype);
        }
        else
        {
          luaL_error(L, "index out of bound");
        }
      }
      break;
    case LUACWRAP_TC_BUFFER:
      {
        assert(0);
      }
      break;
    default:
      {
        assert(0);
      }
  }

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements type dependant __tostring method.
  The object pointer is determined from the pointer to 
  the outer object and the given offset.

*////////////////////////////////////////////////////////////////////////
int luacwrap_type_tostring(lua_State* L, int ud, int offset, luacwrap_Type* desc)
{
  LUASTACK_SET(L);

  switch(desc->typeclass)
  {
    case LUACWRAP_TC_BASIC :
      {
        // call getter
        // return ((luacwrap_BasicType*)desc)->getWrapper((luacwrap_BasicType*)desc, L, p, 0);
      }
      break;
    case LUACWRAP_TC_RECORD:
      {
        // structtostring
        luacwrap_RecordMember* member;
        luacwrap_RecordType* recdesc = (luacwrap_RecordType*)desc;
        int idx = 1;

        lua_getglobal(L, "tostring");

        lua_getfield(L, LUA_GLOBALSINDEX, "table");
        lua_getfield(L, -1, "concat");
        // remove "table"
        lua_remove(L, -2);

        lua_newtable(L);
        lua_pushfstring(L, "{ __ptr = %p,\n", ((PBYTE)lua_touserdata(L, ud)) +  offset);
        lua_rawseti(L, -2, idx++);

        member = recdesc->members;
        while (member->name)
        {
          lua_pushstring(L, member->name);
          lua_rawseti(L, -2, idx++);

          lua_pushstring(L, " = ");
          lua_rawseti(L, -2, idx++);

          lua_pushvalue(L, -3);                                           // function to be called (tostring)
          getEmbedded(L, ud, offset + member->offset, member->typname);   // value to convert
          lua_call(L, 1, 1); 
          lua_rawseti(L, -2, idx++);

          lua_pushstring(L, ",\n");
          lua_rawseti(L, -2, idx++);

          ++member;
        }

        lua_pushstring(L, "}");
        lua_rawseti(L, -2, idx++);

        lua_call(L, 1, 1);

        // remove "tostring"
        lua_remove(L, -2);

        // string.gsub(res, "%z", "\\0")
        lua_getfield(L, LUA_GLOBALSINDEX, "string");
        lua_getfield(L, -1, "gsub");
        lua_remove(L, -2);

        lua_pushvalue(L, -2);
        lua_pushstring(L, "%z");
        lua_pushstring(L, "\\0");
        lua_call(L, 3, 1);

        // remove original string
        lua_remove(L, -2);

        LUASTACK_CLEAN(L, 1);
        return 1;
      }
      break;
    case LUACWRAP_TC_ARRAY :
      {
        // call tabletostring
        lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
        lua_rawget(L, LUA_REGISTRYINDEX);
        lua_getfield(L, -1, "tabletostring");
        lua_remove(L, -2);

        getEmbedded(L, ud, offset, desc->name);   // value to convert
        lua_call(L, 1, 1);

        LUASTACK_CLEAN(L, 1);
        return 1;
      }
      break;
    case LUACWRAP_TC_BUFFER:
      {
        const char* pobj;
        luacwrap_BufferType* bufdesc = (luacwrap_BufferType*)desc;

        pobj = (const char*)lua_touserdata(L, ud);

        // get buffer as string
        lua_pushlstring(L, pobj, bufdesc->size);

        LUASTACK_CLEAN(L, 1);
        return 1;
      }
      break;
    default:
      {
        assert(0);
      }
  }

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Gets type descriptor from ud._ENV["$desc"]

*////////////////////////////////////////////////////////////////////////
luacwrap_Type* luacwrap_getdescriptor(lua_State* L, int ud)
{
  luacwrap_Type* desc = 0;

  LUASTACK_SET(L);

  lua_getfenv(L, ud);
  lua_getfield(L, -1, "$desc");
  assert(!lua_isnil(L, -1));
  
  desc = (luacwrap_Type*)lua_touserdata(L, -1);

  lua_pop(L, 2);

  LUASTACK_CLEAN(L, 0);
  return desc;
}

//////////////////////////////////////////////////////////////////////////
/**

  Gets method table from a given name.
    - first tries a lookup in _M
    - then tries a lookup in _G

*////////////////////////////////////////////////////////////////////////
int luacwrap_getmethodtable_byname(lua_State* L, const char* name)
{
  LUASTACK_SET(L);

  // get access to _M[elemtype]
  lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
  lua_gettable(L, LUA_REGISTRYINDEX);
  lua_getfield(L, -1, "types");
  lua_getfield(L, -1, name);
  if (lua_isnil(L, -1))
  {
    // not found
    luaL_error(L, "Unknown type <%s>", name);
  }
  // remove _M and _M.types
  lua_replace(L, -3);
  lua_pop(L, 1);

  if (lua_istable(L, -1))
  {
    LUASTACK_CLEAN(L, 1);
    return 1;
  }

  lua_pop(L, 1);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Gets type descriptor from a given name.
    - first tries a lookup in _M
    - then tries a lookup in _G

*////////////////////////////////////////////////////////////////////////
luacwrap_Type* luacwrap_getdescriptor_byname(lua_State* L, const char* name)
{
  luacwrap_Type* result = NULL;

  LUASTACK_SET(L);

  // get access to _M[elemtype]
  lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
  lua_gettable(L, LUA_REGISTRYINDEX);
  lua_getfield(L, -1, "types");
  lua_getfield(L, -1, name);
  if (lua_isnil(L, -1))
  {
    // not found
    luaL_error(L, "Unknown type <%s>", name);
  }
  
  if (lua_istable(L, -1))
  {
    lua_getfield(L, -1, "$desc");
    result = (luacwrap_Type*)lua_touserdata(L, -1);
    lua_pop(L, 1);
  }
  else if (lua_isuserdata(L, -1))
  {
    result = (luacwrap_Type*)lua_touserdata(L, -1);
  }
  
  // remove _M, _M.types and $desc
  lua_pop(L, 3);
  
  if (!result)
  {
    luaL_error(L, "Could not get type descriptor for type <%s>", name);
  }
  
  LUASTACK_CLEAN(L, 0);
  return result;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements __index metamethod for an embedded object reference.

  Parameters on lua stack:
    - self  (userdata, embedded object)
    - index

*////////////////////////////////////////////////////////////////////////
int Embedded_index(lua_State* L)
{
  int result;
  luacwrap_Type* desc;
  luacwrap_EmbeddedObject* pobj;

  LUASTACK_SET(L);

  desc = luacwrap_getdescriptor(L, 1);
  pobj = (luacwrap_EmbeddedObject*)lua_touserdata(L, 1);

  lua_rawgeti(L, LUA_REGISTRYINDEX, pobj->outer);
  result = luacwrap_type_index(L, -1, pobj->offset, desc);

  lua_remove(L, -2);

  LUASTACK_CLEAN(L, result);
  return result;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements __newindex metamethod for an embedded object reference.

  Parameters on lua stack:
    - self  (userdata, embedded object)
    - index
    - value

*////////////////////////////////////////////////////////////////////////
int Embedded_newindex(lua_State* L)
{
  luacwrap_Type* desc;
  luacwrap_EmbeddedObject* pobj;

  LUASTACK_SET(L);

  desc = luacwrap_getdescriptor(L, 1);
  pobj = (luacwrap_EmbeddedObject*)lua_touserdata(L, 1);

  lua_rawgeti(L, LUA_REGISTRYINDEX, pobj->outer);

  luacwrap_type_newindex(L, abs_index(L, -1), pobj->offset, desc);
  lua_pop(L, 1);

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements __len metamethod for an embedded object reference.

  Parameters on lua stack:
    - self  (userdata, embedded object)

*////////////////////////////////////////////////////////////////////////
int Embedded_len(lua_State* L)
{
  luacwrap_Type* desc;

  LUASTACK_SET(L);

  desc = luacwrap_getdescriptor(L, 1);
  lua_pushinteger(L, luacwrap_type_len(desc));

  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements __tostring metamethod for an embedded object reference.

  Parameters on lua stack:
    - self  (userdata, embedded object)

*////////////////////////////////////////////////////////////////////////
int Embedded_tostring(lua_State* L)
{
  luacwrap_Type* desc;
  luacwrap_EmbeddedObject* pobj;

  LUASTACK_SET(L);

  desc = luacwrap_getdescriptor(L, 1);
  pobj = (luacwrap_EmbeddedObject*)lua_touserdata(L, 1);

  lua_rawgeti(L, LUA_REGISTRYINDEX, pobj->outer);
  assert(lua_isuserdata(L, -1));
  luacwrap_type_tostring(L, abs_index(L, -1), pobj->offset, desc);
  lua_remove(L, -2);

  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements __gc metamethod for an embedded object reference.
  Used to free the reference to the outer object.

  Parameters on lua stack:
    - self  (userdata, embedded object)

*////////////////////////////////////////////////////////////////////////
int Embedded_gc(lua_State* L)
{
  luacwrap_EmbeddedObject* pobj;

  LUASTACK_SET(L);

  pobj = (luacwrap_EmbeddedObject*)lua_touserdata(L, 1);

  luaL_unref(L, LUA_REGISTRYINDEX, pobj->outer);
  pobj->outer = LUA_REFNIL;

  LUASTACK_CLEAN(L, 0);
  return 0;
}

luaL_reg g_mtEmbedded[ ] = {
  { "__index"   , Embedded_index     },
  { "__newindex", Embedded_newindex  },
  { "__len"     , Embedded_len       },
  { "__tostring", Embedded_tostring  },
  { "__gc",       Embedded_gc  },
  { NULL, NULL }
};

//////////////////////////////////////////////////////////////////////////
/**

  Pushes an embedded object reference onto the lua stack

  userdata
  +-----------------+<----,     +--------------+<---- userdata
  |                 |     '-----| outer        |        metatable
  |                 |           +--------------+          [__index]     -> Embedded_index
  +-----------------+<----------| offset       |          [__newindex]  -> Embedded_newindex
  | Embedded object |           +--------------+          [__tostring]  -> Embedded_tostring
  |                 |                                     [__len]       -> Embedded_len
  +-----------------+                                   environment
  |                 |                                     [$desc]       -> typedescriptor
  |                 |
  +-----------------+

*////////////////////////////////////////////////////////////////////////
int  pushEmbedded(lua_State* L, int ud, int offset, luacwrap_Type* desc)
{
  luacwrap_EmbeddedObject* pobj;

  LUASTACK_SET(L);

  pobj = (luacwrap_EmbeddedObject*)lua_newuserdata(L, sizeof(luacwrap_EmbeddedObject));

  lua_pushvalue(L, ud);
  pobj->outer  = luaL_ref(L, LUA_REGISTRYINDEX);
  pobj->offset = offset;

  // get/attach metatable
  lua_pushlightuserdata(L, (void*)&g_mtEmbedded);
  lua_rawget(L, LUA_REGISTRYINDEX);
  assert(lua_istable(L, -1));
  lua_setmetatable(L, -2);

  // set _ENV[$desc] and _ENV[$methods]
  lua_newtable(L);
  if (luacwrap_getmethodtable_byname(L, desc->name))
  {
    lua_setfield(L, -2, "$methods");
  }
  lua_pushlightuserdata(L, desc);
  lua_setfield(L, -2, "$desc");
  lua_setfenv(L, -2);

  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  Get the embedded value. If typname references a basic type or a 
  buffer then the value is converted to a lua value.
  Otherwise an embedded object reference is created and returned.

*////////////////////////////////////////////////////////////////////////
int  getEmbedded(lua_State* L, int ud, int offset, const char* typname)
{
  luacwrap_Type* desc;

  LUASTACK_SET(L);

  // get descriptor from type name
  desc = luacwrap_getdescriptor_byname(L, typname);
  switch (desc->typeclass)
  {
    case LUACWRAP_TC_BASIC:
      {
        PBYTE pobj;
        luacwrap_BasicType* basdesc = (luacwrap_BasicType*)desc;

        pobj = (PBYTE)lua_touserdata(L, ud) + offset;

        return basdesc->getWrapper(basdesc, L, pobj, offset);
      }
      break;
    case LUACWRAP_TC_BUFFER:
      {
        const char* pobj;
        luacwrap_BufferType* bufdesc = (luacwrap_BufferType*)desc;

        pobj = (const char*)lua_touserdata(L, ud) + offset;

        // get buffer as string
        lua_pushlstring(L, pobj, bufdesc->size);

        return 1;
      }
      break;
    case LUACWRAP_TC_RECORD:
    case LUACWRAP_TC_ARRAY:
      {
        return pushEmbedded(L, ud, offset, desc);
      }
      break;
    default:
      {
        assert(0);
      }
      break;
  }

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Set the embedded value. If typname references a basic type or a 
  buffer then the value is written to the object.
  Assigning to records and arrays is currently not supported.

*////////////////////////////////////////////////////////////////////////
int setEmbedded(lua_State* L, int ud, int val, int offset, const char* typname)
{
  luacwrap_Type* desc;

  LUASTACK_SET(L);

  // get descriptor from string
  desc = luacwrap_getdescriptor_byname(L, typname);
  switch (desc->typeclass)
  {
    case LUACWRAP_TC_BASIC:
      {
        PBYTE pobj;
        luacwrap_BasicType* basdesc = (luacwrap_BasicType*)desc;

        pobj = (PBYTE)lua_touserdata(L, ud) + offset;

        lua_pushvalue(L, val);
        basdesc->setWrapper(basdesc, L, pobj, offset);
        lua_pop(L, 1);
      }
      break;
    case LUACWRAP_TC_BUFFER:
      {
        size_t length;
        PBYTE pobj;
        luacwrap_BufferType* bufdesc = (luacwrap_BufferType*)desc;

        // check for string
        const char* strval = lua_tolstring(L, val, &length);

        pobj = (PBYTE)lua_touserdata(L, ud) + offset;

        // limit length to maximum buffer size
        length = (bufdesc->size < length) ? bufdesc->size : length;

        // clear buffer and copy value
        memset(pobj, 0, bufdesc->size);
        memcpy(pobj, strval, length);
      }
      break;
    case LUACWRAP_TC_RECORD:
    case LUACWRAP_TC_ARRAY:
      {
        // TODO:
        luaL_error(L, "assignment to array/record not supported, yet");
      }
      break;
    default:
      {
        assert(0);
      }
      break;
  }

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements the __index metamethod for boxed (outer) objects.

  Parameters on lua stack:
  - self  (userdata, embedded object)
  - index

*////////////////////////////////////////////////////////////////////////
int Boxed_index(lua_State* L)
{
  luacwrap_Type* desc;

  desc = luacwrap_getdescriptor(L, 1);

  return luacwrap_type_index(L, 1, 0, desc);
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements the __newindex metamethod for boxed (outer) objects.

  Parameters on lua stack:
    - self  (userdata, embedded object)
    - index
    - value

*////////////////////////////////////////////////////////////////////////
int Boxed_newindex(lua_State* L)
{
  luacwrap_Type* desc;

  desc = luacwrap_getdescriptor(L, 1);

  return luacwrap_type_newindex(L, 1, 0, desc);
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements the __len metamethod for boxed (outer) objects.

  Parameters on lua stack:
    - self  (userdata, embedded object)

*////////////////////////////////////////////////////////////////////////
int Boxed_len(lua_State* L)
{
  luacwrap_Type* desc;

  LUASTACK_SET(L);

  desc = luacwrap_getdescriptor(L, 1);
  lua_pushinteger(L, luacwrap_type_len(desc));

  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements the __tostring metamethod for boxed (outer) objects.

  Parameters on lua stack:
  - self  (userdata, embedded object)

*////////////////////////////////////////////////////////////////////////
int Boxed_tostring(lua_State* L)
{
  luacwrap_Type* desc;

  desc = luacwrap_getdescriptor(L, 1);

  return luacwrap_type_tostring(L, abs_index(L, 1), 0, desc);
}

luaL_reg g_mtBoxed[ ] = {
  { "__index"   , Boxed_index     },
  { "__newindex", Boxed_newindex  },
  { "__len"     , Boxed_len},
  { "__tostring", Boxed_tostring},
  { NULL, NULL }
};


//////////////////////////////////////////////////////////////////////////
/**

  Implements the new() constructor method for boxed types. It
    - creates udata with size determined from type descriptor
    - attaches the metatable for boxed objects
    - adds a type descriptor to environment[$desc]

  userdata
  +-----------------+<------------------- userdata
  |                 |                       metatable
  |                 |                         [__index]     -> Boxed_index
  +-----------------+                         [__newindex]  -> Boxed_newindex
  | Embedded object |                         [__tostring]  -> Boxed_tostring
  |                 |                         [__len]       -> Boxed_len
  +-----------------+                       environment
  |                 |                         [$desc]       -> typedescriptor
  |                 |                         [$methods]    -> method table
  +-----------------+


  Parameters on lua stack:
  - self  (userdata, embedded object)

*////////////////////////////////////////////////////////////////////////
int luacwrap_type_new(lua_State* L)
{
  luacwrap_Type* desc;
  size_t udsize;
  void* ud;

  LUASTACK_SET(L);

  /*
    method table
    +-----------------+                     +-----------------+
    | $desc           |-------------------->| TypeDescriptor  |
    | + additional    |                     +-----------------+
    |   methods       |
    +-----------------+
      metatable
        [new]         -> Type_new
        [attach]      -> Type_attach
  */
  luaL_checktype(L, 1, LUA_TTABLE);
  
  // get descriptor
  lua_getfield(L, 1, "$desc");
  if (lua_isnil(L, -1))
  {
    luaL_error(L, "No descriptor found. Don't call new() on instances.");
  }
  desc = lua_touserdata(L, -1);
  lua_pop(L, 1);

  // determine size
  udsize = luacwrap_type_size(desc);

  // create userdate which holds array data
  ud = lua_newuserdata(L, udsize);
  memset(ud, 0, udsize);

  // get/attach metatable
  lua_pushlightuserdata(L, (void*)&g_mtBoxed);
  lua_rawget(L, LUA_REGISTRYINDEX);
  lua_setmetatable(L, -2);

  // set _ENV[$desc] and _ENV[$methods]
  lua_newtable(L);
  lua_pushlightuserdata(L, desc);
  lua_setfield(L, -2, "$desc");
  lua_pushvalue(L, 1);
  lua_setfield(L, -2, "$methods");
  lua_setfenv(L, -2);
  
  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  check a userdata type descriptor against a given type descriptor

*////////////////////////////////////////////////////////////////////////
LUACWRAP_API  void* luacwrap_checktype   ( lua_State*          L
                           , int                 ud
                           , luacwrap_Type*      desc)
{
  void* result;
  luacwrap_Type* uddesc;

  uddesc = luacwrap_getdescriptor(L, ud);
  if (desc != uddesc)
  {
    // type conversion allowed ?
    luaL_error(L, "Expected <%s> but got <%s> on param #%d"
      , desc ? desc->name : "nil", uddesc ? uddesc->name : "nil", ud);
  }

  // get address
  lua_getfield(L, ud, "__ptr");
  result = lua_touserdata(L, -1);
  lua_pop(L, 1);

  return result;
}


//////////////////////////////////////////////////////////////////////////
/**

  push a typed pointer to a not garbage collected object

*////////////////////////////////////////////////////////////////////////
LUACWRAP_API int luacwrap_pushtypedptr(lua_State* L, luacwrap_Type* desc, void* pObj)
{
  int result = 0;
  LUASTACK_SET(L);
  
  lua_pushlightuserdata(L, pObj);
  result = getEmbedded(L, abs_index(L, -1), 0, desc->name);
  lua_remove(L, -2);
  
  LUASTACK_CLEAN(L, result);
  return result;
}

//////////////////////////////////////////////////////////////////////////
/**

  creates the attach() constructor method for boxed types.

  userdata
  +-----------------+<------------------- userdata
  |                 |                       metatable
  |                 |                         [__index]     -> Boxed_index
  +-----------------+                         [__newindex]  -> Boxed_newindex
  | Embedded object |                         [__tostring]  -> Boxed_tostring
  |                 |                         [__len]       -> Boxed_len
  +-----------------+                       environment
  |                 |                         [$desc]       -> typedescriptor
  |                 |
  +-----------------+

  Parameters on lua stack:
  - self    (userdata, embedded object)
  - pointer (udata)

*////////////////////////////////////////////////////////////////////////
int luacwrap_type_attach(lua_State* L)
{
  luacwrap_Type* desc;
  int result = 0;

  LUASTACK_SET(L);
  
  luaL_checktype(L, 1, LUA_TTABLE);
    
  // get descriptor
  lua_getfield(L, 1, "$desc");
  if (lua_isnil(L, -1))
  {
    luaL_error(L, "No descriptor found. Don't call attach() on instances.");
  }
  desc = lua_touserdata(L, -1);
  lua_pop(L, 1);

  // check second parameter
  switch(lua_type(L, 2))
  {
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
      {
        result = getEmbedded(L, 2, 0, desc->name);
      }
      break;
    case LUA_TNUMBER:
      {
        lua_pushlightuserdata(L, (void*)lua_tointeger(L, 2));
        result = getEmbedded(L, abs_index(L, -1), 0, desc->name);
        lua_remove(L, -2);
      }
      break;
    default:
      {
        luaL_error(L, "userdata, string or number expected, got %s", luaL_typename(L, 4));
      }
      break;

  }

  LUASTACK_CLEAN(L, result);
  return result;
}

luaL_reg g_mtTypeCtors[ ] = {
  { "new"   , luacwrap_type_new     },
  { "attach", luacwrap_type_attach  },
  { NULL, NULL }
};

//////////////////////////////////////////////////////////////////////////
/**

  Implements __gc metamethod for userdata which references malloced
  data structures.

  @param[in]  L       lua state

*/////////////////////////////////////////////////////////////////////////
int luacwrap_malloc_gc(lua_State* L)
{
  LUASTACK_SET(L);
  
  // get descriptor
  lua_getfield(L, 1, "$desc");
  if (!lua_isnil(L, -1))
  {
    void* ptrtofree = lua_touserdata(L, -1);
    free(ptrtofree);
  }
  lua_pop(L, 1);
  
  LUASTACK_CLEAN(L, 0);
  return 0;
}

luaL_reg g_mtDynTypeCtors[ ] = {
  { "new"   , luacwrap_type_new     },
  { "attach", luacwrap_type_attach  },
  { "__gc",   luacwrap_malloc_gc  },
  { NULL, NULL }
};


//////////////////////////////////////////////////////////////////////////
/**

  Registers a basic type descriptor

  @param[in]  L       lua state
  @param[in]  desc    basic type descriptor

*/////////////////////////////////////////////////////////////////////////
LUACWRAP_API int luacwrap_registerbasictype(lua_State* L, luacwrap_BasicType* desc)
{
  LUASTACK_SET(L);

  // get module table from registry
  lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (!lua_istable(L, -1))
  {
    luaL_error(L, "module table not defined !");
  }

  lua_getfield(L, -1, "setfield");
  if (!lua_isfunction(L, -1))
  {
    luaL_error(L, "error while getting _M.setfield");
  }
  lua_pushvalue(L, -2);               // _M
  lua_getfield(L, -1, "types");
  lua_remove(L, -2);                  // get _M.types

  lua_pushstring(L, desc->hdr.name);  // typename
  lua_pushlightuserdata(L, desc);     // type descriptor

  // lua stack
  //  1: type descriptor
  //  1: typename
  //  1: _M.types
  //  2: func "setfield"
  //  1: _M

  // check if type is already registered within _M
  lua_getfield(L, -5, "getfield");
  if (!lua_isfunction(L, -1))
  {
    luaL_error(L, "error while getting _M.getfield");
  }
  lua_pushvalue(L, -4);           // _M.types
  lua_pushvalue(L, -4);           // typname
  lua_call(L, 2, 1);
  if (!lua_isnil(L, -1))
  {
    luaL_error(L, "type already registered <%s>", desc->hdr.name);
  }
  lua_pop(L, 1);                  // pop field

  // setfield(_M.types, basictype.name, basictype.descriptor)
  lua_call(L, 3, 0);

  // pop module table
  lua_pop(L, 1);

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements set method for Pointer types. The reference 
  system allows to assign lua strings and userdata and keeps
  them alive during the lifetime of the outer object.

*////////////////////////////////////////////////////////////////////////
static int luacwrap_pointer_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  int    setReference = 0;
  PBYTE* v = (PBYTE*)pData;

  switch (lua_type(L, -1))
  {
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
      {
        setReference = 1;
        *v = (PBYTE)lua_touserdata(L, -1);
      }
      break;
    case LUA_TSTRING:
      {
        setReference = 1;
        *v = (PBYTE)lua_tostring(L, -1);
      }
      break;
    case LUA_TNUMBER:
      {
        *v = (PBYTE)lua_tointeger(L, -1);
      }
      break;
    default:
      {
        luaL_error(L, "userdata, string or number expected, got %s", luaL_typename(L, 4));
      }
      break;
  }

  if (setReference)
  {
    // store reference in outer value
    luacwrap_type_set_reference(L, 1, abs_index(L, -1), offset);
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
  if (!luacwrap_type_get_reference(L, 1, offset))
  {
    // otherwise return raw pointer as light userdata
    PBYTE* v = (PBYTE*)pData;
    lua_pushlightuserdata(L, *v);
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


//////////////////////////////////////////////////////////////////////////
/**

  Implements set method for a special reference type which stores 
  references to lua objects via the lua reference system. 
  
*////////////////////////////////////////////////////////////////////////
static int luacwrap_reference_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  int* v = (int*)pData;

  LUASTACK_SET(L);

  *v = luaL_ref(L, abs_index(L, -1));

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

  LUASTACK_SET(L);

  lua_rawgeti(L, LUA_REGISTRYINDEX, *v);

  LUASTACK_CLEAN(L, 1);
  return 1;
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

//////////////////////////////////////////////////////////////////////////
/**

  The type registration function registers a given 
  type descriptor under it's name in the given namespace table.

  method table
  +-----------------+                     +-----------------+
  | $desc           |-------------------->| TypeDescriptor  |
  | + additional    |                     +-----------------+
  |   methods       |
  +-----------------+
    metatable
      [new]         -> Type_new
      [attach]      -> Type_attach
  
  @param[in]  L             lua state
  @param[in]  nsidx         index of (namespace) table
  @param[in]  desc          type descriptor

*/////////////////////////////////////////////////////////////////////////
LUACWRAP_API int luacwrap_registertype( lua_State*       L
                         , int              nsidx
                         , luacwrap_Type*   desc)
{
  int nsabs = abs_index(L, nsidx);

  LUASTACK_SET(L);

  // get module table from registry
  lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (!lua_istable(L, -1))
  {
    luaL_error(L, "module table not defined !");
  }

  // push key/value
  lua_pushstring(L, desc->name);

  // check if type is already registered within _M
  lua_getfield(L, -2, "getfield");
  if (!lua_isfunction(L, -1))
  {
    luaL_error(L, "error while getting _M.getfield");
  }
  lua_pushvalue(L, -3);           // _M
  lua_getfield(L, -1, "types");
  lua_remove(L, -2);              // get _M.types

  lua_pushvalue(L, -3);           // typname
  lua_call(L, 2, 1);
  if (!lua_isnil(L, -1))
  {
    luaL_error(L, "type already registered <%s>", desc->name);
  }
  lua_pop(L, 1);                  // pop field

  lua_getfield(L, -2, "setfield");
  if (!lua_isfunction(L, -1))
  {
    luaL_error(L, "error while getting _M.setfield");
  }

  // lua stack
  //  3: func "setfield"
  //  2: typname
  //  1: _M

  lua_pushvalue(L, -3);           // _M
  lua_getfield(L, -1, "types");
  lua_remove(L, -2);              // get _M.types
  lua_pushvalue(L, -3);           // typname

  // create method table
  lua_newtable(L);

  // store descriptor in methods["$desc"]
  lua_pushlightuserdata(L, desc);
  lua_setfield(L, -2, "$desc");  

  // metatable = ctor table
  lua_pushlightuserdata(L, (void*)&g_mtTypeCtors);
  lua_rawget(L, LUA_REGISTRYINDEX);
  assert(lua_istable(L, -1));
  lua_setmetatable(L, -2);

  // lua stack
  //  6: method table
  //  5: typname
  //  4: _M.types
  //  3: func "setfield"
  //  2: typname
  //  1: _M

  // _namespace[typename] = method table
  lua_pushvalue(L, -4);           // func "setfield"
  lua_pushvalue(L, nsabs);        // namespace
  lua_pushvalue(L, -4);           // typname
  lua_pushvalue(L, -4);           // method table
  lua_call(L, 3, 0);

  // setfield(_M, typname,. method_table)
  lua_call(L, 3, 0);

  // pop module table and type name
  lua_pop(L, 2);

  LUASTACK_CLEAN(L, 0);
  return 0;
}


//////////////////////////////////////////////////////////////////////////
/**

  Stores string value in global table and prevents collection

  @param[in]  L       lua state
  @param[in]  idx     stack index of string value
  @param[in]  errmsg  error message if param is no string with len > 0

  Parameters on lua stack:
    - name ("buffer")
    - size in bytes (8)

*/////////////////////////////////////////////////////////////////////////
const char* luacwrap_storestring(lua_State* L, int idx, const char* errmsg, int errint)
{
  const char* result;
  size_t len;

  LUASTACK_SET(L);

  result = lua_tolstring(L, idx, &len);
  if ((0 == result) || (0 == len))
  {
    luaL_error(L, errmsg, errint);
  }
  else
  {
    // get access to _M.stringtable
    lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
    lua_gettable(L, LUA_REGISTRYINDEX);
    lua_getfield(L, -1, "stringtable");
    if (lua_isnil(L, -1))
    {
      lua_pop(L, 1);
      lua_newtable(L);
      lua_pushvalue(L, -1);
      lua_setfield(L, -3, "stringtable");
    }
    assert(lua_istable(L, -1));
    
    // lookup in _M.stringtable
    lua_pushvalue(L, idx);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1))
    {
      lua_pushvalue(L, idx);
      lua_pushboolean(L, 1);
      lua_rawset(L, -4);
    }
    // remove val, _M and _M.stringtable
    lua_pop(L, 3);
  }

  LUASTACK_CLEAN(L, 0);
  return result;
}

//////////////////////////////////////////////////////////////////////////
/**

  Creates a dynamically allocated type

  @param[in]  L             lua state
  @param[in]  desc          type descriptor

*/////////////////////////////////////////////////////////////////////////
int luacwrap_create_dyntype( lua_State*       L
                           , luacwrap_Type*   desc)
{
  LUASTACK_SET(L);

  // create wrapped type descriptor with attached metatable
  // create method table
  lua_newtable(L);

  // store descriptor in methods["$desc"]
  lua_pushlightuserdata(L, desc);
  lua_setfield(L, -2, "$desc");  

  // metatable = ctor table
  lua_pushlightuserdata(L, (void*)&g_mtDynTypeCtors);
  lua_rawget(L, LUA_REGISTRYINDEX);
  assert(lua_istable(L, -1));
  lua_setmetatable(L, -2);

  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  Registers a buffer type

  @param[in]  L             lua state

  Parameters on lua stack:
    - name ("buffer")
    - size in bytes (8)

*/////////////////////////////////////////////////////////////////////////
int luacwrap_registerbuffer(lua_State*       L)
{
  luacwrap_BufferType* bufdesc;
  const char* name;
  int bufsize;

  // get parameters
  name = luacwrap_storestring(L, 1, "non empty string expected on parameter #%d", 1);
  bufsize = lua_tointeger(L, 2);

  // create record type descriptor
  bufdesc = malloc(sizeof(luacwrap_BufferType));
  if (!bufdesc)
  {
    luaL_error(L, "allocation failed");
  }

  bufdesc->hdr.typeclass = LUACWRAP_TC_BUFFER;
  bufdesc->hdr.name = name;
  bufdesc->size = bufsize;

  return luacwrap_create_dyntype(L, &bufdesc->hdr);
}

//////////////////////////////////////////////////////////////////////////
/**

  Registers a struct type

  @param[in]  L             lua state

  Parameters on lua stack:
    - name ("TESTSTRUCT")
    - size (8)
    - array of members (name, offset, type)
        { "member1", 0, "$i32" },
        { "member1", 4, "$i32" }

*/////////////////////////////////////////////////////////////////////////
int luacwrap_registerstruct( lua_State*       L)
{
  luacwrap_RecordType* recdesc;
  luacwrap_RecordMember* member;
  const char* name;
  int recsize;
  int nmembers;
  int allocsize;
  int idx;

  LUASTACK_SET(L);

  // get parameters
  name = luacwrap_storestring(L, 1, "non empty string expected on parameter #%d", 1);
  recsize = lua_tointeger(L, 2);
  luaL_checktype(L, 3, LUA_TTABLE);

  // get number of members
  nmembers  = lua_objlen(L, 3);

  // create record type descriptor
  allocsize = sizeof(luacwrap_RecordType) + 
              (sizeof(luacwrap_RecordMember) * (nmembers + 1));
  recdesc = malloc(allocsize);
  if (!recdesc)
  {
    luaL_error(L, "allocation failed");
  }
  memset(recdesc, 0, allocsize);
  member = (luacwrap_RecordMember*)(recdesc + 1);

  // fill members table
  idx = 1;
  while (idx < nmembers)
  {
    int memberoffset;
    const char* membername;
    const char* membertypename;

    lua_rawgeti(L, 3, idx);
    lua_rawgeti(L, -1, 1);
    membername = luacwrap_storestring(L, -1, "non empty string expected for member name on index #%d", idx);
    lua_rawgeti(L, -2, 2);
    memberoffset = lua_tointeger(L, -1);

    lua_rawgeti(L, -3, 3);
    membertypename = luacwrap_storestring(L, -1, "non empty string expected for member type name on index #%d", idx);

    member->name = membername;
    member->offset = memberoffset;
    member->typname = membertypename;

    ++idx;
  }

  recdesc->hdr.typeclass = LUACWRAP_TC_RECORD;
  recdesc->hdr.name = name;
  // recdesc->size = 
  // recdesc->members =

  return luacwrap_create_dyntype(L, &recdesc->hdr);
}

//////////////////////////////////////////////////////////////////////////
/**

  Registers an array type

  @param[in]  L             lua state

  Parameters on lua stack:
    - name ("INT32_8")
    - numelems (8)
    - elementtxpe ("$i32")

*/////////////////////////////////////////////////////////////////////////
int luacwrap_registerarray( lua_State*       L)
{
  luacwrap_ArrayType* arrdesc;
  const char* name;
  int elemcount;
  const char* elemtypename;
  luacwrap_Type* elemtype;
  size_t len;

  LUASTACK_SET(L);

  // get parameters
  name = lua_tolstring(L, 1, &len);
  if (0 == len)
  {
    luaL_error(L, "non empty string expected on parameter #1");
  }
  elemcount = lua_tointeger(L, 2);
  elemtypename = lua_tolstring(L, 1, &len);
  if (0 == len)
  {
    luaL_error(L, "non empty string expected on parameter #3");
  }

  elemtype = luacwrap_getdescriptor_byname(L, elemtypename);
  if (!elemtype)
  {
    luaL_error(L, "specified unknown type <%s> in parameter #3", elemtypename);
  }

  // create array type descriptor
  arrdesc = malloc(sizeof(luacwrap_ArrayType));
  if (!arrdesc)
  {
    luaL_error(L, "allocation failed");
  }

  arrdesc->hdr.typeclass = LUACWRAP_TC_ARRAY;
  arrdesc->hdr.name = name;
  arrdesc->elemcount = elemcount;
  arrdesc->elemsize = luacwrap_type_size(elemtype);
  arrdesc->elemtype = elemtypename;

  return luacwrap_create_dyntype(L, &arrdesc->hdr);
}

//////////////////////////////////////////////////////////////////////////
/**

  Creates a buffer with the given size

  @param[in]  L             lua state

  Parameters on lua stack:
    - size (16)

*/////////////////////////////////////////////////////////////////////////
int luacwrap_createbuffer(lua_State*       L)
{
  int bufsize;

  LUASTACK_SET(L);

  bufsize = lua_tointeger(L, 1);

  // get/create buffer type indexed by size under _M.buftypes
  lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
  lua_rawget(L, LUA_REGISTRYINDEX);
  lua_getfield(L, -1, "buftypes");
  lua_remove(L, -2);

  lua_pushinteger(L, bufsize);
  lua_rawget(L, -2);
  if (lua_isnil(L, -1))
  {
    // create new buffer type
    lua_pushfstring(L, "$buf%d", bufsize);
    lua_pushinteger(L, bufsize);
    luacwrap_registerbuffer(L);
  }

  // buffer type descriptor is on top
  // create a new buffer instance by calling new()
  luaL_getmetafield(L, -1, "new");
  lua_pushvalue(L, -2);
  lua_call(L, 1, 1);

  LUASTACK_CLEAN(L, 1);
  return 1;
}

char* create_moduletable = 
"  local _M = { types = {} }\n"
"  function _M.tabletostring(self)\n"
"    local res = { \"[\" .. #self .. \"]\" .. \" = {\" }\n"
"    for k = 1, #self do\n"
"      res[#res+1] = \"  \" .. k .. \" = \" .. tostring(self[k]) .. \",\"\n"
"    end\n"
"    res[#res+1] = \"}\"\n"
"    return table.concat(res, \"\\n\")\n"
"  end\n"
"  function _M.getfield(t, f)\n"
"    local v = t\n"
"    for w in string.gfind(f, \"[^%.]+\") do\n"
"      v = v[w]\n"
"    end\n"
"    return v\n"
"  end\n"
"  function _M.setfield(t, f, v)\n"
"    for w, d in string.gfind(f, \"([^%.]+)(.?)\") do\n"
"      if d == \".\" then\n"            // not last field?
"        t[w] = t[w] or {}\n"           // create table if absent
"        t = t[w]\n"                    // get the table
"      else\n"
"        t[w] = v\n"                    // assign last field
"      end\n"
"    end\n"
"  end\n"
"  return _M\n";

//////////////////////////////////////////////////////////////////////////
/**

  Initializes the luacwrap module.
    - creates the moduletable _M with the _M.tabletostring function
    - creates metatables for boxed, embedded objects
    - creates metatable for type wrappers
    - registers basic typed for different numeric types

  Parameters on lua stack:
    - module table

  @param[in]  L  pointer lua state

*/////////////////////////////////////////////////////////////////////////
LUACWRAP_API int luaopen_luacwrap(lua_State *L)
{
  LUASTACK_SET(L);

  // create module table
  luaL_dostring(L, create_moduletable);

  // add createstruct() and createarray() to module table
  lua_pushcfunction(L, luacwrap_registerbuffer);
  lua_setfield(L, -2, "registerbuffer");
  lua_pushcfunction(L, luacwrap_registerstruct);
  lua_setfield(L, -2, "registerstruct");
  lua_pushcfunction(L, luacwrap_registerarray);
  lua_setfield(L, -2, "registerarray");
  lua_pushcfunction(L, luacwrap_createbuffer);
  lua_setfield(L, -2, "createbuffer");

  // store module table in registry
  lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
  lua_pushvalue(L, -2);
  lua_rawset(L, LUA_REGISTRYINDEX);

  // create metatable for boxed objects and store it in registry
  lua_pushlightuserdata(L, g_mtBoxed);
  lua_newtable(L);
  luaL_register( L, NULL, g_mtBoxed);
  lua_rawset(L, LUA_REGISTRYINDEX);

  // create metatable for embedded objects and store it in registry
  lua_pushlightuserdata(L, g_mtEmbedded);
  lua_newtable(L);
  luaL_register( L, NULL, g_mtEmbedded);
  lua_rawset(L, LUA_REGISTRYINDEX);

  // create metatable for type wrappers and store it in registry
  lua_pushlightuserdata(L, g_mtTypeCtors);
  lua_newtable(L);
  luaL_register( L, NULL, g_mtTypeCtors);
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  lua_rawset(L, LUA_REGISTRYINDEX);

  // create metatable for dynamically created type wrappers 
  // and store it in registry
  lua_pushlightuserdata(L, g_mtDynTypeCtors);
  lua_newtable(L);
  luaL_register( L, NULL, g_mtDynTypeCtors);
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  lua_rawset(L, LUA_REGISTRYINDEX);

  // now register numeric basicTypes
  luacwrap_registerNumericTypes(L);

  // register pointer type
  luacwrap_registerbasictype(L, &regType_Pointer);
  
  // register reference type
  luacwrap_registerbasictype(L, &regType_Reference);

  LUASTACK_CLEAN(L, 1);
  return 1;
}


