#------
# Load configuration
#
include config

#------
# Hopefully no need to change anything below this line
#
# INSTALL_LUACWRAP_SHARE=$(INSTALL_TOP_SHARE)/luacwrap

all clean test:
	cd src; $(MAKE) $@

manual:
	cd doc; $(MAKE) $@

#------
# Install LuaCwrap according to recommendation
#
install: all
	cd src; mkdir -p $(INSTALL_TOP_LIB)
	# install library
	cd src; $(INSTALL_EXEC) $(LUACWRAP_VNAME) $(INSTALL_TOP_LIB)/$(LUACWRAP_VNAME)
	# enable runtime binding via soname
	$(INSTALL_LINK) $(INSTALL_TOP_LIB)/$(LUACWRAP_VNAME) $(INSTALL_TOP_LIB)/$(LUACWRAP_MNAME)
	# enable -llua5.1-luacwrap for gcc
	$(INSTALL_LINK) $(INSTALL_TOP_LIB)/$(LUACWRAP_VNAME) $(INSTALL_TOP_LIB)/$(LUACWRAP_LIBNAME).$(EXT)
	# enable require("luacwrap") within lua scripts
	sudo mkdir -p $(INSTALL_LUA_LIB)
	$(INSTALL_LINK) $(INSTALL_TOP_LIB)/$(LUACWRAP_VNAME) $(INSTALL_LUA_LIB)/luacwrap.$(EXT)
	# update linker cache
	sudo ldconfig

#------
# Install LuaCwrap according to recommendation
#
uninstall:
	rm $(INSTALL_TOP_LIB)/$(LUACWRAP_VNAME)
	rm $(INSTALL_TOP_LIB)/$(LUACWRAP_MNAME)
	rm $(INSTALL_TOP_LIB)/$(LUACWRAP_LIBNAME).$(EXT)
	rm $(INSTALL_LUA_LIB)/luacwrap.$(EXT)

#------
# End of makefile
#
