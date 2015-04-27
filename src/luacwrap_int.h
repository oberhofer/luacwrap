

#pragma once

#include "luacwrap.h"

#ifdef __cplusplus
extern "C" {
#endif

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
// access to reference table
//
int luacwrap_createreference    (lua_State* L, int index);
int luacwrap_pushreference      (lua_State* L, int tag);

//
//  Registers a list of global constants maintained in an array or
//  luacwrap_DefUIntConst structs. 
//
void luacwrap_defuintconstants  ( lua_State*              L
                                , luacwrap_DefUIntConst*  constants);


#ifdef __cplusplus
}
#endif
