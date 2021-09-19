//////////////////////////////////////////////////////////////////////////
//
// LuaCwrap - Lua <-> C 
// Copyright (C) 2011-2021 Klaus Oberhofer. See Copyright Notice in luacwrap.h
//
//////////////////////////////////////////////////////////////////////////
/**

  C-API

*/////////////////////////////////////////////////////////////////////////

#pragma once

#include "luacwrap.h"

// convert a stack index to an absolute (positive) index
#define abs_index(L, i)   ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : \
                          lua_gettop(L) + (i) + 1)

// _M.$buftypes to store references
extern const char* g_keyRefTable;


//
// access global module table
//
void getmoduletable(lua_State *L);

//
// used to register a basic type descriptor in the basic type table
//
int luacwrap_registerbasictype  ( lua_State*          L
                                , luacwrap_BasicType* desc);

//
// register a type descriptor in the given (namespace) table
//
int luacwrap_registertype       ( lua_State*            L
                                , int                   nsidx
                                , luacwrap_Type*        desc);

//
// check a userdata type descriptor against a given type descriptor
//
void* luacwrap_checktype        ( lua_State*            L
                                , int                   ud
                                , luacwrap_Type*        desc);

//
// create a boxed object on the top of the Lua stack, use initval to fill memory
//
void* luacwrap_pushboxedobj     ( lua_State*            L
                                , luacwrap_Type*        desc
                                , int                   initval);

//
// push a pointer as a typed light (means not garbage collected) object 
//
int luacwrap_pushtypedptr       ( lua_State*            L
                                , luacwrap_Type*        desc
                                , void*                 pObj);

//
// access to managed object reference table
//
int luacwrap_mobj_set_reference     (lua_State *L, int ud, int value, int offset);
int luacwrap_mobj_get_reference     (lua_State *L, int ud, int offset);
int luacwrap_mobj_remove_reference  (lua_State *L, int ud, int offset);
int luacwrap_mobj_copy_references   (lua_State* L, int destoffset, int srcoffset, size_t  size);

//
// access to global reference table
//
int luacwrap_createreference    (lua_State* L, int index);
int luacwrap_pushreference      (lua_State* L, int tag);
int luacwrap_release_reference  (lua_State *L);

//
//  Registers a list of global constants maintained in an array or
//  luacwrap_DefUIntConst structs. 
//
void luacwrap_defuintconstants  ( lua_State*              L
                                , luacwrap_DefUIntConst*  constants);


