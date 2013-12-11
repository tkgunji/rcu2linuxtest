# $Id: Makefile.am,v 1.15 2008/02/27 08:51:57 richter Exp $
# Makefile template for the RCU shell
#
AM_CFLAGS =
if ARM_COMPILING
  AM_CFLAGS_ARCH = -I$(top_srcdir)/dcscMsgBufferInterface -I$(INSTALL_ROOT)/A2F/root/usr/include
  ARM_COMPILING_LD_FLAGS	=  -all-static -mcpu=cortex-m3 -mthumb -L${INSTALL_ROOT}/A2F/root/usr/lib -pthread
else
  AM_CFLAGS_ARCH = -DDCSC_TEST # choose a dummy device
endif





# for some strange reason automake version 1.6/1.7.8 can not handle assignments in the conditionals
AM_CFLAGS += $(AM_CFLAGS_ARCH)

bin_PROGRAMS                    = rcu-sh framegen framever rcuflashprog bitswapper simple app
#rcubitfileprog acteltest xilinx_beamtest
rcu_sh_SOURCES                  = sendRCUcommand.c \
				  mrtimers.c \
				  mrshellprim.c \
				  cmdInterpreter.c \
				  memoryguard.c \
				  Getline.c 
rcu_sh_LDADD                    = $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o \
				  $(top_builddir)/dcscMsgBufferInterface/selectmapInterface.o

#xilinx_beamtest_SOURCES               = xilinx_beamtest.c xilinx_beamtest.h
#xilinx_beamtest_LDADD                 = $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o

#acteltest_SOURCES               = acteltest.c rcubitfileprog.h
#acteltest_LDADD                 = $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o


#rcubitfileprog_SOURCES          = rcubitfileprog.c rcubitfileprog.h
#rcubitfileprog_LDADD             = $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o

framegen_SOURCES		= framegen.c framehelpers.c
framegen_LDADD			= $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o

AM_LDFLAGS			=  $(ARM_COMPILING_LD_FLAGS)


framever_SOURCES		= framever.c framehelpers.c
framever_LDADD			= $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o

rcuflashprog_SOURCES		= rcuflashprog.c framehelpers.c
rcuflashprog_LDADD		= $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o

bitswapper_SOURCES		= bitswapper.c
bitswapper_LDADD		= $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o

simple_SOURCES			= simple.c
simple_LDADD			= $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o

simple_SOURCES			= app.c
simple_LDADD			= $(top_builddir)/dcscMsgBufferInterface/dcscMsgBufferInterface.o



lib_LTLIBRARIES			= 
noinst_PROGRAMS			= 
noinst_LTLIBRARIES		= 
noinst_HEADERS			= mrtimers.h \
				  mrshellprim.h \
				  cmdInterpreter.h \
				  memoryguard.h \
				  framegen.h \
				  framehelpers.h \
				  rcuflashprog.h \
				  bitswapper.h \
				  framever.h 

#EXTRA_DIST			= driver_include/dcs_driver.h


#
# EOF
#
