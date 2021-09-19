//////////////////////////////////////////////////////////////////////////
//
// LuaCwrap - Lua <-> C 
// Copyright (C) 2011-2021 Klaus Oberhofer. See Copyright Notice in luacwrap.h
//
//  LuaCwrap is licensed under the terms of the MIT license reproduced below.
//  This means that LuaCwrap is free software and can be used for both academic
//  and commercial purposes at absolutely no cost.
//
//  Copyright (C) 2011-2021 Klaus Oberhofer
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////
/**

  Type descriptor

    contains
      - type class (basic, struct/union, array, buffer)
      - for basic types
          size in bytes
          getter
          setter
      - for record (struct/union) types
          size in bytes
          array of member descriptors
      - for array types
          number of elements
          element type
      - for buffer type
          size in bytes

      +-----------------+<------------------------------- userdata
      | TypeDescriptor  |                                   metatable
      |                 |                                     [new]         -> Type_new
      +-----------------+                                     [attach]      -> Type_attach

    Type:new
        - creates udata with size of complex type
        - attaches boxed metatable
        - adds type descriptor to environment[$desc]

    Type:attach(to lightuserdata)
        - creates embedded object
        - .outer points to lightuserdata
        - .offset = 0

  Boxed object

      userdata
      +-----------------+<------------------- userdata
      |                 |                       metatable
      |                 |                         [__index]     -> Boxed_index
      +-----------------+                         [__newindex]  -> Boxed_newindex
      | Embedded object |                         [__tostring]  -> Boxed_tostring
      |                 |                         [__len]       -> Boxed_len
      +-----------------+                       environment
      |                 |                         [$desc]       -> typedescriptor
      |                 |
      +-----------------+

  Embedded object

      userdata
      +-----------------+<----,     +--------------+<---- userdata
      |                 |     '-----| outer        |        metatable
      |                 |           +--------------+          [__index]     -> Embedded_index
      +-----------------+<----------| offset       |          [__newindex]  -> Embedded_newindex
      | Embedded object |           +--------------+          [__tostring]  -> Embedded_tostring
      |                 |                                     [__len]       -> Embedded_len
      +-----------------+                                   environment
      |                 |                                     [$desc]       -> typedescriptor
      |                 |
      +-----------------+


*/////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#ifdef __cplusplus
}
#endif

typedef unsigned char   BYTE;
typedef BYTE*           PBYTE;

typedef signed char     INT8;
typedef unsigned char   UINT8;

typedef signed short    INT16;
typedef unsigned short  UINT16;

typedef signed int      INT32;
typedef unsigned int    UINT32;

//
// type classes
//
#define LUACWRAP_TC_BASIC       0
#define LUACWRAP_TC_RECORD      1
#define LUACWRAP_TC_ARRAY       2
#define LUACWRAP_TC_BUFFER      3

//
// type descriptor header
//
struct luacwrap_Type
{
  unsigned int              typeclass;  // type class
  const char*               name;       // name of type
};

//
// type descriptor for basic types
//
struct luacwrap_BasicType
{
  struct luacwrap_Type      hdr;
  unsigned int              size;       // size of type

  int (*getWrapper )(struct luacwrap_BasicType* self, lua_State* L, PBYTE pData, int offset);            // get wrapper
  int (*setWrapper )(struct luacwrap_BasicType* self, lua_State* L, PBYTE pData, int offset);            // set wrapper
};

//
// type descriptor for record types (struct/union)
//
struct luacwrap_ArrayType
{
  struct luacwrap_Type      hdr;
  unsigned int              elemcount;      // size of array 
  unsigned int              elemsize;       // size of one element
  const char*               elemtypename;   // elemnt type name
  struct luacwrap_Type*     elemtypedesc;   // caches type descriptor 
};

//
// member descriptor to describe record members
// (use arrays terminated with the last entry member name == NULL)
//
struct luacwrap_RecordMember
{
  const char*               membername;     // name to access
  unsigned int              memberoffset;   // offset within struct
  const char*               membertypename; // member type name
  struct luacwrap_Type*     membertypedesc; // caches cache type descriptor 
};

//
// type descriptor for array types
//
struct luacwrap_RecordType
{
  struct luacwrap_Type      hdr;
  unsigned int              size;             // size of type
  struct luacwrap_RecordMember*    members;   // array of member descriptors
};

//
// type descriptor for buffer types
//
struct luacwrap_BufferType
{
  struct luacwrap_Type      hdr;
  unsigned int              size;       // size of type
};


//
// reference to a managed or embedded object
//
struct luacwrap_EmbeddedObject
{
  unsigned int  outer;      // reference to outer complex type object
                            //  (if != LUA_REFNIL)
  unsigned int  offset;     // offset within outer complex type object
};

//
// typedef names for the above structs
//
typedef struct luacwrap_Type            luacwrap_Type;
typedef struct luacwrap_BasicType       luacwrap_BasicType;
typedef struct luacwrap_RecordMember    luacwrap_RecordMember;
typedef struct luacwrap_RecordType      luacwrap_RecordType;
typedef struct luacwrap_ArrayType       luacwrap_ArrayType;
typedef struct luacwrap_BufferType      luacwrap_BufferType;
typedef struct luacwrap_EmbeddedObject  luacwrap_EmbeddedObject;

//////////////////////////////////////////////////////////////////////////
/**

  LUACWRAP_DEFINESTRUCT

  helper macr to create struct descriptors

*/////////////////////////////////////////////////////////////////////////
#define LUACWRAP_DEFINESTRUCT(name)                     \
luacwrap_RecordType regType_##name =                    \
{                                                       \
  {                                                     \
    LUACWRAP_TC_RECORD,                                 \
    #name                                               \
  },                                                    \
  sizeof(name),                                         \
  s_member##name                                        \
};

//////////////////////////////////////////////////////////////////////////
/**

  LUACWRAP_DEFINEARRAY

  helper macro to create array descriptors

*/////////////////////////////////////////////////////////////////////////
#define LUACWRAP_DEFINEARRAY(elemtype, nelems)          \
luacwrap_ArrayType regType_##elemtype##_##nelems =      \
{                                                       \
  {                                                     \
    LUACWRAP_TC_ARRAY,                                  \
    #elemtype"_"#nelems                                 \
  },                                                    \
  nelems,                                               \
  sizeof(elemtype),                                     \
  "$" #elemtype                                         \
};

//////////////////////////////////////////////////////////////////////////
/**
    
    Used to register unsigned int constants
    
    usage:

      const luacwrap_DefUIntConst s_MyConstants[] = 
      {
        LUACWRAP_DEFUINTCONSTANT(MY_PREDEFINED_CONSTANT_A)
        LUACWRAP_DEFUINTCONSTANT(MY_PREDEFINED_CONSTANT_B)
        ...
        {0, 0}
      };

*/////////////////////////////////////////////////////////////////////////

typedef struct luacwrap_DefUIntConst
{
  const char*  name;
  unsigned int value;
} luacwrap_DefUIntConst;

#define LUACWRAP_DEFUINTCONSTANT(id) { #id, id },

#ifndef LUACWRAP_API
#define LUACWRAP_API extern
#endif

//////////////////////////////////////////////////////////////////////////
/**
    
    LuaCWrap interface for C modules
    
    usage:

      luacwrap_cinterface*  iface;
    
      // load luacwrap
      // luacwrap = require("luacwrap")
      lua_getglobal(L, "require");
      lua_pushstring(L, "luacwrap");
      lua_call(L, 1, 1);
      
      // get c interface
      lua_getfield(L, "LUACWARP_CINTERFACE_NAME");
      iface = (luacwrap_cinterface*)lua_touserdata(L, -1);
      
      // check interface version
      if (LUACWARP_CINTERFACE_VERSION != iface->version)
      {
        luaL_error(L, "Could not load luacwrap. Incompatible C interface version. Expected %d got %d.", LUACWARP_CINTERFACE_VERSION, iface->version);
      }
      
      // drop package table
      lua_pop(L, 1);
      
      // call function
      iface->registerbasictype(L, &desc);

*/////////////////////////////////////////////////////////////////////////


//
// used to register a basic type descriptor in the basic type table
//
typedef int (*luacwrap_registerbasictype_t  )( lua_State*          L
                                             , luacwrap_BasicType* desc);

//
// register a type descriptor in the given (namespace) table
//
typedef int (*luacwrap_registertype_t       )( lua_State*            L
                                             , int                   nsidx
                                             , luacwrap_Type*        desc);

//
// check a userdata type descriptor against a given type descriptor
//
typedef void* (*luacwrap_checktype_t        )( lua_State*            L
                                             , int                   ud
                                             , luacwrap_Type*        desc);

//
// create a boxed object on the top of the Lua stack, use initval to fill memory
//
typedef void* (*luacwrap_pushboxedobj_t     )( lua_State*            L
                                             , luacwrap_Type*        desc
                                             , int                   initval);

//
// push a pointer as a typed light (means not garbage collected) object 
//
typedef int (*luacwrap_pushtypedptr_t       )( lua_State*            L
                                             , luacwrap_Type*        desc
                                             , void*                 pObj);

//
// access to reference table
//
typedef int (*luacwrap_createreference_t    )(lua_State* L, int index);
typedef int (*luacwrap_pushreference_t      )(lua_State* L, int tag);

//
//  Registers a list of global constants maintained in an array or
//  luacwrap_DefUIntConst structs. 
//
typedef void (*luacwrap_defuintconstants_t  )( lua_State*              L
                                             , luacwrap_DefUIntConst*  constants);

//
// access to type descriptor
//
typedef luacwrap_Type* (*luacwrap_getdescriptor_t         )(lua_State* L, int ud);
typedef luacwrap_Type* (*luacwrap_getdescriptor_byname_t  )(lua_State* L, const char* name, int namelen);

//
// access to managed object environment table
//
typedef int (*luacwrap_mobj_setenvironment_t    )(lua_State *L, int ud);
typedef int (*luacwrap_mobj_getenvironment_t    )(lua_State *L, int ud);
typedef int (*luacwrap_mobj_get_reference_t     )(lua_State *L, int ud, int offset);
typedef int (*luacwrap_mobj_set_reference_t     )(lua_State *L, int ud, int value, int offset);
typedef int (*luacwrap_mobj_remove_reference_t  )(lua_State *L, int ud, int offset);
typedef int (*luacwrap_mobj_copy_references_t   )(lua_State* L);


#define LUACWARP_CINTERFACE_VERSION  2

#define LUACWARP_CINTERFACE_NAME     "c_interface"

typedef struct 
{
  int version;
  luacwrap_registerbasictype_t  registerbasictype;
  luacwrap_registertype_t       registertype;
  luacwrap_checktype_t          checktype;
  luacwrap_pushtypedptr_t       pushtypedptr;
  luacwrap_pushboxedobj_t       pushboxedobj;
  luacwrap_createreference_t    createreference;
  luacwrap_pushreference_t      pushreference;
  luacwrap_defuintconstants_t   defuintconstants;
} luacwrap_cinterface_v1;

typedef struct
{
  int version;
  luacwrap_registerbasictype_t      registerbasictype;
  luacwrap_registertype_t           registertype;
  luacwrap_checktype_t              checktype;
  luacwrap_pushtypedptr_t           pushtypedptr;
  luacwrap_pushboxedobj_t           pushboxedobj;
  luacwrap_createreference_t        createreference;
  luacwrap_pushreference_t          pushreference;
  luacwrap_defuintconstants_t       defuintconstants;
  // v2
  luacwrap_getdescriptor_t          getdescriptor;
  luacwrap_getdescriptor_byname_t   getdescriptorbyname;

  luacwrap_mobj_setenvironment_t    mobjsetenvironment;
  luacwrap_mobj_getenvironment_t    mobjgetenvironment;

  luacwrap_mobj_get_reference_t     mobjgetreference;
  luacwrap_mobj_set_reference_t     mobjsetreference;
  luacwrap_mobj_remove_reference_t  mobjremovereference;
  luacwrap_mobj_copy_references_t   mobjcopyreferences;
} luacwrap_cinterface;

