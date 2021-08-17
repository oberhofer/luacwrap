#------
# Load configuration
#
include ../config

#------
# Modules belonging to LuaCwrap
#
LUACWRAP_OBJS:=\
	luaaux.o \
	luacwrap.o \
	wrapnumeric.o \
	wrappointer.o \
	wrapreference.o

LUACWRAP_HEADERS:=\
	$(LUACWRAP_INCDIR)/luacwrap.h \
	luacwrap_int.h \
	luaaux.h \
	wrapnumeric.h \
	wrappointer.h \
	wrapreference.h

#------
# Modules belonging to TestLuaCwrap
#
TESTLUACWRAP_OBJS:=\
	luaaux.o \
	testluacwrap.o 

#------
# all target
#
all: $(LUACWRAP_VNAME)

#------
# creates luacwrap dynamic library
#
$(LUACWRAP_VNAME): $(LUACWRAP_OBJS)
	$(LD) $(LDFLAGS) -Wl,-soname,$(LUACWRAP_MNAME) -o $@ $(LUACWRAP_OBJS) -llua5.1

#------
# creates testluacwrap dynamic library
#
$(TESTLUACWRAP_LIBNAME).$(EXT): $(TESTLUACWRAP_OBJS)
	$(LD) $(LDFLAGS) -Wl,-soname,$(TESTLUACWRAP_MNAME) -L$(INSTALL_TOP_LIB) -o $@ $(TESTLUACWRAP_OBJS) -llua5.1

#------
# build/execute test
#
test: $(TESTLUACWRAP_LIBNAME).$(EXT)
	lua5.1 unittest.lua

#------
# List of dependencies
#
luaaux.o: luaaux.c luaaux.h
luacwrap.o: luacwrap.c $(LUACWRAP_HEADERS)
wrapnumeric.o: wrapnumeric.c $(LUACWRAP_HEADERS)
wrappointer.o: wrappointer.c $(LUACWRAP_HEADERS)
wrapreference.o: wrapreference.c $(LUACWRAP_HEADERS)
testluacwrap.o: testluacwrap.c $(LUACWRAP_HEADERS)

#------
# cleanup dynamic libraries and links
#
clean:
	rm -f testluacwrap.$(EXT)
	rm -f $(LUACWRAP_VNAME) 
	rm -f $(LUACWRAP_OBJS) $(TESTLUACWRAP_OBJS) 

#------
# End of makefile configuration
#
