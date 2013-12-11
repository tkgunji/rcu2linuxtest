# $Id: Makefile.am,v 1.7 2008/02/27 07:47:54 richter Exp $
# Makefile template for the DCS board tools
#

SUBDIRS = dcscMsgBufferInterface src doc

DRIVER_DIR =	drivers
DRIVER_SRC = 	dcs_driver.c \
		dcs_driver.h \
		mrKernLogging.c \
		mrKernLogging.h \
		rcu_flash.c \
		rcu_flash.h \
		version.h

EXTRA_DIST = 	$(DRIVER_SRC:%=drivers/%) \
		$(DRIVER_DIR)/Makefile.in

if EN_DRIVERS
DRIVERS=make_drivers
endif

all-am: $(DRIVERS)

make_drivers: $(DRIVER_SRC:%=$(top_srcdir)/$(DRIVER_DIR)/%)
	$(MAKE) -C $(DRIVER_DIR) modules

clean-local:
	$(MAKE) -C $(DRIVER_DIR) clean

install-exec-local:
	test "x$(DRIVERS)" == "x" || $(MAKE) $(AM_MAKEFLAGS) -C $(DRIVER_DIR) install KVERS=$(KVERS)

uninstall-am: uninstall-info-am
	test "x$(DRIVERS)" == "x" || $(MAKE) $(AM_MAKEFLAGS) -C $(DRIVER_DIR) uninstall

#
# EOF
#
