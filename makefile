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
	cd src; $(INSTALL_EXEC) $(LUACWRAP_SO) $(INSTALL_TOP_LIB)/luacwrap.$(EXT)
	cd src; $(INSTALL_EXEC) $(TESTLUACWRAP_SO) $(INSTALL_TOP_LIB)/testluacwrap.$(EXT)

#------
# End of makefile
#
