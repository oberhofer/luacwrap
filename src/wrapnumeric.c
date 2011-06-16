//////////////////////////////////////////////////////////////////////////
/**

  Wraps numeric members (such as (U)INT(8,16,32) ) within 
  wrapped structs and implements set/get methods for them

*/////////////////////////////////////////////////////////////////////////

#include "luaaux.h"
#include "wrapnumeric.h"

// implements set method
static int INT8Wrapper_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  INT8* v = (INT8*)pData;
  *v = (INT8)luaL_checknumber(L, -1);

  return 0;
}

// implements get method
static int INT8Wrapper_get(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  INT8* v = (INT8*)pData;
  lua_pushnumber(L, *v);

  return 1;
}

luacwrap_BasicType regType_INT8 =
{
  {
    LUACWRAP_TC_BASIC,
    "$i8"
  },
  sizeof(INT8),
  INT8Wrapper_get,
  INT8Wrapper_set
};

//------------------------------------------------------------------------

// implements set method
static int UINT8Wrapper_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  UINT8* v = (UINT8*)pData;
  *v = (UINT8)luaL_checknumber(L, -1);

  return 0;
}

// implements get method
static int UINT8Wrapper_get(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  UINT8* v = (UINT8*)pData;
  lua_pushnumber(L, *v);

  return 1;
}

luacwrap_BasicType regType_UINT8 =
{
  {
    LUACWRAP_TC_BASIC,
    "$u8"
  },
  sizeof(UINT8),
  UINT8Wrapper_get,
  UINT8Wrapper_set
};

//------------------------------------------------------------------------

// implements set method
static int INT16Wrapper_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  INT16* v = (INT16*)pData;
  *v = (INT16)luaL_checknumber(L, -1);

  return 0;
}

// implements get method
static int INT16Wrapper_get(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  INT16* v = (INT16*)pData;
  lua_pushnumber(L, *v);

  return 1;
}

luacwrap_BasicType regType_INT16 =
{
  {
    LUACWRAP_TC_BASIC,
    "$i16"
  },
  sizeof(INT16),
  INT16Wrapper_get,
  INT16Wrapper_set
};

//------------------------------------------------------------------------

// implements set method
static int UINT16Wrapper_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  UINT16* v = (UINT16*)pData;
  *v = (UINT16)luaL_checknumber(L, -1);

  return 0;
}

// implements get method
static int UINT16Wrapper_get(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  UINT16* v = (UINT16*)pData;
  lua_pushnumber(L, *v);

  return 1;
}

luacwrap_BasicType regType_UINT16 =
{
  {
    LUACWRAP_TC_BASIC,
    "$u16"
  },
  sizeof(UINT16),
  UINT16Wrapper_get,
  UINT16Wrapper_set
};

//------------------------------------------------------------------------

// implements set method
static int INT32Wrapper_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  INT32* v = (INT32*)pData;
  *v = (INT32)luaL_checknumber(L, -1);

  return 0;
}

// implements get method
static int INT32Wrapper_get(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  INT32* v = (INT32*)pData;
  lua_pushnumber(L, *v);

  return 1;
}

luacwrap_BasicType regType_INT32 =
{
  {
    LUACWRAP_TC_BASIC,
    "$i32"
  },
  sizeof(INT32),
  INT32Wrapper_get,
  INT32Wrapper_set
};

//------------------------------------------------------------------------

// implements set method
static int UINT32Wrapper_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  UINT32* v = (UINT32*)pData;
  *v = (UINT32)luaL_checknumber(L, -1);

  return 0;
}

// implements get method
static int UINT32Wrapper_get(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)
{
  // calculate address of struct member 
  UINT32* v = (UINT32*)pData;
  lua_pushnumber(L, *v);

  return 1;
}

luacwrap_BasicType regType_UINT32 =
{
  {
    LUACWRAP_TC_BASIC,
    "$u32"
  },
  sizeof(UINT32),
  UINT32Wrapper_get,
  UINT32Wrapper_set
};


//////////////////////////////////////////////////////////////////////////
/**

  registers numeric basic types

*/////////////////////////////////////////////////////////////////////////
int luacwrap_registerNumericTypes(lua_State* L)
{
  LUASTACK_SET(L);

  luacwrap_registerbasictype(L, &regType_INT8);
  luacwrap_registerbasictype(L, &regType_UINT8);
  luacwrap_registerbasictype(L, &regType_INT16);
  luacwrap_registerbasictype(L, &regType_UINT16);
  luacwrap_registerbasictype(L, &regType_INT32);
  luacwrap_registerbasictype(L, &regType_UINT32);
  
  LUASTACK_CLEAN(L, 0);
  return 0;
}
