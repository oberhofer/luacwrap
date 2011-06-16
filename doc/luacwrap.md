![LuaCwrap logo](logo.png "LuaCwrap logo")

# Introduction

LuaCwrap is a wrapper for C datatypes written in pure C. It utilizes metadata (aka type descriptors)
to describe the layout and names of structures, unions, arrays and buffers.

## Attention

Because LuaCwrap allows to attach wrappers to existing userdata (pointers) it is not safe 
according to the Lua book. Common errors will be detected by LuaCwrap, but as the 'attach' 
method acts like a C cast many things could go wrong.
So if you use 'attach' be sure to know what you are doing.

## Features

LuaCwrap

 * supports struct and union types
 * supports array types
 * supports fixed length buffers
 * supports pointers
 * lua strings and userdata could be assigned to wrapped pointers
 * maintains lifetime of lua objects which had been assigned to wrapped pointers
 * supports C and Lua API

# Usage

## Import LuaCwrap

Importing LuaCwrap is only necessary if you want to declare your own types via the Lua-API.
In this case use the followig line:

    luacwrap = import("luacwrap")

C modules which want to provide wrappers usually create their own namespace table to register their types. 
For an example see the source code of the `testluacwrap` module, which is provided with this package 
for unittesting.

## Registering type descriptors

A valid registered type desriptor is necessary to create data type instances. 
In LuaCwrap you can register type descriptors via a Lua or a C API.

### Lua-API

#### Register array types

    type = luacwrap.registerarraytype(name, size, elemtype)

#### Register record types

    type = luacwrap.registerrecordtype(name, members)

#### Register buffer types

    type, name = luacwrap.registerbuffertype(name, size)

    
### C-API

#### Register array types

    // describe array type
    luacwrap_ArrayType regType_INT32_4 =
    {
      LUACWRAP_TC_ARRAY,
      "INT32_4",
      4,
      sizeof(INT32),
      "$i32"
    };

    luacwrap_registertype(L, LUA_GLOBALSINDEX, &regType_INT32_4.hdr);

#### Register record types

    // member descriptor for INNERSTRUCT
    static luacwrap_RecordMember s_memberINNERSTRUCT[] =
    {
      { "pszText",  offsetof(INNERSTRUCT, pszText),  "$ptr" },
      { NULL, 0 }
    };

    // type descriptor for INNERSTRUCT
    LUACWRAP_DEFINESTRUCT(LIBRARYNAME, INNERSTRUCT)

    // register type within globals table
    luacwrap_registertype(L, LUA_GLOBALSINDEX, &regType_INNERSTRUCT.hdr);

#### Register buffer types

    luacwrap_BasicType regType_Buf32 =
    {
      LUACWRAP_TC_BUFFER,
      "$buf32",
      32
    };

    luacwrap_registerbasictype(L, &regType_Buf32);

#### Register basic types

For these types you have to specify appropriate get and set callbacks which handles
marshalling. Therefore registering basic types is only possible via the C API.
Use the `luacwrap_registerbasictype` function and see the source of LuaCwrap
for usage examples.
    
## Create/Attach instances

### Lua-API

Every registered type descriptor has two methods `new` and `attach` which could be used to
create new instances or create a wrapper around existing userdata.

### C-API


# Internals

## Module table _M

LuaCwrap creates a single module table, where all module global data is stored.
The module table contains:

  * helper functions (e.g. tabletostring, getfield, setfield)
  * register functions (registerbuffer, registerarray, registerstruct)
  * buffer creation function (createbuffer)
  * types table (_M.types)
  
## Type descriptor table _M.types

The type descriptor table spans the namespace for all registered types. Type names containing dots are
stored in the corresponding subtables.
The type table contains predefined types which names start with the '$' character. 
These are

  * $i8 , $u8  (signed/unsigned char)
  * $i16, $u16 (signed/unsigned short)
  * $i32, $u32 (signed/unsigned long)
  * $flt, $dbl (float, double)
  * $ptr       (pointer types)
 
Buffers are registered within the type table, too. The name of buffer types is derived
from the buffer length ($buf*, where * denotes the buffer length).
If a buffer with the requested size is already registered, the existing one is returned.

# License

LuaCwrap is licensed under the terms of the MIT license reproduced below.
This means that LuaCwrap is free software and can be used for both academic
and commercial purposes at absolutely no cost.

Copyright (C) 2011 Klaus Oberhofer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

