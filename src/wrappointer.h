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

#pragma once

#include "luacwrap_int.h"


// extern int luacwrap_registerPointerTypes(lua_State* L); 
extern luacwrap_BasicType regType_Pointer;
