//////////////////////////////////////////////////////////////////////////
// LuaCwrap - lua <-> C 
// Copyright (C) 2011 Klaus Oberhofer. See Copyright Notice in luacwrap.h
//
//////////////////////////////////////////////////////////////////////////
/**

  Wraps numeric members (such as (U)INT(8,16,32) ) within 
  wrapped structs and implements set/get methods for them

*/////////////////////////////////////////////////////////////////////////

#pragma once

#include "luacwrap_int.h"


extern luacwrap_BasicType regType_INT8;
extern luacwrap_BasicType regType_UINT8;
extern luacwrap_BasicType regType_INT16;
extern luacwrap_BasicType regType_UINT16;
extern luacwrap_BasicType regType_INT32;
extern luacwrap_BasicType regType_UINT32;

extern int luacwrap_registerNumericTypes(lua_State* L);
