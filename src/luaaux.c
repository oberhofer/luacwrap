//////////////////////////////////////////////////////////////////////////
//
// LuaCwrap - Lua <-> C 
// Copyright (C) 2011-2021 Klaus Oberhofer. See Copyright Notice in luacwrap.h
//
//////////////////////////////////////////////////////////////////////////
/**

  Auxiliary helper functions for debugging

  The origin of this code is the LuaCOM project.
  LuaCOM is also under the same license as LuaCwrap (MIT license).

*/////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <wchar.h>

#include "luaaux.h"


#define DBGPRINT wprintf

/*
 * Prints the lua stack
 */
void luaaux_printLuaStack(lua_State *L)
{
  int size = lua_gettop(L);
  int i = 0;

  for(i = size; i > 0; i--)
  {
    switch(lua_type(L,i))
    {
      case LUA_TNUMBER:
        DBGPRINT(L"%d: number = %f", i, lua_tonumber(L, i));
        break;

      case LUA_TSTRING:
        DBGPRINT(L"%d: string = \"%S\"", i, lua_tostring(L, i));
        break;

      case LUA_TTABLE:
        {
          const char * debugtag;
          DBGPRINT(L"%d: table = \"%S\"", i, luaL_typename(L, i));

          // Check if table contains a ["__debug"] field
          // which could be used for debugging purposes.
          // ["__debug"] is either a printable value or 
          // could contain a function which delivers 
          // a string.
          lua_getfield(L, i, "__debug");
          if (lua_isfunction(L, -1))
          {
            int ret = lua_pcall(L, 0, 1, 0);
            if (ret != 0)
            {
              // luauser_report(L, ret);
            }
          }
          debugtag = lua_tostring(L, -1);
          if (NULL == debugtag) 
          {
            DBGPRINT(L"    [%S]", debugtag);
          }
          lua_pop(L, 1);
        }
        break;

      case LUA_TUSERDATA:
        {
          const char* asString = lua_tostring(L, i);
          if (asString)
          {
            DBGPRINT(L"%d: userdata = \"%S\"", i, asString);
          }
          else
          {
            DBGPRINT(L"%d: userdata = [%p]", i, lua_touserdata(L, i));

#if 0
            // Check if userdata contains a ["__debug"] field in the metatable
            // which could be used for debugging purposes.
            // ["__debug"] is either a printable value or 
            // could contain a function which delivers 
            // a string.
            luaL_getmetafield(L, i, "__debug");
            if (lua_isfunction(L, -1))
            {
              int ret;
              lua_pushvalue(L, i);
              ret = lua_pcall(L, 1, 1, 0);
              if (ret != 0)
              {
                // luauser_report(L, ret);
              }
            }
            const char * debugtag;
            debugtag = lua_tostring(L, -1);
            if (NULL == debugtag) 
            {
              DBGPRINT(L"    [%S]", debugtag);
            }
            lua_pop(L, 1);
#endif
          }
        }
        break;

      case LUA_TNIL:
        DBGPRINT(L"%d: nil", i);
        break;

      case LUA_TBOOLEAN:
        if(lua_toboolean(L, i))
          DBGPRINT(L"%d: boolean = true", i);
        else
          DBGPRINT(L"%d: boolean = false", i);

        break;

      case LUA_TFUNCTION:
          DBGPRINT(L"%d: function", i);
        break;

      default:
        DBGPRINT(L"%d: unknown type (%d)", i, lua_type(L, i));
        break;
    }      

    DBGPRINT(L"\n");
  }

  DBGPRINT(L"\n");
}

void luaaux_printPreDump(int expected) {
  DBGPRINT(L"STACK DUMP\n");
  DBGPRINT(L"Expected size: %i\n",expected);
}

void luaaux_printLuaTable(lua_State *L, stkIndex t)
{
  lua_pushnil(L);  /* first key */
  while (lua_next(L, t) != 0) {
   /* `key' is at index -2 and `value' at index -1 */
   DBGPRINT(L"%S - %S\n",
     lua_tostring(L, -2), lua_typename(L, lua_type(L, -1)));
   lua_pop(L, 1);  /* removes `value'; keeps `index' for next iteration */
  }
}

const char* luaaux_makeLuaErrorMessage(int return_value, const char* msg)
{
  static char message[1000];
  message[0] = '\0';

  if(return_value == 0)
    return "No error";
  
  switch(return_value)
  {
    case LUA_ERRRUN:
      {
        strncat(message, 
          "Lua runtime error", 
          sizeof(message) - strlen(message) - 1);

        if(msg)
        {
          strncat(message, ": ", sizeof(message) - strlen(message) - 1);
          strncat(message, msg, sizeof(message) - strlen(message) - 1);
        }
      }

      break;

    case LUA_ERRMEM:
      strcpy(message, "Lua memory allocation error.");
      break;

    case LUA_ERRERR:
      strcpy(message, "Lua error: error during error handler execution.");
      break;

    default:
      strcpy(message, "Unknown Lua error.");
      break;
  }

  return message;
}
