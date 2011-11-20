//////////////////////////////////////////////////////////////////////////
/**

  @file   "lua_constdef.cpp"

  @short  Lua utilities

  @author Klaus Oberhofer

  Utility functions to register constants

*/////////////////////////////////////////////////////////////////////////

#include "luacwrap.h"

//////////////////////////////////////////////////////////////////////////
/**

  Registers a list of global constants

  @param[in]  L         lua state
  @param[in]  constants constant table

*/////////////////////////////////////////////////////////////////////////
LUACWRAP_API void luacwrap_defuintconstants(lua_State* L, luacwrap_DefUIntConst* constants)
{
  if (constants)
  {
    while (constants->name)
    {
      lua_pushinteger(L, constants->value);
      lua_setglobal  (L, constants->name);

      ++constants;
    }
  }
}

