// $Id: mrtimers.h,v 1.2 2005/11/01 14:28:01 richter Exp $

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

typedef void (*mrTimerHandler_t) (int);

/* init the timer library
parameter:
  iBaseTime - base time in micro seconds
 */
int initMRTimers(int iBaseTime);

/* release the timer library
 */
int releaseMRTimers();

/* set a debug message flag
 * result: current debug flags 
 */
int setMRTimerDebugFlag(int iFlag);

/* clear a debug message flag
 * result: current debug flags 
 */
int clearMRTimerDebugFlag(int iFlag);

/* starts a timer of the specified type
 * parameter:
 *   iType - ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF
 * result:
 *   id of the timer if succeeded, negative error code if failed
 */
int startTimer(int iType);


/* set timeout and handler for a timer
 * parameter:
 *   id - id of timer returned by startTimer
 *   iTimeOutSec, iTimeOutUsec - the time out in seconds and micro seconds
 *   handler - handler to be called at time out
 * result: 0 if succeeded, negative error code if failed
 */
int setTimerHandler(int id, int iTimeOutSec, int iTimeOutUsec, mrTimerHandler_t handler);

/* stop the timer with specified id
 * parameter:
 *   id - id of the timer, returned by startTimer
 * result: 0 if succeeded, negative error code if failed
 */
int stopTimer(int id);

/* get the timer value
 * parameter:
 *   id - id of timer returned by startTimer
 *   pSec, pUsec - pointer to int to receive the full time result
 * result: time in seconds, negative error code if failed
 */
int getTimerValue(int id, int* pSec, int* pUsec);

/* get the timer value in a formated string
 * parameter:
 *   id - id of timer returned by startTimer
 * result: pointer to string, empty string if failed
 */
const char* getTimerValueString(int id);

