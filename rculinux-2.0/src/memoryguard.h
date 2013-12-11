// $Id: memoryguard.h,v 1.1 2006/02/13 11:16:22 richter Exp $

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

#ifndef _MEMORYGUARD_H
#define _MEMORYGUARD_H

/*
 * @brief logging levels
 */
enum {
  /** log all messages */
  kmmg_all     = 0,
  /** debug message */
  kmmg_debug,
  /** info message */
  kmmg_info,  
  /** warning message */
  kmmg_warning,
  /** error message */
  kmmg_error
};

#ifdef ENABLE_MEMORY_GUARD
   // calls to malloc and free redirected for bookkeeping
#  ifndef __MEMORYGUARD_C
     // redirect calls for all other source files than the memory guard implementation 

//#    define malloc(i) mmg_malloc(i)
//#    define free(p) mmg_free(p)
#    define malloc(i) mmg_malloc(i, __FILE__, __LINE__)
#    define free(p) mmg_free(p, __FILE__, __LINE__)
#  endif

/** init the memory guard
 * internal structures are set up
 */
int mmg_init(const char* pLogFile, int iLogLevel);

/** exit the meory guard
 * a final check for unreleased memory will be performed
 */
int mmg_exit();

/** the alloc function of the memory guard
 */
void* mmg_malloc(int size, const char* file, int lineno);

/** the free function of the memory guard
 */
void  mmg_free(void* p, const char* file, int lineno);

#else //!__DEBUG
   // no memory guard for program releases
   // malloc and free used directly
#  include <stdlib.h>
#  define mmg_init
#  define mmg_exit()
#endif //ENABLE_MEMORY_GUARD

#endif // _MEMORYGUARD_H
