//////////////////////////////////////////////////////////////////////////
// LuaCwrap - lua <-> C 
// Copyright (C) 2011 Klaus Oberhofer. See Copyright Notice in luacwrap.h
//
//////////////////////////////////////////////////////////////////////////
/**

  Test module for luacwrap

  Registers a test structure definition which is needed by test.lua

*/////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>

#include "luaaux.h"
#include "luacwrap.h"


#define LIBRARYNAME "testluacwrap"

typedef struct 
{
  char* pszText;
} INNERSTRUCT;

typedef struct 
{
  UINT8    u8;
  INT8     i8;
  UINT16   u16;
  INT16    i16;
  UINT32   u32;
  INT32    i32;
  char*    ptr;
  int      ref;
  char     chararray[32];
  UINT32   intarray[4];
  INNERSTRUCT inner;
} TESTSTRUCT;

//////////////////////////////////////////////////////////////////////////
/**

  metadata necessary to describe TESTSTRUCT for LuaCWrap

*/////////////////////////////////////////////////////////////////////////

luacwrap_BasicType regType_Buf32 =
{
  {
    LUACWRAP_TC_BUFFER,
    "$buf32"
  },
  32
};

// member descriptor for INNERSTRUCT
static luacwrap_RecordMember s_memberINNERSTRUCT[] =
{
  { "pszText",  offsetof(INNERSTRUCT, pszText),  "$ptr" },
  { NULL, 0 }
};

// type descriptor for INNERSTRUCT
LUACWRAP_DEFINESTRUCT(LIBRARYNAME, INNERSTRUCT)


// member descriptor for TESTSTRUCT
static luacwrap_RecordMember s_memberTESTSTRUCT[] =
{
  { "u8",   offsetof(TESTSTRUCT, u8),   "$u8"   },
  { "i8",   offsetof(TESTSTRUCT, i8),   "$i8"   },
  { "u16",  offsetof(TESTSTRUCT, u16),  "$u16"  },
  { "i16",  offsetof(TESTSTRUCT, i16),  "$i16"  },
  { "u32",  offsetof(TESTSTRUCT, u32),  "$u32"  },
  { "i32",  offsetof(TESTSTRUCT, i32),  "$i32"  },
  { "ptr",  offsetof(TESTSTRUCT, ptr),  "$ptr"  },
  { "ref",  offsetof(TESTSTRUCT, ref),  "$ref"  },
  { "chararray",  offsetof(TESTSTRUCT, chararray),  "$buf32" },
  { "intarray",  offsetof(TESTSTRUCT, intarray),  "INT32_4"  },
  { "inner",  offsetof(TESTSTRUCT, inner),  "INNERSTRUCT"    },
  { NULL, 0 }
};

// type descriptor for TESTSTRUCT
LUACWRAP_DEFINESTRUCT(LIBRARYNAME, TESTSTRUCT)

// describe array type, gets array name "regType_INT32_4"
// and type name "INT32_4"
// LUACWRAP_DEFINEARRAY(LIBRARYNAME, INT32, 4)

// same as 
luacwrap_ArrayType regType_INT32_4 =
{
  {
    LUACWRAP_TC_ARRAY,
    "INT32_4"
  },
  4,
  sizeof(INT32),
  "$i32"
};


//////////////////////////////////////////////////////////////////////////
/**

  function to print a TESTSTRUCT into a string

  @param[in]  L  pointer lua state
  
  @result returns lua string with TESTSTRUCT dump

*/////////////////////////////////////////////////////////////////////////
int printTESTSTRUCT(lua_State* L)
{
  TESTSTRUCT* ud;
  char szTemp[2048];

  LUASTACK_SET(L);

  ud = (TESTSTRUCT*)luacwrap_checktype(L, 1, &regType_TESTSTRUCT.hdr);
  
  sprintf(szTemp, "TESTSTRUCT %p\n{\nu8:%i,\ni8:%i,\nu16:%i,\ni16:%i,\nu32:%i,\ni32:%i,\nptr:%p (%s),\nref:%i,\ninner.pszText:%p (%s)\n}\n", 
    ud,
    ud->u8,
    ud->i8,
    ud->u16,
    ud->i16,
    ud->u32,
    ud->i32,
    ud->ptr,
    "",
    ud->ref, 
    ud->inner.pszText,
    ud->inner.pszText ? ud->inner.pszText : ""
 );

  lua_pushstring(L, szTemp);

  LUASTACK_CLEAN(L, 1);
  return 1;
}

//////////////////////////////////////////////////////////////////////////
/**

  function which pushes a pointer to TESTSTRUCT as light user data 
  (to test attach method)

  @param[in]  L  pointer lua state
  
  @result always 0

*/////////////////////////////////////////////////////////////////////////
int callwithTESTSTRUCT(lua_State* L)
{
  TESTSTRUCT ud = { 0 };
  
  LUASTACK_SET(L);
  
  ud.u8  =   8;
  ud.i8  =  -8;
  ud.u16 =  16;
  ud.i16 = -16;
  ud.u32 =  32;
  ud.i32 = -32;
  ud.ptr =  "a ptr";

  // expects a function as parameter
  if (lua_isfunction(L, -1))
  {
    lua_pushvalue(L, -1);
    lua_pushlightuserdata(L, &ud);
    
    lua_call(L, 1, 0);
  }
  
  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  function which pushes a pointer to TESTSTRUCT as a boxed object

  @param[in]  L  pointer lua state
  
  @result always 0

*/////////////////////////////////////////////////////////////////////////
int callwithBoxedTESTSTRUCT(lua_State* L)
{
  LUASTACK_SET(L);

  // expects a function as parameter
  if (lua_isfunction(L, -1))
  {
    TESTSTRUCT* ud;
  
    lua_pushvalue(L, -1);

    ud = (TESTSTRUCT*)luacwrap_pushboxedobj(L, &regType_TESTSTRUCT.hdr, 0);
    
    ud->u8  =   8;
    ud->i8  =  -8;
    ud->u16 =  16;
    ud->i16 = -16;
    ud->u32 =  32;
    ud->i32 = -32;
    ud->ptr =  "a ptr";
    
    lua_call(L, 1, 0);
  }
  
  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  function which pushes a wrapped pointer to TESTSTRUCT
  (same result after using attach method on unwrapped pointer)

  @param[in]  L  pointer lua state
  
  @result wrapped TESTSTRUCT pointer

*/////////////////////////////////////////////////////////////////////////
int callwithwrappedTESTSTRUCT(lua_State* L)
{
  TESTSTRUCT ud = { 0 };
  
  LUASTACK_SET(L);
  
  ud.u8  =   8;
  ud.i8  =  -8;
  ud.u16 =  16;
  ud.i16 = -16;
  ud.u32 =  32;
  ud.i32 = -32;
  // ud.ptr = "a ptr, too";

  // expects a function as parameter
  if (lua_isfunction(L, 1))
  {
    lua_pushvalue(L, 1);
    luacwrap_pushtypedptr(L, &regType_TESTSTRUCT.hdr, &ud);
    lua_call(L, 1, 0);
  }
  
  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  function which pushes a wrapped pointer to TESTSTRUCT
  (same result after using attach method on unwrapped pointer)

  @param[in]  L  pointer lua state
  
  @result wrapped TESTSTRUCT pointer

*/////////////////////////////////////////////////////////////////////////
int callwithRefType(lua_State* L)
{
  TESTSTRUCT ud = { 0 };
  
  LUASTACK_SET(L);
  
  ud.u8  =   8;
  ud.i8  =  -8;
  ud.u16 =  16;
  ud.i16 = -16;
  ud.u32 =  32;
  ud.i32 = -32;
  // ud.ptr = "a ptr, too";

  // add parameter two as reference
  ud.ref = luacwrap_createreference(L, 2);

  // expects a function as parameter
  if (lua_isfunction(L, 1))
  {
    lua_pushvalue(L, 1);
    luacwrap_pushtypedptr(L, &regType_TESTSTRUCT.hdr, &ud);
    lua_call(L, 1, 0);
  }
  
  LUASTACK_CLEAN(L, 0);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
/**

  function if given INNERSTRUCT is part of the given TESTSTRUCT

  @param[in]  L  pointer lua state
  
  @result true or false

*/////////////////////////////////////////////////////////////////////////
int checkInnerStructAccess(lua_State* L)
{
  TESTSTRUCT*  outer;
  INNERSTRUCT* inner;
  
  int result = 0;

  LUASTACK_SET(L);

  outer = (TESTSTRUCT*)luacwrap_checktype(L, 1, &regType_TESTSTRUCT.hdr);
  inner = (INNERSTRUCT*)luacwrap_checktype(L, 2, &regType_INNERSTRUCT.hdr);
    
  printf("outer %p\n", outer);
  printf("inner %p\n", inner);
    
  // check if inner points inside outer
  if ((&outer->inner) == inner)
  {
    result = 1;
  }

  lua_pushnumber(L, result);
  
  LUASTACK_CLEAN(L, 1);
  return 1;
}


static const luaL_reg testluacwrap_functions[ ] = {
  { "printTESTSTRUCT"   , printTESTSTRUCT },
  { "callwithTESTSTRUCT", callwithTESTSTRUCT },
  { "callwithBoxedTESTSTRUCT", callwithBoxedTESTSTRUCT },
  { "callwithwrappedTESTSTRUCT", callwithwrappedTESTSTRUCT },
  { "callwithRefType", callwithRefType },
  { "checkInnerStructAccess", checkInnerStructAccess },
  { NULL, NULL }
};

//////////////////////////////////////////////////////////////////////////
/**

  registers structures as Lua types

  @param[in]  L  pointer lua state

*/////////////////////////////////////////////////////////////////////////
int luaopen_testluacwrap(lua_State *L)
{
  LUASTACK_SET(L);

  // require("luacwrap")
  lua_getglobal(L, "require");
  lua_pushstring(L, "luacwrap");
  lua_call(L, 1, 0);

  // register package functions
  lua_newtable(L);
  luaL_register(L, NULL, testluacwrap_functions);

  luacwrap_registerbasictype(L, &regType_Buf32);

  luacwrap_registertype(L, LUA_GLOBALSINDEX, &regType_INNERSTRUCT.hdr);
  luacwrap_registertype(L, LUA_GLOBALSINDEX, &regType_INT32_4.hdr);
  luacwrap_registertype(L, LUA_GLOBALSINDEX, &regType_TESTSTRUCT.hdr);

  LUASTACK_CLEAN(L, 1);
  return 1;
}
