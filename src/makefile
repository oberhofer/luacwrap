#------
# Load configuration
#
include ../config

#------
# Modules belonging to LuaCwrap
#
LUACWRAP_OBJS:= \
  luaaux.o \
  luacwrap.o \
  wrapnumeric.o 

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
	$(LD) $(LDFLAGS) -Wl,-soname,$(LUACWRAP_MNAME) -o $@ $(LUACWRAP_OBJS)

#------
# creates testluacwrap dynamic library
#
$(TESTLUACWRAP_LIBNAME).$(EXT): $(TESTLUACWRAP_OBJS)
	$(LD) $(LDFLAGS) -Wl,-soname,$(TESTLUACWRAP_MNAME) -L$(INSTALL_TOP_LIB) -o $@ $(TESTLUACWRAP_OBJS) -llua5.1-luacwrap

#------
# build/execute test
#
test: $(TESTLUACWRAP_LIBNAME).$(EXT)
	lua unittest.lua

#------
# List of dependencies
#
luaaux.o: luaaux.c luaaux.h
luacwrap.o: luacwrap.c $(LUACWRAP_INCDIR)/luacwrap.h luaaux.h wrapnumeric.h
wrapnumeric.o: wrapnumeric.c wrapnumeric.h luaaux.h
testluacwrap.o: testluacwrap.c luaaux.h

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
