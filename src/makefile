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
all: $(LUACWRAP_SO) $(TESTLUACWRAP_SO) 

#------
# creates luacwrap dynamic library
#
$(LUACWRAP_SO): $(LUACWRAP_OBJS)
	$(LD) $(LDFLAGS) -Wl,-soname,luacwrap.$(EXT).$(LUACWRAP_MAJOR) -o $@ $(LUACWRAP_OBJS)
	ln $(LUACWRAP_SO) luacwrap.$(EXT).$(LUACWRAP_MAJOR)
	ln $(LUACWRAP_SO) luacwrap.$(EXT)

#------
# creates testluacwrap dynamic library
#
$(TESTLUACWRAP_SO): $(TESTLUACWRAP_OBJS)
	$(LD) $(LDFLAGS) -Wl,-soname,testluacwrap.$(EXT).$(LUACWRAP_MAJOR) -L. -Wl,-rpath,`pwd` -o $@ $(TESTLUACWRAP_OBJS) $(LUACWRAP_SO)
	ln $(TESTLUACWRAP_SO) testluacwrap.$(EXT)

#------
# List of dependencies
#
luaaux.o: luaaux.c luaaux.h
luacwrap.o: luacwrap.c luacwrap.h luaaux.h wrapnumeric.h
wrapnumeric.o: wrapnumeric.c wrapnumeric.h luaaux.h
testluacwrap.o: testluacwrap.c luaaux.h

#------
# cleanup dynamic libraries and links
#
clean:
	rm -f testluacwrap.$(EXT)
	rm -f luacwrap.$(EXT).$(LUACWRAP_MAJOR)
	rm -f luacwrap.$(EXT)
	rm -f $(LUACWRAP_SO) $(LUACWRAP_OBJS) 
	rm -f $(TESTLUACWRAP_SO) $(TESTLUACWRAP_OBJS) 

#------
# End of makefile configuration
#
