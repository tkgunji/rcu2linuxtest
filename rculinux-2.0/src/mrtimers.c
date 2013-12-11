// $Id: mrtimers.c,v 1.3 2006/02/13 11:16:22 richter Exp $

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

#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "memoryguard.h"
#include "mrtimers.h"

#define MRT_DBG_INIT_MSG               0x01
#define MRT_DBG_HANDLER_MSG            0x02
#define MRT_DBG_TIMER_MSG              0x04
int g_MRTdbgFlags=0;

/***************************************************************************************
 * internal functions
 */

typedef struct {
  int type;
  time_t started;
  struct itimerval value;
  int cycleCount;
} TMRSysTimer;

typedef struct {
  int TimeOutSec;
  int iTimeOutUsec;
  mrTimerHandler_t pHandler;
} TMRTimerHandler;

typedef struct {
  int id;
  int typeIndex;
  int startcycle;
  struct itimerval startvalue;
} TMRTimer;

#define MAX_SYS_TIMERS 3
#define INITIAL_TIMER_ARRAY_SIZE 10
int g_systimertypes[MAX_SYS_TIMERS]={ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF};
TMRSysTimer g_systimers[MAX_SYS_TIMERS];
TMRTimer* g_arrayTimers=NULL;
int g_arrayTimerSize=0;

int getTypeIndexFromTimerType(int iType)
{
  int index=-1;
  switch (iType) {
  case ITIMER_REAL: index=0;
    break;
  case ITIMER_VIRTUAL: index=1;
    break;
  case ITIMER_PROF: index=2;
    break;
  }
  return index;
}

int createTimer(int id, int type, TMRTimer* pTimer)
{
  int iResult=-ENOSYS;
  int iTIndex=getTypeIndexFromTimerType(type);
  if (pTimer && iTIndex>=0 && iTIndex<MAX_SYS_TIMERS) {
    memset(pTimer, 0, sizeof(TMRTimer));
    if (getitimer(g_systimers[iTIndex].type, &pTimer->startvalue)>=0) {
      pTimer->id=id;
      pTimer->typeIndex=iTIndex;
      pTimer->startcycle=g_systimers[iTIndex].cycleCount;
      if (g_MRTdbgFlags&MRT_DBG_TIMER_MSG)
	fprintf(stderr, "create timer %d: type %d, cycle %d, sec %d, usec %d\n", id, g_systimers[iTIndex].type, pTimer->startcycle, (int)pTimer->startvalue.it_value.tv_sec, (int)pTimer->startvalue.it_value.tv_usec);
      iResult=id;
    } else {
      fprintf(stderr, "getitimer failed: error %d\n", errno);
      iResult=-EBADR;
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int deleteTimer(int id) {
  int iResult=0;
  if (g_arrayTimers) {
    if (g_arrayTimers[id-1].id==id) {
      memset(&g_arrayTimers[id-1], 0, sizeof(TMRTimer));
      if (g_MRTdbgFlags&MRT_DBG_TIMER_MSG)
	fprintf(stderr, "delete timer #%d\n", id);
    } else {
      fprintf(stderr, "can not delete timer #%d, entry not found\n", id);
      iResult=-ENOENT;
    }
  } else {
    fprintf(stderr, "mrtimers internal error: not initialized\n");
    iResult=-EPERM;
  }
  return iResult;
}

/* find an existing entry or find an empty entry for a new timer
 * parameter:
 *   pID - pointer to id, if zero an empty entry is searched, otherwize the entry of timer id
 * result:
 *   pointer to timer structure
 *   id of the timer entry in pID
 */
TMRTimer* findTimerEntry(int* pID)
{
  TMRTimer* pResult=NULL;
  if (g_arrayTimers) {
    int iSearchID=0;
    if (pID!=NULL && *pID>=0) {
      iSearchID=*pID;
    }
    int i=0;
    for (i=0; i<g_arrayTimerSize && pResult==NULL; i++) {
      if (g_arrayTimers[i].id==iSearchID) {
	pResult=&g_arrayTimers[i];
	if (pID!=NULL && iSearchID==0) *pID=i+1;
	break;
      }
    }
    if (pResult==0 && iSearchID==0) {
      fprintf(stderr, "mrtimers limited to %d timers at a time\n", INITIAL_TIMER_ARRAY_SIZE);
      //iResult=-E2BIG;
    }
  } else {
    fprintf(stderr, "mrtimers internal error: not initialized\n");
    //iResult=-EPERM;
  }
  return pResult;
}

/***************************************************************************************
 * signal handlers
 */
void sigalarmHandler(int param) {
  g_systimers[0].cycleCount++;
  if (g_MRTdbgFlags&MRT_DBG_HANDLER_MSG)
    fprintf(stderr, "sigalarmHandler: %d param=%d\n", g_systimers[0].cycleCount, param);
}

void sigvtalarmHandler(int param) {
  g_systimers[1].cycleCount++;
  if (g_MRTdbgFlags&MRT_DBG_HANDLER_MSG)
    fprintf(stderr, "sigvtalarmHandler: %d param=%d\n", g_systimers[1].cycleCount, param);
}

void sigprofHandler(int param) {
  g_systimers[2].cycleCount++;
  if (g_MRTdbgFlags&MRT_DBG_HANDLER_MSG)
    fprintf(stderr, "sigprofHandler: %d param=%d\n", g_systimers[2].cycleCount, param);
}

typedef void (*sighandler_t)(int);
int g_signals[MAX_SYS_TIMERS]={SIGALRM, SIGVTALRM, SIGPROF};
sighandler_t g_signalHandler[MAX_SYS_TIMERS]={sigalarmHandler, sigvtalarmHandler, sigprofHandler};

/***************************************************************************************
 * interface functions
 */
int initMRTimers(int iBaseTime)
{
  int iResult=0;
  int i=0;
  struct itimerval value;
  if (iBaseTime==0) iBaseTime=1000000;
  if (g_MRTdbgFlags&MRT_DBG_INIT_MSG)
    fprintf(stderr, "initMRTimers: iBaseTime=%d\n", iBaseTime);
  for (i=0; i<MAX_SYS_TIMERS && iResult>=0; i++) {
    // register handlers for timer signals
    if (signal(g_signals[i], g_signalHandler[i])!=SIG_ERR) {
      // check timer
      if ((iResult=getitimer(g_systimertypes[i], &value))>=0) {
	if (g_MRTdbgFlags&MRT_DBG_INIT_MSG)
	  fprintf(stderr, "initMRTimers: system timer %d: value=%d interval=%d\n", g_systimertypes[i], (int)value.it_value.tv_usec, (int)value.it_interval.tv_usec);
	value.it_value.tv_usec=iBaseTime%1000000;
	value.it_value.tv_sec=iBaseTime/1000000;
	value.it_interval.tv_usec=value.it_value.tv_usec;
	value.it_interval.tv_sec=value.it_value.tv_sec;
	// set timer value
	if ((iResult=setitimer(g_systimertypes[i], &value, NULL))>=0) {
	  // init systimer struct
	  g_systimers[i].type=g_systimertypes[i];
	  time(&(g_systimers[i].started));
	  memcpy(&(g_systimers[i].value), &value, sizeof(struct itimerval));
	  g_systimers[i].cycleCount=0;
	} else {
	  fprintf(stderr, "initMRTimers: can not set system timer %d, error %d\n", g_systimertypes[i], iResult);
	  iResult=-EPERM;
	}
      } else {
	fprintf(stderr, "initMRTimers: can not get status of system timer %d, error %d\n", g_systimertypes[i], iResult);
	iResult=-EPERM;
      }
    } else {
      fprintf(stderr, "can not register signal handler for signal %d\n", g_systimertypes[i]);
      iResult=-EPERM;
    }
  }
  if (iResult>=0) {
    g_arrayTimerSize=10;
    g_arrayTimers=(TMRTimer*)malloc(g_arrayTimerSize*sizeof(TMRTimer));
    if (g_arrayTimers!=NULL) {
      memset(g_arrayTimers, 0, g_arrayTimerSize*sizeof(TMRTimer));
    } else {
      g_arrayTimerSize=0;
      iResult=-ENOMEM;
    }
  }
  return iResult;
}

int releaseMRTimers()
{
  int iResult=0;
  if (g_arrayTimers!=NULL) {
    TMRTimer* array=g_arrayTimers;
    g_arrayTimerSize=0;
    g_arrayTimers=NULL;
    free(array);
  }
  return iResult;
}

int setMRTimerDebugFlag(int iFlag)
{
  g_MRTdbgFlags|=iFlag;
  return g_MRTdbgFlags;
}

int clearMRTimerDebugFlag(int iFlag)
{
  g_MRTdbgFlags&=~iFlag;
  return g_MRTdbgFlags;
}

int startTimer(int iType)
{
  int iResult=0;
  int iTimerID=0;
  TMRTimer* pTimer=findTimerEntry(&iTimerID);
  iResult=createTimer(iTimerID, iType, pTimer);
  if (pTimer) {
  } else {
    iResult=-1;
  }
  return iResult;
}

int setTimerHandler(int id, int iTimeOutSec, int iTimeOutUsec, mrTimerHandler_t handler)
{
  int iResult=-ENOSYS;
  return iResult;
}

int stopTimer(int id)
{
  int iResult=deleteTimer(id);
  return iResult;
}

int getTimerValue(int id, int* pSec, int* pUsec)
{
  int iResult=0;
  TMRTimer* pTimer=findTimerEntry(&id);
  if (pTimer) {
    struct itimerval currval;
    if ((iResult=getitimer(g_systimers[pTimer->typeIndex].type, &currval))>=0) {
      if (g_MRTdbgFlags&MRT_DBG_TIMER_MSG)
	fprintf(stderr, "calculating time value for timer %d: systimer cycle %d, sec %d, usec %d\n", (int)id, g_systimers[pTimer->typeIndex].cycleCount, (int)currval.it_value.tv_sec, (int)currval.it_value.tv_usec);
      int iCycles=g_systimers[pTimer->typeIndex].cycleCount-pTimer->startcycle;
      int iTimeUsec=0;
      int iTimeSec=0;
      iTimeUsec=iCycles*pTimer->startvalue.it_interval.tv_usec+pTimer->startvalue.it_value.tv_usec-currval.it_value.tv_usec;
      iTimeSec=iCycles*pTimer->startvalue.it_interval.tv_sec+pTimer->startvalue.it_value.tv_sec-currval.it_value.tv_sec;
      while (iTimeUsec<0) {
	iTimeUsec+=1000000;
	iTimeSec--;
      }
      while (iTimeUsec>1000000) {
	iTimeUsec-=1000000;
	iTimeSec++;
      }
      if (pSec!=NULL && pUsec!=NULL) {
	*pSec=iTimeSec;
	*pUsec=iTimeUsec;
      }
      iResult=iTimeSec;
    } else {
      fprintf(stderr, "mrtimers: can not get current time of systemtimer %d\n", g_systimers[pTimer->typeIndex].type);
    }
  } else {
    fprintf(stderr, "mrtimers: can not get entry for timer %d\n", id);
    iResult=-ENOENT;
  }
  return iResult;
}

char g_strTime[20];
const char* getTimerValueString(int id)
{
  g_strTime[0]=0;
  int iSec=0;
  int iUsec=0;
  if (getTimerValue(id, &iSec, &iUsec)>=0) {
    sprintf(g_strTime, "%d.%d", iSec, iUsec);
  }
  return (const char*)g_strTime;
}
