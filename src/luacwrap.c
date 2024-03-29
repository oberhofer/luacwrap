//////////////////////////////////////////////////////////////////////////
//
// LuaCwrap - Lua <-> C 
// Copyright (C) 2011-2021 Klaus Oberhofer. See Copyright Notice in luacwrap.h
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
#include "wrappointer.h"
#include "wrapreference.h"

// enable this to add reserved __ptr attribute for debugging
#define LUACWRAP_DEBUG_PTR

// define min if not defined via windef.h
#ifndef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

// address of this string is used as key to register module table _M
const char* g_keyLibraryTable = "luacwrap";

// _M.$buftypes to store references
const char* g_keyRefTable     = "reftable";

// _M.$buftypes to cache strings
const char* g_keyStringTable  = "stringtable";

// _M.$buftypes to cache buffer type descriptors
const char* g_keyBufTypes     = "buftypes";


// forward declarations
static int getEmbedded(lua_State* L, int ud, int offset, luacwrap_Type* desc);
static int setEmbedded(lua_State* L, int offset, luacwrap_Type* desc);
static int pushEmbedded(lua_State* L, int ud, int offset, luacwrap_Type* desc);

static int luacwrap_type_set(lua_State* L);
static int luacwrap_type_dup(lua_State* L);

static int luacwrap_type_size(luacwrap_Type* desc);

static int luacwrap_getouter(lua_State* L, int ud, int* offset);

static luacwrap_Type* luacwrap_getdescriptor(lua_State* L, int ud);
static luacwrap_Type* luacwrap_getdescriptor_byname(lua_State* L, const char* name, int namelen);

// function prototype for getting the outer object
// and the offset within the outer object
typedef int (*GET_OBJECTOUTER)(lua_State* L, int ud, int* offset);

// key under which the getouter function is stored
const char* g_keyGetOuter = "getouter";

//////////////////////////////////////////////////////////////////////////
/**

  get module table from registry

*////////////////////////////////////////////////////////////////////////
void getmoduletable(lua_State *L)
{
  lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
  lua_rawget(L, LUA_REGISTRYINDEX);
  if (!lua_istable(L, -1))
  {
    luaL_error(L, "module table not defined !");
  }
}

//////////////////////////////////////////////////////////////////////////
/**

  Find a member within a members array by name.
  Uses slow linear search.

*////////////////////////////////////////////////////////////////////////
static luacwrap_RecordMember* findMember(luacwrap_RecordMember* members, const char* name)
{
  luacwrap_RecordMember* result = members;
  while (result->membername)
  {
    if (0 == strcmp(result->membername, name))
    {
      return result;
    }
    ++result;
  };
  return NULL;
}

//////////////////////////////////////////////////////////////////////////
/**

  Get environment of managed object or nil for other objects

*////////////////////////////////////////////////////////////////////////
int luacwrap_getenvironment(lua_State *L, int ud)
{
#if (LUA_VERSION_NUM > 501)
  if (LUA_TUSERDATA == lua_type(L, ud))
  {
    lua_getuservalue(L, ud);
  }
  else
  {
    lua_pushnil(L);
  }
#else
  lua_getfenv(L, ud);
#endif
  return (!lua_isnil(L, -1));
}

//////////////////////////////////////////////////////////////////////////
/**

  Set environment of managed object or throw error

*////////////////////////////////////////////////////////////////////////
int luacwrap_setenvironment(lua_State *L, int ud)
{
#if (LUA_VERSION_NUM > 501)
  if (LUA_TUSERDATA == lua_type(L, ud))
  {
    lua_setuservalue(L, ud);
    return 1;
  }
  else
  {
    const char *msg = lua_pushfstring(L, "luacwrap: try to set environment for parameter of type <%s>", luaL_typename(L, ud));
    luaL_argerror(L, ud, msg);
    return 0;
  }
#else
  return lua_setfenv(L, ud);
#endif
}

//////////////////////////////////////////////////////////////////////////
/**

  Get reference from environment of managed object to get a
  pointer referenced value.
  The offset of the pointer is used as index in the reference table.

*////////////////////////////////////////////////////////////////////////
int luacwrap_mobj_get_reference(lua_State *L, int ud, int offset)
{
  LUASTACK_SET(L);

  if (luacwrap_getenvironment(L, ud))
  {
    // result = env[offset]
    lua_rawgeti(L, -1, offset);

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
      // no reference stored -> return 0
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
int luacwrap_mobj_set_reference(lua_State *L, int ud, int value, int offset)
{
  LUASTACK_SET(L);

  // create environment if not already present
  if (!luacwrap_getenvironment(L, ud))
  {
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    luacwrap_setenvironment(L, ud);
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

  Remove a reference within environment of managed object to a
  pointer referenced value.

*////////////////////////////////////////////////////////////////////////
int luacwrap_mobj_remove_reference(lua_State *L, int ud, int offset)
{
  LUASTACK_SET(L);
  
  // get environment
  if (luacwrap_getenvironment(L, ud))
  {
    // env[offset] = nil
    lua_pushnil(L);
    lua_rawseti(L, -2, offset);

    // pop environment table
    lua_pop(L, 1);
  }

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  copy reference table from source object to destination object

*////////////////////////////////////////////////////////////////////////
int luacwrap_mobj_copy_references(lua_State* L)
{
  LUASTACK_SET(L);

  luacwrap_Type* srcdesc = luacwrap_getdescriptor(L, -1);
  if (NULL == srcdesc)
  {
    // todo: 
  }

  int srcsize = luacwrap_type_size(srcdesc);

  // get environment from (outer) source
  if (luacwrap_getenvironment(L, -1))
  {
    // create destination environment if not already present
    if (!luacwrap_getenvironment(L, -3))
    {
      lua_pop(L, 1);
      lua_newtable(L);
      lua_pushvalue(L, -1);
      luacwrap_setenvironment(L, -4);
    }

    // copy source content to destination
    lua_pushnil(L);                    // first key
    while (0 != lua_next(L, -3))
    {
      // stack holds key and value
      if (lua_type(L, -2) == LUA_TNUMBER) 
      {
        // check if index is within range of source object
        lua_Number v = lua_tonumber(L, -2);
        // v -= srcbase;
        if ((v >= 0) && (v <= srcsize))
        {
          // transfer key into range of destination object
          // lua_pushnumber(L, v + destoffset);
          lua_pushnumber(L, v);
          lua_pushvalue(L, -2);
          lua_rawset(L, -5);
        }
      }

      // removes 'value'; keeps 'key' for next iteration
      lua_pop(L, 1);
    }

    // pop environment table
    lua_pop(L, 1);
  }

  // pop environment table
  lua_pop(L, 1);

  LUASTACK_CLEAN(L, 0);

  return 0;
}


//////////////////////////////////////////////////////////////////////////
/**

  Determine the size of a type in [bytes].

*////////////////////////////////////////////////////////////////////////
static int luacwrap_type_size(luacwrap_Type* desc)
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
static int luacwrap_type_len(luacwrap_Type* desc)
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
static int luacwrap_get_closure(lua_State* L)
{
  int offset;
  const char* typname;
  luacwrap_Type* desc;
  size_t len;

  offset = lua_tointeger(L, lua_upvalueindex(1));
  typname = lua_tolstring(L, lua_upvalueindex(2), &len);

  // get descriptor from type name
  desc = luacwrap_getdescriptor_byname(L, typname, len);

  return getEmbedded(L, 1, offset, desc);
}

//////////////////////////////////////////////////////////////////////////
/**

  Closure to implement set() on buffers and basic types

*////////////////////////////////////////////////////////////////////////
static int luacwrap_set_closure(lua_State* L)
{
  int offset;
  const char* typname;
  luacwrap_Type* desc;
  size_t len;

  offset = lua_tointeger(L, lua_upvalueindex(1));
  typname = lua_tolstring(L, lua_upvalueindex(2), &len);

  // get descriptor from type name
  desc = luacwrap_getdescriptor_byname(L, typname, len);

  return setEmbedded(L, offset, desc);
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements type dependant __index method.
  The object pointer is determined from the pointer to
  the outer object and the given offset.

*////////////////////////////////////////////////////////////////////////
static int luacwrap_type_index(lua_State* L, int ud, int offset, luacwrap_Type* desc)
{
  size_t len;
  const char* stridx;

  LUASTACK_SET(L);

  ud = abs_index(L, ud);

  switch(desc->typeclass)
  {
    case LUACWRAP_TC_RECORD:
      {
        luacwrap_RecordMember* member;
        luacwrap_RecordType* recdesc = (luacwrap_RecordType*)desc;

        stridx = lua_tolstring(L, 2, &len);

        member = findMember(recdesc->members, stridx);
        if (member)
        {
          if (NULL == member->membertypedesc)
          {
            luacwrap_Type* desc;

            // get descriptor from type name
            desc = luacwrap_getdescriptor_byname(L, member->membertypename, -1);

            // cache type descriptor
            member->membertypedesc = desc;
          }

          return getEmbedded(L, ud, offset+member->memberoffset, member->membertypedesc);
        }
        else if (0 == strcmp(stridx, "__dup"))
        {
          lua_pushcfunction(L, luacwrap_type_dup);
          LUASTACK_CLEAN(L, 1);
          return 1;
        }
        else
        {
          // try to return methods from method table
          if (luacwrap_getenvironment(L, ud))
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

        if (NULL == arrdesc->elemtypedesc)
        {
          luacwrap_Type* desc;

          // get descriptor from type name
          desc = luacwrap_getdescriptor_byname(L, arrdesc->elemtypename, -1);

          // cache type descriptor
          arrdesc->elemtypedesc = desc;
        }

        if ((idx>0) && (idx <= arrdesc->elemcount))
        {
          int arroffs = (idx - 1) * arrdesc->elemsize;

          return getEmbedded(L, ud, offset+arroffs, arrdesc->elemtypedesc);
        }
        // else return nil
      }
      break;
    case LUACWRAP_TC_BASIC :
    case LUACWRAP_TC_BUFFER:
      {
        // special handling for get/set
        stridx = lua_tolstring(L, 2, &len);
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

#ifdef LUACWRAP_DEBUG_PTR
  // special handling for __ptr
  stridx = lua_tolstring(L, 2, &len);
  if (0 == strcmp("__ptr", stridx))
  {
    PBYTE pobj;
    pobj = (PBYTE)lua_touserdata(L, ud) + offset;
    lua_pushlightuserdata(L, pobj);
    LUASTACK_CLEAN(L, 1);
    return 1;
  }
#endif

  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements type dependant __newindex method.
  The object pointer is determined from the pointer to
  the outer object and the given offset.

  Parameters on lua stack:
  - self  (userdata, embedded object)  -3
  - index                              -2
  - value                              -1

*////////////////////////////////////////////////////////////////////////
static int luacwrap_type_newindex(lua_State* L, int offset, luacwrap_Type* desc)
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

        const char* stridx = lua_tostring(L, -2);

        member = findMember(recdesc->members, stridx);
        if (member)
        {
          if (NULL == member->membertypedesc)
          {
            luacwrap_Type* desc;

            // get descriptor from type name
            desc = luacwrap_getdescriptor_byname(L, member->membertypename, -1);

            // cache type descriptor
            member->membertypedesc = desc;
          }

          return setEmbedded(L, offset+member->memberoffset, member->membertypedesc);
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
        int idx = lua_tointeger(L, -2);

        if (NULL == arrdesc->elemtypedesc)
        {
          luacwrap_Type* desc;

          // get descriptor from type name
          desc = luacwrap_getdescriptor_byname(L, arrdesc->elemtypename, -1);

          // cache type descriptor
          arrdesc->elemtypedesc = desc;
        }

        if ((idx>0) && (idx <= arrdesc->elemcount))
        {
          int arroffs = (idx - 1) * arrdesc->elemsize;

          return setEmbedded(L, offset+arroffs, arrdesc->elemtypedesc);
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
static int luacwrap_type_tostring(lua_State* L, int ud, int offset, luacwrap_Type* desc)
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

        lua_getglobal(L, "table");
        lua_getfield(L, -1, "concat");
        // remove "table"
        lua_remove(L, -2);

        lua_newtable(L);
        lua_pushfstring(L, "{ __ptr = %p,\n", ((PBYTE)lua_touserdata(L, ud)) +  offset);
        lua_rawseti(L, -2, idx++);

        member = recdesc->members;
        while (member->membername)
        {
          if (NULL == member->membertypedesc)
          {
            luacwrap_Type* desc;

            // get descriptor from type name
            desc = luacwrap_getdescriptor_byname(L, member->membertypename, -1);

            // cache type descriptor
            member->membertypedesc = desc;
          }

          lua_pushstring(L, member->membername);
          lua_rawseti(L, -2, idx++);

          lua_pushstring(L, " = ");
          lua_rawseti(L, -2, idx++);

          lua_pushvalue(L, -3);                                           // function to be called (tostring)
          getEmbedded(L, ud, offset + member->memberoffset, member->membertypedesc);   // value to convert
          if (lua_isnumber(L, -1))
          {
            lua_call(L, 1, 1);                                            // call tostring
            lua_rawseti(L, -2, idx++);
          }
          else
          {
            lua_call(L, 1, 1);                                            // call tostring
            lua_pushstring(L, "[[");
            lua_rawseti(L, -3, idx++);
            lua_rawseti(L, -2, idx++);
            lua_pushstring(L, "]]");
            lua_rawseti(L, -2, idx++);
          }
          
          lua_pushstring(L, ",\n");
          lua_rawseti(L, -2, idx++);

          ++member;
        }

        lua_pushstring(L, "}");
        lua_rawseti(L, -2, idx++);

        lua_call(L, 1, 1);                                                // call concat

        // remove "tostring"
        lua_remove(L, -2);

        // string.gsub(res, "%z", "\\0")
        lua_getglobal(L, "string");
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
        luacwrap_ArrayType* arrdesc = (luacwrap_ArrayType*)desc;
        
        if (NULL == arrdesc->elemtypedesc)
        {
          luacwrap_Type* desc;

          // get descriptor from type name
          desc = luacwrap_getdescriptor_byname(L, arrdesc->elemtypename, -1);

          // cache type descriptor
          arrdesc->elemtypedesc = desc;
        }

        // maybe better use:
        //  if (&regType_char.hdr == arrdesc->elemtypedesc)
        if (1 == arrdesc->elemsize)
        {
          // if element type is 1 byte long convert directly to string
          const char* pobj;
          pobj = (const char*)lua_touserdata(L, ud) + offset;
        
          lua_pushlstring(L, pobj , arrdesc->elemsize * arrdesc->elemcount);
        }
        else
        {
          // call tabletostring
          getmoduletable(L);
          lua_getfield(L, -1, "tabletostring");
          lua_remove(L, -2);
          
          getEmbedded(L, ud, offset, desc);   // value to convert
          lua_call(L, 1, 1);
        }

        LUASTACK_CLEAN(L, 1);
        return 1;
      }
      break;
    case LUACWRAP_TC_BUFFER:
      {
        const char* pobj;
        luacwrap_BufferType* bufdesc = (luacwrap_BufferType*)desc;

        pobj = (const char*)lua_touserdata(L, ud) + offset;

        // get buffer as string
        lua_pushlstring(L, pobj, bufdesc->size);

        LUASTACK_CLEAN(L, 1);
        return 1;
      }
      break;
    default:
      {
        luaL_argerror(L, ud, "tostring not supported for this type");
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
static luacwrap_Type* luacwrap_getdescriptor(lua_State* L, int ud)
{
  luacwrap_Type* desc = 0;

  LUASTACK_SET(L);

  if (luacwrap_getenvironment(L, ud))
  {
    lua_getfield(L, -1, "$desc");
    if (!lua_isnil(L, -1))
    {
      desc = (luacwrap_Type*)lua_touserdata(L, -1);
    }
    // pop descriptor
    lua_pop(L, 1);
  }
  // pop ENV
  lua_pop(L, 1);

  LUASTACK_CLEAN(L, 0);
  return desc;
}

//////////////////////////////////////////////////////////////////////////
/**

  Gets method table from a given name.
    - first tries a lookup in _M
    - then tries a lookup in _G

*////////////////////////////////////////////////////////////////////////
static int luacwrap_getmethodtable_byname(lua_State* L, const char* name)
{
  LUASTACK_SET(L);

  // get access to _M[elemtype]
  getmoduletable(L);
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
static luacwrap_Type* luacwrap_getdescriptor_byname(lua_State* L, const char* name, int namelen)
{
  luacwrap_Type* result = NULL;

  LUASTACK_SET(L);

  if (-1 == namelen)
    namelen = strlen(name);

  // get access to _M[elemtype]
  getmoduletable(L);
  lua_getfield(L, -1, "types");
  lua_pushlstring(L, name, namelen);
  lua_rawget(L, -2);
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
static int Embedded_index(lua_State* L)
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
static int Embedded_newindex(lua_State* L)
{
  luacwrap_Type* desc;
  luacwrap_EmbeddedObject* pobj;

  desc = luacwrap_getdescriptor(L, 1);
  pobj = (luacwrap_EmbeddedObject*)lua_touserdata(L, 1);

  lua_rawgeti(L, LUA_REGISTRYINDEX, pobj->outer);
  lua_replace(L, 1);

  return luacwrap_type_newindex(L, pobj->offset, desc);
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements __len metamethod for an embedded object reference.

  Parameters on lua stack:
    - self  (userdata, embedded object)

*////////////////////////////////////////////////////////////////////////
static int Embedded_len(lua_State* L)
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
static int Embedded_tostring(lua_State* L)
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
static int Embedded_gc(lua_State* L)
{
  luacwrap_EmbeddedObject* pobj;

  LUASTACK_SET(L);

  pobj = (luacwrap_EmbeddedObject*)lua_touserdata(L, 1);

  luaL_unref(L, LUA_REGISTRYINDEX, pobj->outer);
  pobj->outer = LUA_REFNIL;

  LUASTACK_CLEAN(L, 0);
  return 0;
}


//////////////////////////////////////////////////////////////////////////
/**

  gets the pointer to the outer object wrapped by the embedded object

*////////////////////////////////////////////////////////////////////////
static int Embedded_getouter(lua_State* L, int ud, int* offset)
{
  luacwrap_EmbeddedObject* pobj;
  pobj = (luacwrap_EmbeddedObject*)lua_touserdata(L, ud);

  // get pointer to outer object
  lua_rawgeti(L, LUA_REGISTRYINDEX, pobj->outer);
  *offset = pobj->offset;

  return 1;
}


luaL_Reg g_mtEmbedded[ ] = {
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

*////////////////////////////////////////////////////////////////////////
static int pushEmbedded(lua_State* L, int ud, int offset, luacwrap_Type* desc)
{
  luacwrap_EmbeddedObject* pobj;
  int fromoffset = 0;

  LUASTACK_SET(L);
  
  // special handling for offset = 0 (cast operation)
  if (0 == offset)
  {
    // same types -> push a copy of ud
    luacwrap_Type* fromdesc = luacwrap_getdescriptor(L, ud);
    if (fromdesc == desc)
    {
      lua_pushvalue(L, ud);
      LUASTACK_CLEAN(L, 1);
      return 1;
    }
  }

  pobj = (luacwrap_EmbeddedObject*)lua_newuserdata(L, sizeof(luacwrap_EmbeddedObject));

  // get the outer object
  if (luacwrap_getouter(L, ud, &fromoffset))
  {
    // outer object is now on lua stack
    // additional offset is in fromoffset
  }
  else
  {
    // this is a non wrapped object (pointer = light user data, or blob = userdata)
    // so wrap it
    fromoffset = 0;
    lua_pushvalue(L, ud);
  }
 
  // create new wrapper on outer object
  pobj->outer  = luaL_ref(L, LUA_REGISTRYINDEX);
  pobj->offset = fromoffset + offset;

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
  luacwrap_setenvironment(L, -2);

  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  Get the embedded value. If typname references a basic type or a
  buffer then the value is converted to a lua value.
  Otherwise an embedded object reference is created and returned.

*////////////////////////////////////////////////////////////////////////
static int getEmbedded(lua_State* L, int ud, int offset, luacwrap_Type* desc)
{
  LUASTACK_SET(L);

  // get descriptor from type name
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

  Parameters on lua stack:
    - self  (userdata, embedded object)  -3
    - index                              -2
    - value                              -1

*////////////////////////////////////////////////////////////////////////
static int setEmbedded(lua_State* L, int offset, luacwrap_Type* desc)
{
  LUASTACK_SET(L);

  // get descriptor from string
  switch (desc->typeclass)
  {
    case LUACWRAP_TC_BASIC:
      {
        PBYTE pobj;
        luacwrap_BasicType* basdesc = (luacwrap_BasicType*)desc;

        pobj = (PBYTE)lua_touserdata(L, -3) + offset;

        lua_pushvalue(L, -1);
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
        const char* strval = lua_tolstring(L, -1, &length);

        pobj = (PBYTE)lua_touserdata(L, -3) + offset;

        // limit length to maximum buffer size
        length = (bufdesc->size < length) ? bufdesc->size : length;

        // copy value and clear rest of buffer
        memcpy(pobj, strval, length);
        memset(pobj + length, 0, bufdesc->size - length);
      }
      break;
    case LUACWRAP_TC_RECORD:
    case LUACWRAP_TC_ARRAY:
      {
        // recurse
        lua_pushcfunction(L, luacwrap_type_set);
        pushEmbedded(L, 1, offset, desc);           // push embedded
        lua_pushvalue(L, -3);                       // push value
        lua_call(L, 2, 0);
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
static int Boxed_index(lua_State* L)
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
static int Boxed_newindex(lua_State* L)
{
  luacwrap_Type* desc;

  desc = luacwrap_getdescriptor(L, 1);

  return luacwrap_type_newindex(L, 0, desc);
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements the __len metamethod for boxed (outer) objects.

  Parameters on lua stack:
    - self  (userdata, embedded object)

*////////////////////////////////////////////////////////////////////////
static int Boxed_len(lua_State* L)
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
static int Boxed_tostring(lua_State* L)
{
  luacwrap_Type* desc;

  desc = luacwrap_getdescriptor(L, 1);

  return luacwrap_type_tostring(L, abs_index(L, 1), 0, desc);
}

//////////////////////////////////////////////////////////////////////////
/**

  gets the pointer to the boxed object

*////////////////////////////////////////////////////////////////////////
static int Boxed_getouter(lua_State* L, int ud, int* offset)
{
  lua_pushvalue(L, ud);
  *offset = 0;
  return 1;
}

luaL_Reg g_mtBoxed[ ] = {
  { "__index"   , Boxed_index     },
  { "__newindex", Boxed_newindex  },
  { "__len"     , Boxed_len},
  { "__tostring", Boxed_tostring},
  { NULL, NULL }
};

//////////////////////////////////////////////////////////////////////////
/**

  Pushes a boxed object on the lua stack 
  (C-API equivalent to TYPE:new function)

  @param[in]  L       lua state
  @param[in]  desc    basic type descriptor
  @param[in]  initval value used to initialize object memory with
  
  @return pointer to raw object memory

*////////////////////////////////////////////////////////////////////////
void* luacwrap_pushboxedobj( lua_State*            L
                           , luacwrap_Type*        desc
                           , int                   initval)
{
  size_t udsize;
  void* ud;

  LUASTACK_SET(L);
  
  // determine size
  udsize = luacwrap_type_size(desc);

  // create userdata which holds type instance
  ud = lua_newuserdata(L, udsize);

  // by clear memory with given value
  memset(ud, initval, udsize);
  
  // get/attach metatable
  lua_pushlightuserdata(L, (void*)&g_mtBoxed);
  lua_rawget(L, LUA_REGISTRYINDEX);
  assert(!lua_isnil(L, -1));
  lua_setmetatable(L, -2);

  // set _ENV[$desc] and _ENV[$methods]
  lua_newtable(L);
  if (luacwrap_getmethodtable_byname(L, desc->name))
  {
    lua_setfield(L, -2, "$methods");
  }
  lua_pushlightuserdata(L, desc);
  lua_setfield(L, -2, "$desc");
  luacwrap_setenvironment(L, -2);

  LUASTACK_CLEAN(L, 1);

  return ud;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements the new() constructor method for boxed types. It
    - creates udata with size determined from type descriptor
    - attaches the metatable for boxed objects
    - adds a type descriptor to environment[$desc]

  Parameters on lua stack:
    - self  (type descriptor)
    - init parameter (optional)

*////////////////////////////////////////////////////////////////////////
static int luacwrap_type_new(lua_State* L)
{
  size_t         udsize;
  void*          ud;
  int            initval;
  luacwrap_Type* desc;

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

  // push default init parameter
  int hassetparam = !lua_isnoneornil(L, 2);

  // if optional init parameter is a number use it to fill memory block
  initval = 0;
  if (lua_isnumber(L, 2))
  {
    // init memory with a given value
    initval = lua_tointeger(L, 2);
    hassetparam = 0;
  }

  // determine size
  udsize = luacwrap_type_size(desc);

  // create userdata which holds type instance
  ud = lua_newuserdata(L, udsize);

  // by clear memory with given value
  memset(ud, initval, udsize);
  
  // get/attach metatable
  lua_pushlightuserdata(L, (void*)&g_mtBoxed);
  lua_rawget(L, LUA_REGISTRYINDEX);
  lua_setmetatable(L, -2);

  // set _ENV[$desc] and _ENV[$methods]
  lua_newtable(L);
  lua_pushvalue(L, 1);
  lua_setfield(L, -2, "$methods");
  lua_pushlightuserdata(L, desc);
  lua_setfield(L, -2, "$desc");
  luacwrap_setenvironment(L, -2);

  // if optional init table parameter present then call set()
  if (hassetparam)
  {
    lua_pushcfunction(L, luacwrap_type_set);
    lua_pushvalue(L, -2);         // push userdata
    lua_pushvalue(L,  2);         // push value
    lua_call(L, 2, 0);            // call set()
  }

  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  Implements the __dup() copy constructor method. It determines the
  type descriptor and calls new()

  Parameters on lua stack:
    - self  (object to duplicates)

*////////////////////////////////////////////////////////////////////////
static int luacwrap_type_dup(lua_State* L)
{
  luacwrap_Type* desc;

  LUASTACK_SET(L);

  desc = luacwrap_getdescriptor(L, 1);
  if (!desc)
  {
     return luaL_argerror(L, 1, "Failed to get descriptor");
  }

  lua_pushcfunction(L, luacwrap_type_new);
  if (!luacwrap_getmethodtable_byname(L, desc->name))
  {
    luaL_error(L, "Could not get method table for type %s", desc->name);
  }
  lua_pushvalue(L,  1);         // push value
  lua_call(L, 2, 1);            // call set()

  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  get memory descriptor (baseptr, offset, size) of given object

*////////////////////////////////////////////////////////////////////////
static int luacwrap_getouter(lua_State* L, int ud, int* offset)
{
  if (luaL_getmetafield(L, ud, g_keyGetOuter))
  {
    GET_OBJECTOUTER getouter = (GET_OBJECTOUTER)lua_touserdata(L, -1);
    lua_pop(L, 1);

    return getouter(L, ud, offset);
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  get the memory pointer to a given object

*////////////////////////////////////////////////////////////////////////
void* luacwrap_mobj_getbaseptr(lua_State* L, int ud)
{
  int offset;
  if (luacwrap_getouter(L, ud, &offset))
  {
    PBYTE baseptr = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return (baseptr + offset);
  }
  else
  {
    PBYTE baseptr = lua_touserdata(L, -1);
    return baseptr;
  }
}

//////////////////////////////////////////////////////////////////////////
/**

  check a userdata type descriptor against a given type descriptor

*////////////////////////////////////////////////////////////////////////
void* luacwrap_checktype   ( lua_State*          L
                           , int                 ud
                           , luacwrap_Type*      desc)
{
  luacwrap_Type* uddesc;

  uddesc = luacwrap_getdescriptor(L, ud);
  if (desc != uddesc)
  {
    // type conversion allowed ?
    luaL_error(L, "Expected <%s> but got <%s> on param #%d"
      , desc ? desc->name : "nil", uddesc ? uddesc->name : "nil", ud);
  }

  // getting address depends on if this is a boxed or an embedded object
  return luacwrap_mobj_getbaseptr(L, ud);
}


//////////////////////////////////////////////////////////////////////////
/**

  push a typed pointer to a not garbage collected object

*////////////////////////////////////////////////////////////////////////
int luacwrap_pushtypedptr(lua_State* L, luacwrap_Type* desc, void* pObj)
{
  int result = 0;
  LUASTACK_SET(L);

  lua_pushlightuserdata(L, pObj);
  result = getEmbedded(L, abs_index(L, -1), 0, desc);
  lua_remove(L, -2);

  LUASTACK_CLEAN(L, result);
  return result;
}

//////////////////////////////////////////////////////////////////////////
/**

  creates the attach() constructor method for boxed types.

  Parameters on lua stack:
    - self    (userdata, embedded object)
    - pointer (udata)

*////////////////////////////////////////////////////////////////////////
static int luacwrap_type_attach(lua_State* L)
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
      {
        result = getEmbedded(L, 2, 0, desc);
      }
      break;
    case LUA_TUSERDATA:
      {
        // check for __ptr field
        result = pushEmbedded(L, 2, 0, desc);
      }
      break;
    case LUA_TNUMBER:
      {
        lua_pushlightuserdata(L, (void*)lua_tointeger(L, 2));
        result = getEmbedded(L, abs_index(L, -1), 0, desc);
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

//////////////////////////////////////////////////////////////////////////
/**

  set attributes

  Parameters on lua stack:
  - self (userdata, embedded object)
  - data

  Return values on lua stack
  - self (userdata, embedded object)

*////////////////////////////////////////////////////////////////////////
static int luacwrap_type_set(lua_State* L)
{
  luacwrap_Type *desc, *descfrom;

  LUASTACK_SET(L);

  // get descriptor
  desc = luacwrap_getdescriptor(L, 1);
  if (!desc)
  {
     return luaL_argerror(L, 1, "Failed to get descriptor");
  }

  // init from table with init values
  if (lua_istable(L, 2))
  {
    switch(desc->typeclass)
    {
      case LUACWRAP_TC_RECORD:
        {
          // try to get __newindex metamethod
          luaL_getmetafield(L, 1, "__newindex");
          assert(LUA_TFUNCTION == lua_type(L, -1));

          lua_pushnil(L);  // first key
          while (0 != lua_next(L, 2))
          {
            // push __newindex function
            lua_pushvalue(L, -3);

            // push userdata
            lua_pushvalue(L, 1);

            // push key and value
            lua_pushvalue(L, -4);
            lua_pushvalue(L, -4);

            // push __newindex
            lua_call(L, 3, 0);

            // removes 'value'; keeps 'key' for next iteration
            lua_pop(L, 1);
          }

          // pop __newindex
          lua_pop(L, 1);
        }
        break;
      case LUACWRAP_TC_ARRAY :
        {
          int idx = 1;
          // check second parameter

          // try to get __newindex metamethod
          luaL_getmetafield(L, 1, "__newindex");
          assert(LUA_TFUNCTION == lua_type(L, -1));

          // push __newindex function
          lua_pushvalue(L, -1);

          // push userdata
          lua_pushvalue(L, 1);

          // push key
          lua_pushinteger(L, idx);

          // push value
          lua_rawgeti(L, 2, idx);
          while (!lua_isnil(L, -1))
          {
            // call __newindex
            lua_call(L, 3, 0);

            // push __newindex function
            lua_pushvalue(L, -1);

            // push userdata
            lua_pushvalue(L, 1);

            // next idx
            ++idx;
            lua_pushinteger(L, idx);
            lua_rawgeti(L, 2, idx);
          }

          // pop __newindex, __newindex, userdata, key and nil (value)
          lua_pop(L, 5);
        }
        break;
      case LUACWRAP_TC_BUFFER:
        {
          luaL_error(L, "Setting buffers via set/new from table is not supported, yet");
        }
        break;
      case LUACWRAP_TC_BASIC :
        {
          luaL_error(L, "Setting basic types via set/new from table is not supported, yet");
        }
        break;
      default:
        {
          assert(0);
        }
        break;
    }
  }
  else if (lua_isstring(L, 2))
  {
    switch(desc->typeclass)
    {
      case LUACWRAP_TC_RECORD:
        {
          luaL_error(L, "Assigning string to record is not supported");
        }
        break;
      case LUACWRAP_TC_ARRAY :
        {
          PBYTE destbase;

          luacwrap_ArrayType* arrdesc = (luacwrap_ArrayType*)desc;
          
          if (NULL == arrdesc->elemtypedesc)
          {
            luacwrap_Type* desc;

            // get descriptor from type name
            desc = luacwrap_getdescriptor_byname(L, arrdesc->elemtypename, -1);

            // cache type descriptor
            arrdesc->elemtypedesc = desc;
          }

          destbase = luacwrap_mobj_getbaseptr(L, 1);

          // assign string to char array
          const char* src;
          size_t srclen, arrsize;
          src = lua_tolstring(L, 2, &srclen);

          arrsize = arrdesc->elemsize * arrdesc->elemcount;

          // limit size to copy
          if (arrsize < srclen)
            arrsize = srclen;

          // copy binary content
          memcpy( destbase
                , src
                , min(srclen, arrsize));
        }
        break;
      case LUACWRAP_TC_BUFFER:
      case LUACWRAP_TC_BASIC :
        {
          // you should not get here
          assert(0);
          luaL_error(L, "Assigning string to buffer/basic type is not supported");
        }
        break;
      default:
        {
          assert(0);
        }
        break;
    }
  }
  else if (!lua_isnil(L, 2))
  {
    // get descriptor
    descfrom = luacwrap_getdescriptor(L, 2);

    // check if same type
    if (desc == descfrom)
    {
      PBYTE srcbase, destbase;

      destbase = luacwrap_mobj_getbaseptr(L, 1);
      srcbase  = luacwrap_mobj_getbaseptr(L, 2);

      int destsize = luacwrap_type_size(desc);

      // copy binary content
      memcpy( destbase
            , srcbase
            , destsize);

      // copy object references
      luacwrap_mobj_copy_references(L);
    }
    else
    {
      luaL_error(L, "Assignment of incompatible types");
    }
  }

  // return ud (parameter 1) as first result
  lua_pushvalue(L, 1);

  LUASTACK_CLEAN(L, 1);
  return 1;
}

// used for static type descriptors
luaL_Reg g_mtTypeCtors[ ] = {
  { "new"   , luacwrap_type_new     },
  { "set"   , luacwrap_type_set     },
  { "attach", luacwrap_type_attach  },
  { NULL, NULL }
};

//////////////////////////////////////////////////////////////////////////
/**

  Implements __gc metamethod for userdata which references malloced
  data structures.

  @param[in]  L       lua state

*/////////////////////////////////////////////////////////////////////////
static int luacwrap_malloc_gc(lua_State* L)
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

// used for dynamically alloced type descriptors
luaL_Reg g_mtDynTypeCtors[ ] = {
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
int luacwrap_registerbasictype(lua_State* L, luacwrap_BasicType* desc)
{
  LUASTACK_SET(L);

  // get module table from registry
  getmoduletable(L);

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
  //  2: typename
  //  3: _M.types
  //  4: func "setfield"
  //  5: _M

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
int luacwrap_registertype( lua_State*       L
                         , int              nsidx
                         , luacwrap_Type*   desc)
{
  int nsabs = abs_index(L, nsidx);

  LUASTACK_SET(L);

  // get module table from registry
  getmoduletable(L);

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
static const char* luacwrap_storestring(lua_State* L, int idx, const char* errmsg, int errint)
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
    getmoduletable(L);
    lua_getfield(L, -1, g_keyStringTable);
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
static int luacwrap_create_dyntype( lua_State*       L
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
static int luacwrap_registerbuffer(lua_State*       L)
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
static int luacwrap_registerstruct( lua_State*       L)
{
  luacwrap_RecordType* recdesc;
  luacwrap_RecordMember* member;
  const char* name;
  int recsize;
  int nmembers;
  int allocsize;
  int idx;

  // get parameters
  name = luacwrap_storestring(L, 1, "non empty string expected on parameter #%d", 1);
  recsize = lua_tointeger(L, 2);
  luaL_checktype(L, 3, LUA_TTABLE);

  // get number of members
#if (LUA_VERSION_NUM > 501)
  nmembers  = lua_rawlen(L, 3);
#else
  nmembers  = lua_objlen(L, 3);
#endif

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

  recdesc->hdr.typeclass = LUACWRAP_TC_RECORD;
  recdesc->hdr.name = name;
  recdesc->size = recsize;
  recdesc->members = member;

  // fill members table
  idx = 1;
  while (idx <= nmembers)
  {
    int memberoffset;
    const char* membername;
    const char* membertypename;

    lua_rawgeti(L, 3, idx);
    lua_rawgeti(L, -1, 1);
    membername = luacwrap_storestring(L, abs_index(L, -1), "non empty string expected for member name on index #%d", idx);
    lua_rawgeti(L, -2, 2);
    memberoffset = lua_tointeger(L, -1);

    lua_rawgeti(L, -3, 3);
    membertypename = luacwrap_storestring(L, abs_index(L, -1), "non empty string expected for member type name on index #%d", idx);

    member->membername = membername;
    member->memberoffset = memberoffset;

    member->membertypename = membertypename;
    member->membertypedesc = NULL;               // may cache descriptor later

    lua_pop(L, 4);
    ++member;
    ++idx;
  }

  // clear last entry
  member->membername = NULL;
  member->memberoffset = 0;
  member->membertypename = NULL;

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
static int luacwrap_registerarray( lua_State*       L)
{
  luacwrap_ArrayType* arrdesc;
  const char* name;
  int elemcount;
  const char* elemtypename;
  luacwrap_Type* elemtype;
  size_t len;

  // get parameters
  name = lua_tolstring(L, 1, &len);
  if (0 == len)
  {
    luaL_error(L, "non empty string expected on parameter #1");
  }
  elemcount = lua_tointeger(L, 2);
  elemtypename = lua_tolstring(L, 3, &len);
  if (0 == len)
  {
    luaL_error(L, "non empty string expected on parameter #3");
  }

  elemtype = luacwrap_getdescriptor_byname(L, elemtypename, -1);
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
  arrdesc->elemtypename = elemtype->name;
  arrdesc->elemtypedesc = elemtype;

  return luacwrap_create_dyntype(L, &arrdesc->hdr);
}

//////////////////////////////////////////////////////////////////////////
/**

  Creates a buffer with the given size

  @param[in]  L             lua state

  Parameters on lua stack:
    - size (16)

*/////////////////////////////////////////////////////////////////////////
static int luacwrap_createbuffer(lua_State*       L)
{
  int bufsize;

  LUASTACK_SET(L);

  bufsize = lua_tointeger(L, 1);

  // get/create buffer type indexed by size under _M.$buftypes
  getmoduletable(L);
  lua_getfield(L, -1, g_keyBufTypes);
  assert( lua_istable(L, -1));

  // drop module table
  lua_remove(L, -2);

  // lookup buffer type descriptor by buffer size
  lua_pushinteger(L, bufsize);
  lua_rawget(L, -2);
  if (lua_isnil(L, -1))
  {
    // pop nil
    lua_pop(L, 1);

    // create new buffer type
    lua_pushcfunction(L, luacwrap_registerbuffer);
    lua_pushfstring(L, "$buf%d", bufsize);
    lua_pushinteger(L, bufsize);
    lua_call(L, 2, 1);

    // store in cache
    lua_pushvalue(L, -1);
    lua_rawseti(L, -3, bufsize);
  }
  // _M.$buftypes table
  lua_remove(L, -2);

  // buffer type descriptor is on top
  // create a new buffer instance by calling new()
  luaL_getmetafield(L, -1, "new");
  lua_pushvalue(L, -2);
  lua_call(L, 1, 1);

  // drop buffer type descriptor
  lua_remove(L, -2);

  LUASTACK_CLEAN(L, 1);
  return 1;
}

char* create_moduletable =
"  local _M = { types = {} }\n"
"  function _M.tabletostring(self)\n"
"    local res = { \" = {\" }\n"
"    for k = 1, #self do\n"
"      res[#res+1] = \"  \" .. k .. \" = \" .. tostring(self[k]) .. \",\"\n"
"    end\n"
"    res[#res+1] = \"}\"\n"
"    return table.concat(res, \"\\n\")\n"
"  end\n"
"  function _M.getfield(t, f)\n"
"    local v = t\n"
"    for w in string.gmatch(f, \"[^%.]+\") do\n"
"      v = v[w]\n"
"    end\n"
"    return v\n"
"  end\n"
"  function _M.setfield(t, f, v)\n"
"    for w, d in string.gmatch(f, \"([^%.]+)(.?)\") do\n"
"      if d == \".\" then\n"            // not last field?
"        t[w] = t[w] or {}\n"           // create table if absent
"        t = t[w]\n"                    // get the table
"      else\n"
"        t[w] = v\n"                    // assign last field
"      end\n"
"    end\n"
"  end\n"
"  return _M\n";

static luacwrap_cinterface g_cinterface = 
{
  LUACWARP_CINTERFACE_VERSION,
  luacwrap_registerbasictype,
  luacwrap_registertype,
  luacwrap_checktype,
  luacwrap_pushtypedptr,
  luacwrap_pushboxedobj,
  luacwrap_createreference,
  luacwrap_pushreference,
  luacwrap_defuintconstants,

  // v2
  luacwrap_getdescriptor,
  luacwrap_getdescriptor_byname,

  luacwrap_setenvironment,
  luacwrap_getenvironment,

  luacwrap_mobj_get_reference,
  luacwrap_mobj_set_reference,
  luacwrap_mobj_remove_reference,
  luacwrap_mobj_copy_references,

  luacwrap_mobj_getbaseptr,
};
  
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
  if (luaL_dostring(L, create_moduletable))
  {
    lua_error(L);
    return 0;
  }
  else
  {
    // add createstruct() and createarray() to module table
    lua_pushcfunction(L, luacwrap_registerbuffer);
    lua_setfield(L, -2, "registerbuffer");
    lua_pushcfunction(L, luacwrap_registerstruct);
    lua_setfield(L, -2, "registerstruct");
    lua_pushcfunction(L, luacwrap_registerarray);
    lua_setfield(L, -2, "registerarray");
    lua_pushcfunction(L, luacwrap_createbuffer);
    lua_setfield(L, -2, "createbuffer");
    lua_pushcfunction(L, luacwrap_release_reference);
    lua_setfield(L, -2, "releasereference");

    // add reftable and string table to module table
    lua_newtable(L);
    lua_setfield(L, -2, g_keyRefTable);
    lua_newtable(L);
    lua_setfield(L, -2, g_keyStringTable);

    // add _M.$buftypes to cache buffer type descriptors
    lua_newtable(L);
    lua_setfield(L, -2, g_keyBufTypes);

    // store module table in registry
    lua_pushlightuserdata(L, (void*)&g_keyLibraryTable);
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_REGISTRYINDEX);

    // create metatable for reference objects and store it in registry
    lua_pushlightuserdata(L, g_mtReferences);
    lua_newtable(L);
#if (LUA_VERSION_NUM > 501)
    luaL_setfuncs(L, g_mtReferences, 0);
#else
    luaL_openlib(L, NULL, g_mtReferences, 0);
#endif
    lua_rawset(L, LUA_REGISTRYINDEX);

    // create metatable for boxed objects and store it in registry
    lua_pushlightuserdata(L, g_mtBoxed);
    lua_newtable(L);
#if (LUA_VERSION_NUM > 501)
    luaL_setfuncs(L, g_mtBoxed, 0);
#else
    luaL_openlib(L, NULL, g_mtBoxed, 0);
#endif

    // register getouter in metatable
    lua_pushlightuserdata(L, Boxed_getouter);
    lua_setfield(L, -2, g_keyGetOuter);
    lua_rawset(L, LUA_REGISTRYINDEX);

    // create metatable for embedded objects and store it in registry
    lua_pushlightuserdata(L, g_mtEmbedded);
    lua_newtable(L);
#if (LUA_VERSION_NUM > 501)
    luaL_setfuncs(L, g_mtEmbedded, 0);
#else
    luaL_openlib(L, NULL, g_mtEmbedded, 0);
#endif

    // register getouter in metatable
    lua_pushlightuserdata(L, Embedded_getouter);
    lua_setfield(L, -2, g_keyGetOuter);
    lua_rawset(L, LUA_REGISTRYINDEX);

    // create metatable for type wrappers and store it in registry
    lua_pushlightuserdata(L, g_mtTypeCtors);
    lua_newtable(L);
#if (LUA_VERSION_NUM > 501)
    luaL_setfuncs(L, g_mtTypeCtors, 0);
#else
    luaL_openlib(L, NULL, g_mtTypeCtors, 0);
#endif
    
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    lua_rawset(L, LUA_REGISTRYINDEX);

    // create metatable for dynamically created type wrappers
    // and store it in registry
    lua_pushlightuserdata(L, g_mtDynTypeCtors);
    lua_newtable(L);
#if (LUA_VERSION_NUM > 501)
    luaL_setfuncs(L, g_mtDynTypeCtors, 0);
#else
    luaL_openlib(L, NULL, g_mtDynTypeCtors, 0);
#endif
    
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    lua_rawset(L, LUA_REGISTRYINDEX);

    // now register numeric basicTypes
    luacwrap_registerNumericTypes(L);

    // register pointer type
    luacwrap_registerbasictype(L, &regType_Pointer);

    // register reference type
    luacwrap_registerbasictype(L, &regType_Reference);
    
    // register c interface
    lua_pushlightuserdata(L, &g_cinterface);
    lua_setfield(L, -2, LUACWARP_CINTERFACE_NAME);

    // set info fields
    lua_pushstring(L, "Klaus Oberhofer");
    lua_setfield(L, -2, "_AUTHOR");

    lua_pushstring(L, "2.0.0-1");
    lua_setfield(L, -2, "_VERSION");

    lua_pushstring(L, "MIT license: See LICENSE for details.");
    lua_setfield(L, -2, "_LICENSE");

    lua_pushstring(L, "https://github.com/oberhofer/luacwrap");
    lua_setfield(L, -2, "_URL");
  }

  LUASTACK_CLEAN(L, 1);
  return 1;
}


