// $Id: version.h,v 1.7 2006/05/26 12:14:57 richter Exp $

/*
25.04.2004 version 0.1
 - implementation of ioctl functions:
  IOCLT_READ_REG, IOCLT_WRITE_REG, IOCLT_GET_MSGBUF_IN_SIZE, IOCLT_GET_MSGBUF_OUT_SIZE,
  IOCLT_GET_REGFILE_SIZE, IOCLT_GET_VERSION
 - access restriction to 4 byte two for byte words abandoned, each number of bytes can be read/written now
 
26.04.2004 version 0.2
 - new default values for the message buffer size, new design has 1024 byte in and out buffer

04.01.2005 version 0.3
 - log messages concentrated in one log function which can be switched off easily
 - log flags introduced for different levels of messages 
 - 'release' version introduced, activated with 'make release=yes'
 - this is just a test

04.05.2005 version 0.4
 - renamed to rcubus_driver
 - bugfix in ioctl read/write register
 - changed default regfile address, now 0x80000060
25.05.2006 version 0.6
 - buffer size check in module init removed in order to make the driver more general
   if all 3 buffers are provided, the buffers are assumed to represent the Msg
   Buffer Interface and the memory check is performed 
 - bugfixes in driver lock
 - master lock introduced 
 */
#define DRIVER_MAJOR_VERSION_NUMBER 0
#define DRIVER_MINOR_VERSION_NUMBER 6
#define DRIVER_BUILD_TIME "04.05.2005"
