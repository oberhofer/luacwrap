testluacwrap = require("testluacwrap")

print("--> create TESTSTRUCT")
local struct = TESTSTRUCT:new()

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

-- check sizes
print("--> check __len")
-- assert(#struct == 80)       -- depends on packing, could be 72 
print("struct size: ", #struct)
assert(#struct.intarray == 4)
assert(#struct.chararray == 32)

print("--> print")
print(struct)

print("--> internal print")
print(testluacwrap.printTESTSTRUCT(struct))

print("--> all checks passed")
