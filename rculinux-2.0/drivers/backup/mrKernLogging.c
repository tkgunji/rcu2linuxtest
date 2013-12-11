// $Id: mrKernLogging.c,v 1.5 2006/05/30 10:07:37 richter Exp $

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
#ifndef __MR_KERN_LOGGING_C
#define __MR_KERN_LOGGING_C

#include "mrKernLogging.h"

/**************************************************************************************************
 */
//#define MR_KERN_DEBUG KERN_INFO
#define MR_KERN_DEBUG KERN_DEBUG

#define LOG_BUFFER_SIZE 1024
#define LOG_FILE_NAME_MAX_LENGTH 100
#define DEFAULT_LOG_PREFIX "dcsc: "

const char* g_mrLogPrefix=DEFAULT_LOG_PREFIX;
//u16 g_logflags=LOG_FLAG_ALL;
u16 g_logflags=LOG_ERROR|LOG_WARNING|LOG_INFO|LOG_OPEN_CLOSE;
char g_logfilename[LOG_FILE_NAME_MAX_LENGTH]="";
#ifndef NODEBUG
//int mrLogMessage(const char* origin, u16 logflag, const char *fmt, ...)
int mrlogmessage(u16 logflag, const char *fmt, ...)
{
	va_list args;
	int tgtLen;
	static char logBuffer[LOG_BUFFER_SIZE];
	int iBufferSize=LOG_BUFFER_SIZE;
	char* tgtBuffer=logBuffer;
	const char* srcFmt=fmt;

	// is there a kernel log level string? copy it first
	if (fmt[0]=='<' && fmt[1]>='0' && fmt[1]<='7' && fmt[2]=='>') {
	  *tgtBuffer++=*srcFmt++;
	  *tgtBuffer++=*srcFmt++;
	  *tgtBuffer++=*srcFmt++;
	  iBufferSize-=3;
	}

	tgtLen = snprintf(tgtBuffer, iBufferSize, g_mrLogPrefix); // add logging prefix
	if (tgtLen>=0) {
	  tgtBuffer+=tgtLen; iBufferSize-=tgtLen;
	  va_start(args, fmt);
	  tgtLen = vsnprintf(tgtBuffer, iBufferSize, srcFmt, args);
	  if (tgtLen>0) {
	    tgtBuffer+=tgtLen;
	    if (tgtLen<LOG_BUFFER_SIZE-1) {
	      *tgtBuffer++='\n'; // add newline if space in buffer
	    }
	    *tgtBuffer=0; // terminate the buffer
	    if (((g_logflags&LOG_FLAG_ALL_LEVELS)&logflag)>0 &&    // check the log level
		((logflag&~LOG_FLAG_ALL_LEVELS)==0                 // no specific bit set 
		 || ((g_logflags&~LOG_FLAG_ALL_LEVELS)&logflag)>0) // check the specific bit
		) {
	      // print if logflag matches
	      printk(logBuffer);
	    }
	  }
	}
	return tgtLen;
}

// so far variable arguments doesnt seem to work
/* int mrlogmessage(u16 logflag, const char *fmt, ...) */
/* { */
/*   return mrLogMessage(NULL, logflag, fmt, __VA_ARGS__); */
/* } */
#endif //NODEBUG

void mrAddLogPrefix(const char* logprefix, const char* origin) {
  g_mrLogPrefix=logprefix;
}

u16 mrSetLogFilter(u16 filter) {
  g_logflags=filter;
  return  g_logflags;
}

#endif //__MR_KERN_LOGGING_C
