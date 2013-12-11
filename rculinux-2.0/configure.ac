dnl -*- mode: autoconf -*- 
dnl
dnl $Id: configure.ac,v 1.12 2008/02/27 07:47:54 richter Exp $
dnl template for the configuration script for the RCU linux tools
dnl 
dnl ------------------------------------------------------------------
AC_INIT([RCU linux tools and drivers for the DCS board] , [1.5], 
	[Matthias.Richter@ift.uib.no], rculinux)

RCU_SH_VERSION=1.4.2
RCUBUS_DRIVER_VERSION=0.6
AC_SUBST(RCU_SH_VERSION)
AC_SUBST(RCUBUS_DRIVER_VERSION)

dnl ------------------------------------------------------------------
AC_CANONICAL_SYSTEM
AC_PREFIX_DEFAULT(${PWD})
AC_CONFIG_SRCDIR(src/sendRCUcommand.c)
AM_INIT_AUTOMAKE

dnl ------------------------------------------------------------------
dnl 
dnl Check host type
dnl  
dnl AC_CANONICAL_HOST
dnl

case "$host" in
arm-*-linux*|arm-*linux*)
   # the ARM_COMPILING switch is used in the feeserver/Makefile.am
   # to set the linkink option to libtool (-all-static)
   # so far I couldn't find out how to do this on the configure 
   # command line. Although this flag should be set according to
   # users choice of --anable/disable-shared
  arm_compiling=yes ;
#  KSRC=/usr/local/arm/armlinux/kernel/linux-2.4.21-rmk1-dcs1 ;
#  KSRC=/home/trd/firmware/firmware.371/linux-2.4.21 ;
#  KVERS=2.4;;
  KSRC=/rcu2/release/linux-cortexm-1.11.0 ;
  KVERS=1.11;;
*-*-linux*) 
  KVERS=`uname -r` 
  ;;
*) 
  # this package is currently for linux only
  AC_MSG_ERROR([rculinux is currently only supported on Linux])
  ;;
esac
AM_CONDITIONAL(ARM_COMPILING, test "x$arm_compiling" = "xyes" )

AC_MSG_CHECKING([for Linux kernel version])
AC_ARG_WITH([kernel-version],
	    [AC_HELP_STRING([--with-kernel-version=VERSION],
	                    [Specify a different kernel version VERSION for driver compilation])])
if test "x$with_kernel_version" != "x" && 
   test "x$with_kernel_version" != "xno" ;then
   KVERS=$with_kernel_version
fi

AC_ARG_WITH([kernel-src],
	    [AC_HELP_STRING([--with-kernel-src=PATH],
	                    [Specify kernel sources for driver compilation])])
if test "x$with_kernel_src" != "x" && 
   test "x$with_kernel_src" != "xno" ;then
   KSRC=$with_kernel_src
fi

case $KVERS in 
2.2*|2.4*)
   pre26_build=yes
   MODEXT=o
   ;;
*)
   pre26_build=no
   MODEXT=ko
   ;;
esac

AM_CONDITIONAL(PRE26_BUILD, test "x$pre26_build" = "xyes")
AC_MSG_RESULT($KVERS)
AC_SUBST(KVERS)
AC_SUBST(MODEXT)
AC_MSG_CHECKING(for kernel sources)
if test "x$KSRC" = "x" ; then
   KSRC=/lib/modules/$KVERS/build
fi
if test "x$arm_compiling" != "xyes"; then
   AC_MSG_RESULT(no - driver built for arm architecture only)
elif test ! -d  $KSRC/include/linux ; then 
   AC_MSG_RESULT(no - driver not built)
else 
   AC_MSG_RESULT($KSRC)
fi
AC_SUBST(KSRC)
AM_CONDITIONAL(EN_DRIVERS, test "x$arm_compiling" = "xyes" )

dnl ------------------------------------------------------------------
AC_PROG_CC
AC_PROG_LIBTOOL
AC_SUBST(LD)

AC_DEBUG
AC_OPTIMIZATION

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable DCSC simulation])
AH_TEMPLATE([DCSC_SIMULATION],[Simulated DCS])
AC_ARG_ENABLE(dcscsim,
  [AC_HELP_STRING([--enable-dcscsim],
      [Turn on register simulation feature for Msg Buffer Interface ])],
  [],[enable_dcscsim=no])
if test "x$enable_dcscsim" = "xyes" ; then 
  AC_DEFINE(DCSC_SIMULATION)
fi
AC_MSG_RESULT([$enable_dcscsim])

dnl ------------------------------------------------------------------
if test "x$enable_debug" != "xno" ; then
   AC_DEFINE(ENABLE_MEMORY_GUARD)
fi

dnl ------------------------------------------------------------------
dnl
dnl Documentation
dnl
AC_ARG_VAR(DOXYGEN, The Documentation Generator)
AC_PATH_PROG(PERL, perl)
AC_PATH_PROG(DOXYGEN, doxygen)
AM_CONDITIONAL(HAVE_DOXYGEN, test ! "x$DOXYGEN" = "x")
HAVE_DOT=NO
DOT_PATH=
AC_PATH_PROG(DOT, dot)
if ! test "x$DOT" = "x" ; then
   HAVE_DOT=YES
   DOT_PATH=`dirname $DOT`
fi
AC_SUBST([HAVE_DOT])
AC_SUBST([DOT_PATH])

AC_MSG_CHECKING([whether to build pdf documentation])
AC_ARG_ENABLE(pdf-doc,
  [AC_HELP_STRING([--enable-pdf-doc],
      [Build documentation in pdf format])],
  [],[enable_pdf_doc=no])
AM_CONDITIONAL(CREATE_PDF_DOC, test "x$enable_pdf_doc" = "xyes" )
AC_MSG_RESULT([$enable_pdf_doc])

dnl ------------------------------------------------------------------
AC_CONFIG_FILES([Makefile 
		 src/Makefile
		 dcscMsgBufferInterface/Makefile
		 doc/Makefile
		 drivers/Makefile
		 doc/doxygen.conf])

AC_OUTPUT
dnl
dnl EOF
dnl
