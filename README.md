[![test](https://github.com/oberhofer/luacwrap/workflows/test/badge.svg)](https://github.com/oberhofer/luacwrap?query=workflow%3Atest)
[![Build status](https://ci.appveyor.com/api/projects/status/github/oberhofer/luacwrap?svg=true&branch=master)](https://ci.appveyor.com/project/oberhofer/luacwrap/branch/master)

# Introduction

LuaCwrap is a wrapper for C datatypes written in pure C. It utilizes metadata (aka type descriptors) to describe the layout and names of structures, unions, arrays and buffers.

# Features

 * supports struct and union types
 * supports array types
 * supports fixed length buffers
 * supports pointers
 * lua strings and userdata could be assigned to wrapped pointers
 * maintains lifetime of lua objects which had been assigned to wrapped pointers
 * supports C and Lua API

# Documentation

see luacwrap.html within the doc directory

# License

LuaCwrap is licensed under the terms of the MIT license reproduced below.
This means that LuaCwrap is free software and can be used for both academic
and commercial purposes at absolutely no cost.

Copyright (C) 2011-2021 Klaus Oberhofer

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


