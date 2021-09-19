--////////////////////////////////////////////////////////////////////////
--
-- LuaCwrap - Lua <-> C 
-- Copyright (C) 2011-2021 Klaus Oberhofer. See Copyright Notice in luacwrap.h
--
-- unit tests for LuaCwrap
--
--////////////////////////////////////////////////////////////////////////
lu = require("luaunit")

luacwrap = require("luacwrap")

-- wrapper for the test struct type
testluacwrap = require("testluacwrap")

--
-- helper function to get a table as a string
-- outputs members sorted by member names
-- used for table comparison
--
local function getTable(t)
  local keys = {}
  for k, _ in pairs(t) do
    keys[#keys+1] = k
  end
  table.sort(keys)
  local tmp = {}
  for _, k in ipairs(keys) do
    local v = t[k]
    tmp[#tmp+1] = type(v) .. " " .. k
  end
  return table.concat(tmp, ", ")
end

--
-- test suite
--
TestTESTSTRUCT = {}

--
-- test different struct creation methods
--
function TestTESTSTRUCT:testCreateTESTSTRUCT()

    -- create a ne struct instance
    local struct = TESTSTRUCT:new()
    assert(nil ~= struct)

    -- semantic of attach()
    local attached = TESTSTRUCT:attach(struct)
    assertEquals(attached, struct)
    assertEquals(struct.__ptr, attached.__ptr)

    -- check metatable
    assert(nil ~= getmetatable(struct))
    assert(getTable(getmetatable(struct)) == [[function __index, function __len, function __newindex, function __tostring, userdata getouter]])

    -- check inner struct access
    assert(nil ~= getmetatable(struct.inner))
    assert(getTable(getmetatable(struct.inner)) == [[function __gc, function __index, function __len, function __newindex, function __tostring, userdata getouter]])
    assert(1 == testluacwrap.checkInnerStructAccess(struct, struct.inner))

    -- check $ref init values
    -- by default references should have the value nil
    assert(nil == struct.ref.value)

    -- and the reserved index 0
    assert(0 == struct.ref.ref)

    -- access __ptr
    assert("userdata" == type(struct.__ptr))
    assert("userdata" == type(struct.intarray.__ptr))
    assert("userdata" == type(struct.inner.__ptr))

    -- access psztext member
    assert("nil" == type(struct.inner.pszText))
    assert(nil   == struct.inner.pszText)
end

--
-- test assignment to struct members
--
function TestTESTSTRUCT:testAssignment()
    local struct = TESTSTRUCT:new()

    struct.u8  = 11
    struct.i8  = 22
    struct.u16 = 33
    struct.i16 = 44
    struct.u32 = 55
    struct.i32 = 66
    struct.ptr = "hello"
    struct.chararray = "hello"
    struct.wchararray = "world"
    struct.intarray[1] = 10
    struct.intarray[2] = 20
    struct.intarray[3] = 30
    struct.intarray[4] = 40

    -- check assigned values
    assert(struct.u8  == 11)
    assert(struct.i8  == 22)
    assert(struct.u16 == 33)
    assert(struct.i16 == 44)
    assert(struct.u32 == 55)
    assert(struct.i32 == 66)
    assert(struct.ptr == "hello")
    assert(tostring(struct.chararray) == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(tostring(struct.wchararray) == "world\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(struct.intarray[1] == 10)
    assert(struct.intarray[2] == 20)
    assert(struct.intarray[3] == 30)
    assert(struct.intarray[4] == 40)
end

--
-- test references
--
function TestTESTSTRUCT:testReference()
    local struct = TESTSTRUCT:new()

    -- test pointer reset (reference removal)
    struct.ptr = "hello2"
    assert(struct.ptr == "hello2")

    struct.ptr = nil
    assert(struct.ptr == nil)

    struct.ptr = "hello2"
    assert(struct.ptr == "hello2")

    struct.ptr = 0
    assert(struct.ptr == nil)
end

--
-- test assignment of table to struct via set function
--
function TestTESTSTRUCT:testTableAssignment()

    local struct = TESTSTRUCT:new()

    -- test table assignment
    struct:set{
      u8  = 91,
      i8  = 92,
      u16 = 93,
      i16 = 94,
      u32 = 95,
      i32 = 96,
      ptr = "hello",
      chararray = "hello",
      wchararray = "world",
      intarray = { 19,
             29,
             39,
             49,
      },
    }

    assert(struct.u8  == 91)
    assert(struct.i8  == 92)
    assert(struct.u16 == 93)
    assert(struct.i16 == 94)
    assert(struct.u32 == 95)
    assert(struct.i32 == 96)
    assert(struct.ptr == "hello")
    assert(tostring(struct.chararray) == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(tostring(struct.wchararray) == "world\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(struct.intarray[1] == 19)
    assert(struct.intarray[2] == 29)
    assert(struct.intarray[3] == 39)
    assert(struct.intarray[4] == 49)
end

--
-- test assignment of table to struct during new
--
function TestTESTSTRUCT:testNewWithTableAssignment()

    -- test new with table assignment
    local struct = TESTSTRUCT:new{
      u8  = 91,
      i8  = 92,
      u16 = 93,
      i16 = 94,
      u32 = 95,
      i32 = 96,
      ptr = "hello",
      chararray = "hello",
      wchararray = "world",
      intarray = { 19,
             29,
             39,
             49,
      }
    }

    assert(struct.u8  == 91)
    assert(struct.i8  == 92)
    assert(struct.u16 == 93)
    assert(struct.i16 == 94)
    assert(struct.u32 == 95)
    assert(struct.i32 == 96)
    assert(struct.ptr == "hello")
    assert(tostring(struct.chararray) == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(tostring(struct.wchararray) == "world\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(struct.intarray[1] == 19)
    assert(struct.intarray[2] == 29)
    assert(struct.intarray[3] == 39)
    assert(struct.intarray[4] == 49)

    assert(nil == struct.inner.pszText)

    -- test new with assignment from existing struct
    local struct2 = TESTSTRUCT:new(struct)

    assert(struct2.u8  == 91)
    assert(struct2.i8  == 92)
    assert(struct2.u16 == 93)
    assert(struct2.i16 == 94)
    assert(struct2.u32 == 95)
    assert(struct2.i32 == 96)
    assert(struct2.ptr == "hello")
    assert(tostring(struct2.chararray) == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(tostring(struct.wchararray) == "world\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(struct2.intarray[1] == 19)
    assert(struct2.intarray[2] == 29)
    assert(struct2.intarray[3] == 39)
    assert(struct2.intarray[4] == 49)

    assert(nil == struct2.inner.pszText)


    -- test __dup() from existing struct
    local struct3 = struct:__dup()

    assert(struct3.u8  == 91)
    assert(struct3.i8  == 92)
    assert(struct3.u16 == 93)
    assert(struct3.i16 == 94)
    assert(struct3.u32 == 95)
    assert(struct3.i32 == 96)
    assert(struct3.ptr == "hello")
    assert(tostring(struct3.chararray) == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(tostring(struct.wchararray) == "world\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
    assert(struct3.intarray[1] == 19)
    assert(struct3.intarray[2] == 29)
    assert(struct3.intarray[3] == 39)
    assert(struct3.intarray[4] == 49)
end

--
-- test member size operator
--
function TestTESTSTRUCT:testSizes()

    local struct = TESTSTRUCT:new()

    -- check sizes
    -- assert(#struct == 88)             -- depends on packing

    assert(#struct.intarray == 4)
    assert(#struct.chararray == 32)

    assert(0 == struct.ref.ref)
    assert(nil == struct.ref.value)

    -- check $ref types
    struct.ref = "my unique reference string"

    assert(0 ~= struct.ref.ref)

    assert("string" == type(struct.ref.value))
    assert("my unique reference string" == struct.ref.value)
end

--
-- test attaching to a given pointer
--
function TestTESTSTRUCT:testAttach()
    -- test attach
    function myfunc(s)
      local wrap = TESTSTRUCT:attach(s)

      assert(nil ~= wrap.ptr)

      assert(wrap.u8  ==   8)
      assert(wrap.i8  == - 8)
      assert(wrap.u16 ==  16)
      assert(wrap.i16 == -16)
      assert(wrap.u32 ==  32)
      assert(wrap.i32 == -32)

      -- print(wrap)

    end
    testluacwrap.callwithTESTSTRUCT(myfunc)
end

--
-- test attach to inner struct
--
function TestTESTSTRUCT:testAttachWithLightEmbeddedStructs()

    -- semantic of attach() with light embedded structs
    function myfunc(struct)
      local wrap = TESTSTRUCT:attach(struct)

      local attached = INNERSTRUCT:attach(wrap)

      -- check if they point to the same memory
      assert(wrap.__ptr == attached.__ptr)
    end
    testluacwrap.callwithTESTSTRUCT(myfunc)
end

--
-- test access to boxed object
--
function TestTESTSTRUCT:testBoxed()
    -- test boxed
    function myfunc(struct)
      -- print(struct)
      -- print(testluacwrap.printTESTSTRUCT(struct))

      assert(struct.u8  ==   8)
      assert(struct.i8  == - 8)
      assert(struct.u16 ==  16)
      assert(struct.i16 == -16)
      assert(struct.u32 ==  32)
      assert(struct.i32 == -32)
    end
    testluacwrap.callwithBoxedTESTSTRUCT(myfunc)
end

--
-- test access to wrapped stack based objects
--
function TestTESTSTRUCT:testCallwithwrappedTESTSTRUCT()

    -- callwithwrappedTESTSTRUCT")
    -- test wrapped stack based objects
    function wrapfunc(wrap)
      -- print(wrap)
      assert(wrap.u8  ==   8)
      assert(wrap.i8  == - 8)
      assert(wrap.u16 ==  16)
      assert(wrap.i16 == -16)
      assert(wrap.u32 ==  32)
      assert(wrap.i32 == -32)

    end
    testluacwrap.callwithwrappedTESTSTRUCT(wrapfunc)
end

--
-- test call with reference type
--
function TestTESTSTRUCT:testCallwithRefType()

    -- callwithRefType
    -- test wrapped stack based objects
    function wrapfunc(wrap)
      -- print(wrap)

      assert(wrap.u8  ==   8)
      assert(wrap.i8  == - 8)
      assert(wrap.u16 ==  16)
      assert(wrap.i16 == -16)
      assert(wrap.u32 ==  32)
      assert(wrap.i32 == -32)

      -- print("wrap.__ptr", wrap.__ptr)
      assert(0 ~= wrap.ref.ref)

      assert("string" == type(wrap.ref.value))
      assert("callwithRefType" == wrap.ref.value)

      -- print(testluacwrap.printTESTSTRUCT(wrap))
    end
    testluacwrap.callwithRefType(wrapfunc, "callwithRefType")
end

--
-- test fixed memory buffers
--
function TestTESTSTRUCT:testBuffers()
    -- checkbuffers
    local mybuf = luacwrap.createbuffer(256)
    -- print(mybuf)
    local mybuf2 = luacwrap.createbuffer(256)
    -- print(mybuf2)

    -- create instance
    mybuf = "hello"

    assert("hello" == mybuf)
    -- print("content of mybuf:", mybuf)
end

--
-- test array type
--
function TestTESTSTRUCT:testArrays()
    -- create type descriptor
    local type_double128 = luacwrap.registerarray("double128", 128, "$dbl")
    -- create instance
    local myarray = type_double128:new()
    -- print(myarray)
    
    assert(#myarray  == 128)
    assert("number" == type(myarray[1]))
    
    for idx=1, 128 do
      myarray[idx] = idx
    end
    
    for idx=1, 128 do
      assert(myarray[idx] == idx)
    end
end

--
-- test registering custom struct type
--
function TestTESTSTRUCT:testRegisterStructType()
    -- create type descriptor
    local type_mystruct = luacwrap.registerstruct("mystruct", 8,
      {
        { "member1", 0, "$i32" },
        { "member2", 4, "$i32" }
      }
    )
    -- create struct instance
    local mystruct = type_mystruct:new()

    -- access members
    mystruct.member1 = 10
    mystruct.member2 = 22
    
    assert(mystruct.member1 == 10)
    assert(mystruct.member2 == 22)
end


os.exit(lu.run())
