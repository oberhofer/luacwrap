testluacwrap = require("testluacwrap")

-- helper functions
function printtable(t)
  for k,v in pairs(t) do
    print("  ", k, v)
  end
end

print("--> create TESTSTRUCT")
local struct = TESTSTRUCT:new()

print("--> semantic of attach()")
local attached = TESTSTRUCT:attach(struct)
assert(struct.__ptr == attached.__ptr)

print("--> checkInnerStructAccess")
print(getmetatable(struct))
for k, v in pairs(getmetatable(struct)) do
	print(k, v)
end
print("----")
print(getmetatable(struct.inner))
for k, v in pairs(getmetatable(struct.inner)) do
	print(k, v)
end
print("----")

assert(1 == testluacwrap.checkInnerStructAccess(struct, struct.inner))

print("--> check $ref init values")

-- by default references should have the value nil
assert(nil == struct.ref.value)
-- and the reserved index 0
assert(0 == struct.ref.ref)

-- print adresses
print("--> print adresses (__ptr)")
print("struct.__ptr", struct.__ptr)
print("struct.intarray.__ptr", struct.intarray.__ptr)
print("struct.inner.__ptr", struct.inner.__ptr)

-- print psztext member
print("struct.inner.pszText", struct.inner.pszText)

print("--> test assignment")
struct.u8  = 11
struct.i8  = 22
struct.u16 = 33
struct.i16 = 44
struct.u32 = 55
struct.i32 = 66
struct.ptr = "hello"
struct.chararray = "hello"
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
assert(struct.chararray == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
assert(struct.intarray[1] == 10)
assert(struct.intarray[2] == 20)
assert(struct.intarray[3] == 30)
assert(struct.intarray[4] == 40)

print("--> test pointer reset (reference removal)")

struct.ptr = "hello2"
assert(struct.ptr == "hello2")

struct.ptr = nil
assert(struct.ptr == nil)

struct.ptr = "hello2"
assert(struct.ptr == "hello2")

struct.ptr = 0
assert(struct.ptr == nil)

print("--> test table assignment")
struct:set{
	u8  = 91,
	i8  = 92,
	u16 = 93,
	i16 = 94,
	u32 = 95,
	i32 = 96,
	ptr = "hello",
	chararray = "hello",
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
assert(struct.chararray == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
assert(struct.intarray[1] == 19)
assert(struct.intarray[2] == 29)
assert(struct.intarray[3] == 39)
assert(struct.intarray[4] == 49)

print("--> test new with table assignment")
struct = TESTSTRUCT:new{
	u8  = 91,
	i8  = 92,
	u16 = 93,
	i16 = 94,
	u32 = 95,
	i32 = 96,
	ptr = "hello",
	chararray = "hello",
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
assert(struct.chararray == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
assert(struct.intarray[1] == 19)
assert(struct.intarray[2] == 29)
assert(struct.intarray[3] == 39)
assert(struct.intarray[4] == 49)

print("struct.inner.pszText", struct.inner.pszText)

--[[
print("--> test new with assignment")
struct2 = TESTSTRUCT:new(struct)

print(struct2)

assert(struct2.u8  == 91)
assert(struct2.i8  == 92)
assert(struct2.u16 == 93)
assert(struct2.i16 == 94)
assert(struct2.u32 == 95)
assert(struct2.i32 == 96)
assert(struct2.ptr == "hello")
assert(struct2.chararray == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
assert(struct2.intarray[1] == 19)
assert(struct2.intarray[2] == 29)
assert(struct2.intarray[3] == 39)
assert(struct2.intarray[4] == 49)
--]]

print("--> test new with assignment")
for k, v in pairs(getmetatable(struct)) do
	print(k, v)
end



struct2 = struct:__dup()

print(struct2)

assert(struct2.u8  == 91)
assert(struct2.i8  == 92)
assert(struct2.u16 == 93)
assert(struct2.i16 == 94)
assert(struct2.u32 == 95)
assert(struct2.i32 == 96)
assert(struct2.ptr == "hello")
assert(struct2.chararray == "hello\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")
assert(struct2.intarray[1] == 19)
assert(struct2.intarray[2] == 29)
assert(struct2.intarray[3] == 39)
assert(struct2.intarray[4] == 49)


-- check sizes
print("--> check __len")
-- assert(#struct == 80)       -- depends on packing, could be 72
print("struct size: ", #struct)
assert(#struct.intarray == 4)
assert(#struct.chararray == 32)

print("--> print")
print(struct)
print("struct.__ptr", struct.__ptr)
print("struct.inner.__ptr", struct.inner.__ptr)
print("struct.ptr", struct.ptr)
print("struct.inner.pszText", struct.inner.pszText)

print("--> internal print")
print(testluacwrap.printTESTSTRUCT(struct))


print("--> check $ref types")
struct.ref = "my ref"
-- check internal reference assignment
assert(struct.ref.ref == 1)

assert("string" == type(struct.ref.value))
assert("my ref" == struct.ref.value)

print("--> test attach")
-- test attach
function myfunc(struct)
  print("within myfunc")
  print(struct)

  local wrap = TESTSTRUCT:attach(struct)
  print(wrap)
  printtable(debug.getfenv(wrap))

  print("wrap.__ptr", wrap.__ptr)
  print(testluacwrap.printTESTSTRUCT(wrap))
end
testluacwrap.callwithTESTSTRUCT(myfunc)

print("--> semantic of attach() with light embedded structs")
function myfunc(struct)
  local wrap = TESTSTRUCT:attach(struct)
  -- print(wrap)
  
  local attached = INNERSTRUCT:attach(wrap)
  -- print(attached)

  -- check if they point to the same memory
  assert(wrap.__ptr == attached.__ptr)  
end
testluacwrap.callwithTESTSTRUCT(myfunc)

print("--> test boxed")
function myfunc(struct)
  print(struct)
  print(debug.getfenv(struct))
  printtable(debug.getfenv(struct))
  print("struct.__ptr", struct.__ptr)
  print(testluacwrap.printTESTSTRUCT(struct))
end
testluacwrap.callwithBoxedTESTSTRUCT(myfunc)


print("--> callwithwrappedTESTSTRUCT")
-- test wrapped stack based objects
function wrapfunc(wrap)
  print(wrap)
end
testluacwrap.callwithwrappedTESTSTRUCT(wrapfunc)


print("--> callwithRefType")
-- test wrapped stack based objects
function wrapfunc(wrap)
  print(wrap)

  assert(wrap.u8  ==   8)
  assert(wrap.i8  == - 8)
  assert(wrap.u16 ==  16)
  assert(wrap.i16 == -16)
  assert(wrap.u32 ==  32)
  assert(wrap.i32 == -32)

  print("wrap.__ptr", wrap.__ptr)

  assert("string" == type(wrap.ref.value))
  assert("callwithRefType" == wrap.ref.value)

  print(testluacwrap.printTESTSTRUCT(wrap))
end
testluacwrap.callwithRefType(wrapfunc, "callwithRefType")


print("--> all checks passed")
