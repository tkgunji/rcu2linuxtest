# $Id: Makefile.am,v 1.9 2007/04/01 22:19:19 richter Exp $
# Makefile template for the DCS board message buffer interface
#
noinst_LTLIBRARIES                   = libdcscMsgBufferInterface.la
libdcscMsgBufferInterface_la_SOURCES = 	dcscMsgBufferInterface.c \
					dcscMsgBufferInterface.h \
					selectmapInterface.c \
					selectmapInterface.h
noinst_HEADERS		             = driver_include/dcs_driver.h \
					driver_include/virtex_io.h
#AM_LDFLAGS			     =
AM_CFLAGS			     = -I$(top_srcdir)/drivers \
					-I$(srcdir)/driver_include

if ARM_COMPILING
ARM_COMPILING_C_FLAGS		     =  -I$(INSTALL_ROOT)/A2F/root/usr/include
ARM_COMPILING_LD_FLAGS		     =  -mcpu=cortex-m3 -mthumb -L${INSTALL_ROOT}/A2F/root/usr/lib -pthread
else
ARM_COMPILING_C_FLAGS		     =  -DDCSC_TEST
ARM_COMPILING_LD_FLAGS		     =  
endif
AM_CFLAGS			     += $(ARM_COMPILING_C_FLAGS)
AM_LDFLAGS			     = $(ARM_COMPILING_LD_FLAGS)
#
# EOF
#
