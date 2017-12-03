//////////////////////////////////////////////////////////////////////////
/**

  Wraps numeric members (such as (U)INT(8,16,32) ) within 
  wrapped structs and implements set/get methods for them

*/////////////////////////////////////////////////////////////////////////

#include "luaaux.h"
#include "wrapnumeric.h"

#define WRAPPER(PREFIX, TYPE, NAME)                                                           \
                                                                                              \
static int PREFIX ## Wrapper_set(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)   \
{                                                                                             \
  TYPE* v = (TYPE*)pData;                                                                     \
  *v = (TYPE)luaL_checknumber(L, -1);                                                         \
                                                                                              \
  return 0;                                                                                   \
}                                                                                             \
                                                                                              \
static int PREFIX ## Wrapper_get(luacwrap_BasicType* self, lua_State *L, PBYTE pData, int offset)   \
{                                                                                             \
  TYPE* v = (TYPE*)pData;                                                                     \
  lua_pushnumber(L, *v);                                                                      \
                                                                                              \
  return 1;                                                                                   \
}                                                                                             \
                                                                                              \
luacwrap_BasicType regType_ ## PREFIX =                                                       \
{                                                                                             \
  {                                                                                           \
    LUACWRAP_TC_BASIC,                                                                        \
    NAME                                                                                      \
  },                                                                                          \
  sizeof(TYPE),                                                                               \
  PREFIX ## Wrapper_get,                                                                      \
  PREFIX ## Wrapper_set                                                                       \
};

WRAPPER(INT8,   INT8,   "$i8")
WRAPPER(UINT8,  UINT8,  "$u8")
WRAPPER(INT16,  INT16,  "$i16")
WRAPPER(UINT16, UINT16, "$u16")
WRAPPER(INT32,  INT32,  "$i32")
WRAPPER(UINT32, UINT32, "$u32")

// platform dependant types
WRAPPER(INT,    int,            "$int")
WRAPPER(UINT,   unsigned int,   "$uint")
WRAPPER(LONG,   long,           "$long")
WRAPPER(ULONG,  unsigned long,  "$ulong")

// floating point types
WRAPPER(FLOAT,  float,  "$flt")
WRAPPER(DOUBLE, double, "$dbl")


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

  // platform dependant types
  luacwrap_registerbasictype(L, &regType_INT);
  luacwrap_registerbasictype(L, &regType_UINT);
  luacwrap_registerbasictype(L, &regType_LONG);
  luacwrap_registerbasictype(L, &regType_ULONG);

  // floating point types
  luacwrap_registerbasictype(L, &regType_FLOAT);
  luacwrap_registerbasictype(L, &regType_DOUBLE);
  
  LUASTACK_CLEAN(L, 0);
  return 0;
}
