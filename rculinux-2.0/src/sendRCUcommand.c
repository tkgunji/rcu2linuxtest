// $Id: sendRCUcommand.c,v 1.20 2007/03/17 00:01:16 richter Exp $

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

/** @file   sendRCUcommand.c
    @author Matthias Richter
    @date   
    @brief  The @ref rcu_sh.
*/

/**
 * @defgroup rcu_sh rcu-sh
 * The RCU shell provides low level functionality to access RCU memory space.
 *
 * More specifications will follow.
 */
 
#include <stdio.h>
#include <string.h>
#include "memoryguard.h"
#include <linux/errno.h>
#include <signal.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <unistd.h>
#include "cmdInterpreter.h"
#include "dcscMsgBufferInterface.h"
#include "selectmapInterface.h"
#include "mrtimers.h"
#include "mrshellprim.h"


#define ENABLE_GETLINE
#ifdef ENABLE_GETLINE
char *Getline(const char *prompt); // Getline.c
void Gl_histadd(char *buf); // Getline.c
#endif // ENABLE_GETLINE
#define promt "enter operation (h/i/q/r/w):"


/***************************************************************************************
 * signal handler
 */


void sigquitHandler(int param) {
  if (terminateBatchProcessing()<=0)
    // no batch processing active, restore the default
    signal(SIGINT, SIG_DFL);
  else {
  }
}

extern unsigned int g_mrShellPrimDbg;

TFctMode scanModeNoError = {SCANMODE_SILENT|SCANMODE_FORCE_TERMINATION, NULL, NULL};
TFctMode scanModeMainOptions = {SCANMODE_FORCE_TERMINATION|SCANMODE_READ_ONE_CMD|SCANMODE_PERSISTENT, NULL, NULL};
TFctMode scanModeShellArgs = {SCANMODE_FORCE_TERMINATION, " ", NULL};

TArgDef shellArgs[] = {
  {"h","help",{eFctNoArg, {(void*)printHelp}},{NULL},ARGDEF_UNTERM_SHORT},
  {"info","i",{eFctNoArg, {(void*)printInfo}},{NULL},ARGDEF_UNTERM_SHORT},
  {"","",{eUnknownType, {NULL}},{NULL},0},
};

TArgDef mainOptions[] = {
  {"-d","--device=",{eConstString, {NULL}},{NULL},ARGDEF_UNTERM_LONG},
  {"-e","--encode",{eBool, {NULL}},{NULL},0},
  {"-a","--append",{eBool, {NULL}},{NULL},0},
  {"-v","--verbosity",{eInteger, {NULL}},{NULL},0},
  {NULL,"--debug",{eHex, {NULL}},{NULL},0},
  {NULL,"--mibsize",{eHex, {NULL}},{NULL},0},
  {"-mmgv","--memory-guard-verbosity",{eInteger, {NULL}},{NULL},0},
  {"-mmgf","--memory-guard-logfile",{eConstString, {NULL}},{NULL},0},
  {"-h","--help",{eFctNoArg, {(void*)printHelp}},{NULL},ARGDEF_TERMINATE},
  {"-i","--info",{eFctNoArg, {(void*)printInfo}},{NULL},ARGDEF_TERMINATE},
  {"","",{eUnknownType, {NULL}},{NULL},0},
};

TArgDef mainArgs[] = {
  {"--mrsdbg",NULL,{eHexArray, {(void*)&g_mrShellPrimDbg}},{(void*)1},0},
  {"--mrshelp",NULL,{eFctNoArg, {(void*)mrShellPrimPrintDbgFlags}},{(void*)1},ARGDEF_TERMINATE},
  {"-*","--*",{eFctInclusive, {(void*)mainOptions}},{(void*)&scanModeMainOptions},0},
  {NULL,NULL,{eBool, {NULL}},{NULL},ARGDEF_BREAK},
  {"","",{eUnknownType, {NULL}},{NULL},0},
};


int main(int argc, char** arg) 
{

  printf("execute rcu-sh\n");


  int iResult=0;
  char cmdBuffer[100]="i";
  char queryBuffer[100]="i";
  const char* pDevice=NULL; // by default the dcscRCUacces choose /dev/dcsc if this parameter is NULL
  int iPos=0;
  int iArg=1;
  if (argc-iArg>0)
    iResult=ScanArguments((const char**)arg+iArg, argc-iArg, 0, mainArgs, &scanModeNoError);
  if (iResult>=0) {
    int iArgShift=(iResult&SCANRET_MASK_PROCESSED_ARGS)>>SCANRET_BITSHIFT_PROCESSED_ARGS;
    iArg+=iArgShift;
    iPos+=(iResult&SCANRET_MASK_OFFSET_LAST_ARG)>>SCANRET_BITSHIFT_OFFSET_LAST_ARG;
    if (iArg<argc && arg[iArg]!=NULL && sizeof(arg[iArg])<=iPos) {
      iArg++;
      iPos=0;
    }

  /*     pDevice=mainOptions[0].data.pString; */
  /*     iResult=initRcuAccess(pDevice); */


    if (mrShellPrimGetData(mainOptions, "-d", (void**)&pDevice, eConstString)<0) pDevice=NULL;
    TdcscInitArguments dcscArgs;
    memset(&dcscArgs, 0, sizeof(TdcscInitArguments));
    int bool=0;
    int mmgv=kmmg_error; // verbosity of the memory guard
    const char* mmgf=NULL; // logfile for the memory guard
    unsigned int debugFlags=0;
    if (mrShellPrimGetInt(mainOptions, "-e", &bool)>=0 && bool==1) dcscArgs.flags|=DCSC_INIT_ENCODE; bool=0;
    if (mrShellPrimGetInt(mainOptions, "-a", &bool)>=0 && bool==1) dcscArgs.flags|=DCSC_INIT_APPEND; bool=0;
    if (mrShellPrimGetHex(mainOptions, "--mibsize", &dcscArgs.iMIBSize)<0) dcscArgs.iMIBSize=0;
    if ((iResult=mrShellPrimGetHex(mainOptions, "--debug", &debugFlags))>=0 && ARGPROC_EXISTS(iResult)) setDebugOptions(debugFlags);
    if ((iResult=mrShellPrimGetInt(mainOptions, "-v", &dcscArgs.iVerbosity))>=0) {
      // set the default verbosity if argument not specified
      if (ARGPROC_EXISTS(iResult)==0) {
	if (argc-iArg>0) dcscArgs.iVerbosity=0; // default verbosity for command line mode
	else dcscArgs.iVerbosity=1;             // default verbosity for interactive
      }
    }


    if (mrShellPrimGetInt(mainOptions, "--memory-guard-verbosity", &mmgv)<0) mmgv=kmmg_error;
    if (mrShellPrimGetData(mainOptions, "--memory-guard-logfile", (void**)&mmgf, eConstString)<0) mmgf=NULL;
    mmg_init(mmgf, mmgv);

    iResult=initRcuAccessExt(pDevice, &dcscArgs);


    //initSmAccess(NULL);
    signal(SIGINT, sigquitHandler);

    if (iResult>=0){
      initMRTimers(1000000);
#ifdef DCSC_SIMULATION
      startSimulation();
#endif //DCSC_SIMULATION
      if (argc-iArg>0) {
	// shell argument mode
	if (dcscArgs.iVerbosity>=2)
	  fprintf(stderr, "Command line mode: argc=%d iArg=%d %s\n", argc, iArg, *(arg+iArg));
	iResult=executeCommandArgs(argc-iArg, (const char**)arg+iArg);
	if (iResult==-EINTR) iResult=0; // -EINTR was used to indicated termination
      } else {
	// interactive mode
	while (cmdBuffer[0]!='q') {
	  strcpy(queryBuffer, cmdBuffer);// the processing of the command may alter the buffer, work on a copy
	  executeCommandLine(queryBuffer);
#ifdef DCSC_SIMULATION
	  resetSimulation();
#endif //DCSC_SIMULATION
#ifdef ENABLE_GETLINE
	  char* pInput=Getline(promt);
	  Gl_histadd(pInput);
	  strcpy(queryBuffer, pInput);
#else //!ENABLE_GETLINE
	  printf(promt);
	  gets(queryBuffer);
#endif // ENABLE_GETLINE
	  removePrecAndTrailingSpecChars(queryBuffer);
	  if (strlen(queryBuffer)==0 || queryBuffer[0]=='a' || queryBuffer[0]=='p') {
	    printf("repeat: %s\n", cmdBuffer);
	  } else {
	    strcpy(cmdBuffer, queryBuffer);
	  }
	}
      }
#ifdef DCSC_SIMULATION
      stopSimulation();
#endif //DCSC_SIMULATION
      releaseMRTimers();  // cleanup the timers
      releaseSmAccess();
      releaseRcuAccess(); // cleanup the msg buffer interface
    } else {
      fprintf(stderr, "initialisation failed (%d)\n", iResult);
    }
    mmg_exit();         // cleanup the memory guard

  } else if (iResult!=-EINTR) {
    fprintf(stderr, "argument scan failed with %d\n", iResult);
  }

  if (argc-iArg==0)
    fprintf(stderr, "bye, bye\n");
  return iResult;

}

/* ideas/requests for unit tests
- execute in shell and interactive mode 
- execute all commands
- execute command encoding
- check command line options
 */
