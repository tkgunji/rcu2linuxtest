// $Id: mrKernLogging.h,v 1.5 2006/05/30 10:07:37 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter,
** Matthias.Richter@ift.uib.no
**
** Permission to use, copy, modify and distribute this software and its  
** documentation strictly for non-commercial purposes is hereby granted  
** without fee, provided that the above copyright notice appears in all  
** copies and that both the copyright notice and this permission notice  
** appear in the supporting documentation. The authors make no claims    
** about the suitability of this software for any purpose. It is         
** provided "as is" without express or implied warranty.                 
**
*************************************************************************/

#ifndef __MR_KERN_LOGGING_H
#define __MR_KERN_LOGGING_H

// logging flags
#define LOG_FLAG_ALL  0xffff
#define LOG_FLAG_ALL_LEVELS 0x000f
// bits 0 to 3 represent debug type flags
#define LOG_ERROR      0x0001
#define LOG_WARNING    0x0002
#define LOG_INFO       0x0004
#define LOG_DBG        0x0008

// bits 4 to 15 represent log group flags, e.g. the read function is a logging group
#define LOG_INTERNAL   0x0010 // internal stuff
#define LOG_LLSEEK     0x0020 // seek
#define LOG_READ_WRITE 0x0040 // read/write
#define LOG_IOCTL      0x0080 // ioctl messages
#define LOG_MMAP       0x0100 // everthing concerning mmap
#define LOG_OPEN_CLOSE 0x0200 // open/ close procedures
#define LOG_FPGA_CONF  0x0400 // fpga configuration through separate driver mode
#define LOG_DRIVER_LCK 0x0800 // locking of the driver
#define LOG_SELFTEST   0x1000 // selftest at module load

#ifdef NODEBUG
// for now there are no messages in the release version
// error and warning should be enabled later
// just empty defines
#define mrlogmessage(logflag, ...) 
/* #define mrKernDebug(flag, ...) */
/* #define mrKernInfo(flag, ...) */
/* #define mrKernWarning(flag, ...) */
/* #define mrKernError(flag, ...) */
#else //NODEBUG
// defines for the messages
// couldnt get variable arguments working
/* #define mrKernDebug(flag, ...)   mrlogmessage(__FILE__, flag|LOG_DBG, __VA_ARGs__) */
/* #define mrKernInfo(flag, ...)    mrlogmessage(__FILE__, flag|LOG_INFO, __VA_ARGs__) */
/* #define mrKernWarning(flag, ...) mrlogmessage(__FILE__, flag|LOG_WARNING, __VA_ARGs__) */
/* #define mrKernError(flag, ...)   mrlogmessage(__FILE__, flag|LOG_ERROR, __VA_ARGs__) */

// the messages itself
int mrlogmessage(u16 logflag, const char *fmt, ...);
int mrLogMessage(const char* origin, u16 logflag, const char *fmt, ...);
#endif // NODEBUG

/* add a logging prefix
 * for now just a global variable will be set, for later extension, a list will be
 * created
 * for a stand-alone driver, this logging prefix is taken for all log messages
 * for the composite rcubus driver the logging prefix will be derived from the origin
 */
void mrAddLogPrefix(const char* logprefix, const char* origin);

#endif //__MR_KERN_LOGGING_H
