// $Id: memoryguard.c,v 1.1 2006/02/13 11:16:22 richter Exp $

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

#define __MEMORYGUARD_C
#include "memoryguard.h"

#ifdef ENABLE_MEMORY_GUARD // set in memory guard.h
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>

/********************************************************************/

/* @name Types
 *
 */

/* @struct mmg_entry
 * @brief  descriptor of memory segments
 */
struct mmg_entry {
  /** pointer to next entry */
  struct mmg_entry *pNext;
  /** pointer to previous entry */
  struct mmg_entry *pPrev;
  /** pointer to the memory block */
  void* p;
  /** size of the memory block */
  int iSize;
  /** total size of the memory block including safety margin */
  int iAllocated;
  /** source file name of the allocation call */
  const char* file;
  /** line number of the allocation call */
  int lineno;
  /** timestamp of the call */
  time_t ts;
};


/********************************************************************/

/* @name Global variables
 *
 */

/* @def   MMG_INITIAL_LIST_SIZE
 * @brief size of the initial table entires allocated in the program heap
 */ 
#define MMG_INITIAL_LIST_SIZE 100
/* @global g_mmg_fixed
 * @brief the initial elements of the mmg list are allocated in the program heap
 */
struct mmg_entry g_mmg_fixed[MMG_INITIAL_LIST_SIZE];
/* @global g_AnchorFree
 * @brief anchor of the list of free entries
 */
struct mmg_entry g_AnchorFree;
/* @global g_AnchorActive
 * @brief anchor of the list of active entries
 */
struct mmg_entry g_AnchorActive;
/* @global g_nofAllocated
 * @brief  the number of curently allocated blocks
 */
int    g_nofAllocated=0;
/* @global g_maxAllocated
 * @brief  the maximum count of allocated blocks
 */
int    g_maxAllocated=0;
/* @global g_nofAllocations
 * @brief  the total number of allocations
 */
int    g_nofAllocations=0;
/* @global g_fpLog
 * @brief  log file instance
 */
FILE* g_fpLog=NULL;
/* @global g_logLevel
 * @brief global logging level of the memory guard
 */
int g_logLevel=0;
/* @global mutex
 * @brief  mutex for protection of the table operations
 */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/********************************************************************/

/* @name Logging macros
 */

/* @def mmg_debug
 */
#define mmg_debug(format, ...)   if (g_logLevel<=kmmg_debug)   fprintf(g_fpLog==NULL?stderr:g_fpLog, "mmg debug: "   format, ##__VA_ARGS__);
/* @def mmg_info
 */
#define mmg_info(format, ...)    if (g_logLevel<=kmmg_info )   fprintf(g_fpLog==NULL?stderr:g_fpLog, "mmg info: "    format, ##__VA_ARGS__);
/* @def mmg_warning
 */
#define mmg_warning(format, ...) if (g_logLevel<=kmmg_warning) fprintf(g_fpLog==NULL?stderr:g_fpLog, "mmg warning: " format, ##__VA_ARGS__);
/* @def mmg_error
 */
#define mmg_error(format, ...)   if (g_logLevel<=kmmg_error)   fprintf(g_fpLog==NULL?stderr:g_fpLog, "mmg error: "   format, ##__VA_ARGS__);

/********************************************************************/
 
/* insert an element into the list
 * @param pPrev    the element to insert after  
 * @param pElement the element to insert
 */
int mmg_insert(struct mmg_entry* pPrev, struct mmg_entry* pElement) 
{
  int iResult=0;
  if (pPrev && pElement) {
    pthread_mutex_lock(&mutex);
    pElement->pPrev=pPrev;
    pElement->pNext=pPrev->pNext;
    (pPrev->pNext)->pPrev=pElement;
    pPrev->pNext=pElement;
    pthread_mutex_unlock(&mutex);
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

/* remove entry from the list
 * the next and previous element will be changed
 * @param pElement the element to be removed
 */
int mmg_remove(struct mmg_entry* pElement)
{
  int iResult=0;
  if (pElement==NULL) return -EINVAL;
  if (pElement==&g_AnchorFree || pElement==&g_AnchorActive) {
    fprintf(stderr, "mmg error: can not remove anchor %p\n", pElement);
    return -ENOENT;
  }
  pthread_mutex_lock(&mutex);
  (pElement->pPrev)->pNext=pElement->pNext;
  (pElement->pNext)->pPrev=pElement->pPrev;
  pElement->pPrev=NULL;
  pElement->pNext=NULL;
  pthread_mutex_unlock(&mutex);

  return iResult;
}

struct mmg_entry* mmg_findpointer(void* p, struct mmg_entry* anchor)
{
  struct mmg_entry* pEntry=NULL;
  if (anchor) {
    pEntry=anchor->pNext;
    while (pEntry!=anchor) {
      if (pEntry->p!=NULL && pEntry->p==p) break;
      pEntry=pEntry->pNext;
    }
    if (pEntry==anchor) pEntry=NULL;
  }
  return pEntry;
}

int mmg_init(const char* pLogFile, int iLogLevel)
{
  int i=0;
  if (pLogFile) {
    g_fpLog=fopen(pLogFile ,"w");
    if (g_fpLog) {
      fprintf(g_fpLog, "start memory guard logging\n");
      time_t t;
      if (time(&t)>-1) {
	fprintf(g_fpLog, "%s", ctime(&t));
      }
    }
  }
  g_logLevel=iLogLevel;
  memset(g_mmg_fixed, 0, MMG_INITIAL_LIST_SIZE*sizeof(struct mmg_entry));
  memset(&g_AnchorFree, 0, sizeof(struct mmg_entry));
  g_AnchorFree.pNext=&g_AnchorFree;
  g_AnchorFree.pPrev=&g_AnchorFree;
  memset(&g_AnchorActive, 0, sizeof(struct mmg_entry));
  g_AnchorActive.pNext=&g_AnchorActive;
  g_AnchorActive.pPrev=&g_AnchorActive;
  struct mmg_entry *pPrev=&g_AnchorFree;
  for (; i<MMG_INITIAL_LIST_SIZE; i++) mmg_insert(pPrev, &(g_mmg_fixed[i]));
  mmg_debug("memory guard initialized with %d table entries, loglevel %d, logfile %s\n", MMG_INITIAL_LIST_SIZE, iLogLevel, pLogFile);
}

int mmg_exit()
{
  // check if everthing has been released
  while (g_AnchorActive.pPrev!=&g_AnchorActive) {
    mmg_error("leftover memory: %p allocated %s at %s line %d\n", g_AnchorActive.pPrev->p, ctime(&(g_AnchorActive.pPrev->ts)), g_AnchorActive.pPrev->file, g_AnchorActive.pPrev->lineno);
    mmg_remove(g_AnchorActive.pPrev);
  }
  mmg_debug("memory guard exited:\n %d total allocations\n balance %d\n fill level %.2f%%\n", g_nofAllocations, g_nofAllocated, ((float)100*g_maxAllocated)/MMG_INITIAL_LIST_SIZE);
}

void* mmg_malloc(int size, const char* file, int lineno)
{
  int allocate=size+strlen(PACKAGE_BUGREPORT)+1;
  void* p=malloc(allocate);
  if (p) {
    struct mmg_entry* pEntry=NULL;
    g_nofAllocations++;
    if (++g_nofAllocated>g_maxAllocated) g_maxAllocated=g_nofAllocated;
    if (g_AnchorFree.pPrev!=&g_AnchorFree) {
      pEntry=g_AnchorFree.pPrev;
      mmg_remove(pEntry);
    } else {
      // think about dynamic allocation
      // for now just flag that the array was exceeded
      if (g_maxAllocated==MMG_INITIAL_LIST_SIZE)
	mmg_warning("mmg_alloc: out of free entries\n");
    }
    if (pEntry) {
      pEntry->p=p;
      strcpy(p+size, PACKAGE_BUGREPORT);
      pEntry->iSize=size;
      pEntry->iAllocated=allocate;
      pEntry->file=file;
      pEntry->lineno=lineno;
      time(&(pEntry->ts));
      mmg_insert(&g_AnchorActive, pEntry);
      mmg_info("%s allocated %p (%d byte) at %s line %d\n", ctime(&(pEntry->ts)), pEntry->p, pEntry->iSize, file, lineno);
      mmg_debug(" -> descriptor %p\n", pEntry);
    }
  }
  return p;
}

void  mmg_free(void* p, const char* file, int lineno)
{
  if (p) {
    if (g_nofAllocated>0) g_nofAllocated--;
    else mmg_warning("number missmatch in allocated and deallocated blocks\n");
    struct mmg_entry* pEntry=mmg_findpointer(p, &g_AnchorActive);
    if (pEntry) {
      mmg_debug("process \'free\' for descriptor %p\n", pEntry);
      time_t t;
      time(&t);
      mmg_info("%s free %p at %s line %d \n allocated %s at %s %d\n", ctime(&t), p, file, lineno, ctime(&(pEntry->ts)), pEntry->file, pEntry->lineno);
      if (pEntry->iSize<pEntry->iAllocated && strcmp((pEntry->p)+pEntry->iSize, PACKAGE_BUGREPORT)!=0) {
	mmg_error("!!!!!!!!!!!!! buffer overrun detected: %p !!!!!!!!!!!!\n allocated %s at %s line %d\n", pEntry->p, ctime(&(pEntry->ts)), pEntry->file, pEntry->lineno);
      }
      mmg_remove(pEntry);
      memset(pEntry, 0, sizeof(struct mmg_entry));
      mmg_insert(&g_AnchorFree, pEntry);
    } else {
      mmg_warning("mmg_free: can not find pointer %p\n", p);
    }
  }
  free(p);
}

#endif //ENABLE_MEMORY_GUARD
