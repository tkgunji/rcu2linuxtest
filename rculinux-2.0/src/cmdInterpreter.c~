// $Id: cmdInterpreter.c,v 1.26 2007/03/17 00:01:16 richter Exp $

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

#include <stdio.h>
#include <string.h>
#include "memoryguard.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "cmdInterpreter.h"
#include "dcscMsgBufferInterface.h"
#include "selectmapInterface.h"
//#include "rcuMemtest.h"
#include "mrtimers.h"
//#include "FileReader.h"
#include "mrshellprim.h"

#define PRINT_COMMAND_BUFFER_CTR_STRING   "pcb (print command buffer)"
#define PRINT_RESULT_BUFFER_CTR_STRING    "prb (print result buffer)"
#define CHECK_COMMAND_BUFFER_CTR_STRING   "c   (check command buffer)"
#define IGNORE_BUFFER_CHECK_CTR_STRING    "i   (ignore buffer check)"
#define PRINT_CTRL_REGISTER_CTR_STRING    "pcr (print control register)"
#define PRINT_COMMAND_RESULT_CTR_STRING   "r   (print command result)"
#define CMD_STRING_PROFILE                "profile"
#define CMD_STRING_MRT_DEBUG              "timerdbg"

/*
#define CA_ADDRESS_VALID       0x01
#define CA_DATA_VALID          0x02
#define CA_FILE_NAME_VALID     0x04
#define CA_NOF_WORDS_VALID     0x08

#define FF_BINARY_8            0x01
#define FF_BINARY_16           0x02
#define FF_BINARY_32           0x03
#define FF_CHARACTER           0x04
*/

int g_profiling=0;

/*
typedef struct {
  unsigned int uiAddress;
  unsigned int uiData;
  int iNofWords;
  char* pFileName;
  int iFileFormatFlags;
  unsigned int valid;
} TCommandArgs;
*/

enum ConditionTypes {
  e_fail = 0,
  e_continue,
};

typedef struct {
  int type;
  unsigned int bitMask;
  unsigned int pattern;
} TCondition;

void* ConvertASCII2Bin(char* pASCIIBuffer, int* piBufferSize, int iWordSize)
{
  void* pResult=NULL;
  int iSize=0;
  int iNofWords=0;
  if (pASCIIBuffer && iWordSize>0 && iWordSize<=4) {
    int i=-1;
    int j=0;
    __u32 data=0;
    char cSeparators[]={'\n','\t',' ', 0}; // the array has to bee 0 terminated
    int k=0;
    do {
      // data words can be separated by \n, \t or SPC
      for (k=0; cSeparators[k]!=0 && cSeparators[k]!=pASCIIBuffer[j]; k++);
      if (cSeparators[k]!=0 || pASCIIBuffer[j]==0) {
	if (i>=0) {
	  // this is the end of an argument
	  iNofWords++;
	}
	i=-1;
      } else if (i<0) {
	// this is the start of an argument
	i=j;
      }
      //printf("%d : %d\n",j, pASCIIBuffer[j]); 
    } while (pASCIIBuffer[j++]!=0);
    if (setDebugOptionFlag(0)&DBG_FILE_CONVERT)
      fprintf(stderr, "ConvertASCII2Bin: %d word(s)\n", iNofWords);
    if (iNofWords>0) {
      iSize=iNofWords*iWordSize;
      pResult=malloc(iSize);
      if (pResult) {
	iNofWords=0;
	int iWarning=0;
	i=-1; j=0;
	do {
	  // data words can be separated by \n, \t or SPC
	  for (k=0; cSeparators[k]!=0 && cSeparators[k]!=pASCIIBuffer[j]; k++);
	  if (cSeparators[k]!=0 || pASCIIBuffer[j]==0) {
	    if (setDebugOptionFlag(0)&DBG_FILE_CONVERT)
	      fprintf(stderr, "i=%d, j=%d\n", i, j);
	    if (i>=0) {
	      data=0;
	      if (sscanf(&pASCIIBuffer[i], " 0x%x", &data)==1 || sscanf(&pASCIIBuffer[i], " %x", &data)==1) {
		if (setDebugOptionFlag(0)&DBG_FILE_CONVERT)
		  fprintf(stderr, "%d: %#x\n",iNofWords, data);
		__u32 mask=0xffffffff;
		if (iWordSize==1) mask=0x0ff;
		else if (iWordSize==2) mask=0x0ffff;
		if (iWarning==0 && (data&mask)!=data) {
		  fprintf(stderr, "ConvertASCII2Bin: data word(s) truncated, exceed(s) bit width of %d\n", iWordSize*8);
		  iWarning++;
		}
		data&=mask;
		memcpy(((char*)pResult)+iWordSize*iNofWords++, &data, iWordSize);
	      }
	    }
	    i=-1;
	  } else if (i<0) {
	    i=j;
	  }
	} while (pASCIIBuffer[j++]!=0);
      }
    } else {
      fprintf(stderr, "ConvertASCII2Bin warning: file doesnt contain meaningful data \n");
    }
  }
  if (iSize>0 && iSize!=iNofWords*iWordSize) {
    fprintf(stderr, "ConvertASCII2Bin error: parse error, fewer words than expected\n");
    iSize=0;
  }
  if (iSize==0 && pResult!=NULL) {
    free(pResult);
    pResult=NULL;
  }
  if (piBufferSize)
    *piBufferSize=iSize;
  return pResult;
}

int printInfo()
{
  printf("\n**********************************\nrcu bus easy read/write access\n\n");
  printf("version %s (compiled "__DATE__", "__TIME__")\n", VERSION);
  printf("Matthias Richter, University of Bergen\n");
  printf("Matthias.Richter@ift.uib.no\n\n");
  return 0;
}

void printDebugHelp(int iLevel)
{
  printf("  debug options: logical or of bits\n");
  /*
    printf("  <+/-> <%s>\t\t turn on/off printing of command buffer\n", PRINT_COMMAND_BUFFER_CTR_STRING);
    printf("  <+/-> <%s>\t\t turn on/off printing of result buffer\n", PRINT_RESULT_BUFFER_CTR_STRING);
    printf("  <+/-> <%s>\t\t turn on/off check (read back) of command buffer before execution\n", CHECK_COMMAND_BUFFER_CTR_STRING);
    printf("  <+/-> <%s>\t\t turn on/off ignoring the result of the test\n", IGNORE_BUFFER_CHECK_CTR_STRING);
    printf("  <+/-> <%s>\t\t turn on/off printing the status of the control register\n", PRINT_CTRL_REGISTER_CTR_STRING);
  */
  printf("  <+/- 0x%03x>\t turn on/off printing of command buffer\n", PRINT_COMMAND_BUFFER);
  printf("  <+/- 0x%03x>\t turn on/off printing of result buffer\n", PRINT_RESULT_BUFFER);
  printf("  <+/- 0x%03x>\t turn on/off check (read back) of cmd buffer before execution\n", CHECK_COMMAND_BUFFER);
  printf("  <+/- 0x%03x>\t turn on/off ignoring the result of the test\n", IGNORE_BUFFER_CHECK);
  printf("  <+/- 0x%03x>\t turn on/off printing of access to the control registers\n", PRINT_REGISTER_ACCESS);
  printf("  <+/- 0x%03x>\t turn on/off print the result of the command\n", PRINT_COMMAND_RESULT);
  if (iLevel>0) printf("  <+/- 0x%03x>\t turn on/off print debug info on splitting in multiple operations\n", PRINT_SPLIT_DEBUG);
  if (iLevel>0) printf("  <+/- 0x%03x>\t turn on/off print debug info on file conversion\n", DBG_FILE_CONVERT);
  printf("  <+/- 0x%03x>\t turn on/off translate result human readable\n", PRINT_RESULT_HUMAN_READABLE);
  if (iLevel>0) printf("  <+/- 0x%03x>\t turn on/off print debug info on argument conversion\n", DBG_ARGUMENT_CONVERT);
  if (iLevel>0) printf("  <+/- 0x%03x>\t turn on/off print debug info on the 'check' command\n", DBG_CHECK_COMMAND);
  printf("  - turn off all messages\n");
  printf("  + turn on default messages (%#x)\n", DBG_DEFAULT);
}

int printHelp()
{
  printf("  all parameters are hex values preceeded by a '0x' or decimal numbers\n");
  printf("  some of the commands have a short version, in most cases the first letter\n");
  printf("  quit                   : q\n");
  printf("  info                   : i\n");
  printf("  driver info            : d\n");
  printf("  repeat previous command: p\n");
  printf("  single read rcu bus memory   : r[ead] 0x<address> (e.g.: r 0x7000)\n");
  printf("  multiple read rcu bus memory : r[ead] 0x<address> <dec no> (e.g.: r 0x7000 16)\n");
  printf("                                 see 'hr\' for details and further options\n");
  printf("  single write rcu bus memory  : w[rite] [-s,--swap] 0x<address> 0x<data>\n");
  printf("                                 (e.g.: w 0x6800 0x34)\n");
  printf("  multiple write with const    : w[rite] [-s,--swap] 0x<address> <dec no> 0x<data>\n");
  printf("                                 (e.g.: w 0x6800 12 0x0a)\n");
  printf("  write file to rcu bus memory : w[rite] [-s,--swap] 0x<address> <fspec> 'filepath' <count> \n");
  printf("     optional format spec: -b4(default) -b2 -b1 binary 32,16,8 bit\n");
  printf("                           -b10 binary compressed 10 bit, -c ascii\n");
  printf("     optional number after the filepath specifies count for partially write\n");
  printf("     e.g.: w 0x6800 -c 'pedestal.dat' 512,  w 0x7000 'pgm.dat'\n");
  printf("  sending a single command     : c 0x<address> (translated to w <address> 0x0\n");
  printf("  check status of mem location : ? 0x<address> [[c,f] 0x<bitmask> 0x<pattern>]\n");
  printf("                                               [t n s(ec)/u(sec)]\n");
  printf("  batch processing             : b[atch] 'filepath' [[-l] <count>,-i]\n");
  printf("                                 see \'hb\' for details and further options\n");
  printf("  wait command                 : wait <n> s(ec)/u(sec)\n");
  printf("  log message                  : e[cho] [-o,-a <filename>] <message>\n");
  printf("     the message can contain a \'-t\' specifier to print a timestamp\n\n");
  printf("  flash memory commands        : flash <cmd> (try \'flash help\')\n");
  printf("  selectmap commands           : sm[selectmap] <cmd> (try \'sm help\')\n");
  printf("  firmware commands            : fw[firmware] <cmd> (try \'fw help\')\n");
  printf("  read bus control register    : rcr[read-ctrlreg]\n");
  //printf("  check rcu bus memory: m 0x<address> 0x<size in byte>\n");
  //printf("  read from file\n");
  printf("  -/+ profile: switch on/off profiling\n");
  printf("  - turn off all messages\n");
  printf("  + turn on default messages\n");
  printf("  hd debug message info\n");
  printf("  hr detailed help for the read-command\n");
  return 0;
}

void printReadHelp() {
  printf("  the \'read\' command:\n");
  printf("  r[ead] 0x<address> [number of words] [-f <format>] [-o,a <file name>]\n");
  printf("   number of words optional for multiple read\n");
  printf("   format of the output: the format string can be an arbitrary string \n");
  printf("     with specifiers similar to the \'printf\' standard\n");
  printf("     %%a - print the address\n");
  printf("     %%x - hexadecimal\n");
  printf("     %%d - decimal\n");
  printf("     %%f - float\n");
  printf("     %%b - binary (%%b1, %%b2, %%b4=%%b: one, two and four byte words)\n");
  printf("     between the \'%%\' character and the specifier similar format options\n");
  printf("     as for printf apply, \\n is interpreted as newline \n");
  printf("     important note: enclose the string into quotes, if it contains blanks\n");
  printf("       examples: \'%%a: %%#x\' (default), \'%%3.2f\', \'%%d\\n%%d\\n\'\n");
  printf("     specifiers can be followd by a bit mask and coefficients for a\n");
  printf("     linear conversion enclosed in \'[]\': [&0x<mask>*<factor>+/-<offset>]\n");
  printf("     e.g. [&0xff*8 -5] means masking of the 8 LSBs, multiplication by 8 and\n");
  printf("     subtraction by 5, other examples [*-1 +6] (multiplication by -1)\n");
  printf("   redirection of the output: -o creates a new file, -a appends to a file\n");
}

void printBatchProcHelp() {
  printf("  the \'batch\' command:\n");
  printf("  reads commands from a file and executes them sequencially\n");
  printf("  b[atch] 'filepath' [[-l] <count>,-i] [-v <level>,-s] [-w <n> s(ec)/u(sec)]\n");
  printf("   -l specifies number of loops, -i infinite (terminated by CTRL-C)\n");
  printf("   -v verbosity level (0,1,2), -s silent (-v 0)\n");
  printf("   -w wait n sec/usec between the cycles\n");
}

/* reads the next two arguments and translates it into the sec/usec
 */
int readTime(const char** arrayArg, int iNofArgs, int *pSec, int* pMusec)
{
  int iResult=0;
  if (arrayArg!=NULL && iNofArgs>=2 && pSec && pMusec) {
    int iSec=0;
    int iMusec=0;
    float fVal=0;
    if (sscanf(arrayArg[0], " %f", &fVal)==1) {
      if (*arrayArg[1]=='s' || *arrayArg[1]=='S') {
	iSec=fVal;
	iMusec=(fVal-iSec)*1000000;
      } else if (*arrayArg[1]=='u' || *arrayArg[1]=='U') {
	iSec=0;
	iMusec=fVal;
      } else {
	fprintf(stderr, "invalid time unit (%s)\n", arrayArg[1]);
	iResult=-EINVAL;
      }
      if (iResult>=0) {
	*pSec=iSec;
	*pMusec=iMusec;
      }
    } else {
      iResult=-EINVAL;
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int waitCondition(const char** arrayArg, int iNofArgs)
{
  int iResult=0;
  if (iNofArgs>=4 && arrayArg) {
    int i=0;
    TCondition* arrayCond=NULL;
    int iInitialArraySize=0;
    int iNofConditions=0;
    static int iDefaultTimeOut=500000;
    int iTimeoutMusec=iDefaultTimeOut;
    int iTimeoutSec=0;
    int iSec=0, iMusec=0;
    unsigned int address=0;
    // the first argument is supposed to be the address to check
    if ((iResult=getHexNumberFromArg(arrayArg[i], &address, 1))>=0) {
      startSimulation();
      i++;
      for (; i<iNofArgs && iResult>=0; i++) {
	const char* pArgSwitch=arrayArg[i];
	while (pArgSwitch!=NULL && *pArgSwitch!=0 && iResult>=0) {
	  int iType=e_continue;
	  switch (*pArgSwitch) {
	  case 'f':
	    iType=e_fail;
	    //fall through is intended
	  case 'c':
	    // read the wait condition, format: f,c 0xbitMask 0xpattern
	    // fails or continues if the bitMask matches the pattern
	    if (iNofArgs-i>2) {
	      if (iNofConditions>=iInitialArraySize) {
		iInitialArraySize+=5;
		TCondition* arrayNew=(TCondition*)malloc(iInitialArraySize*sizeof(TCondition));
		if (arrayNew) {
		  memset(arrayNew, 0, iInitialArraySize*sizeof(TCondition));
		  if (arrayCond) {
		    memcpy(arrayNew, arrayCond, iNofConditions*sizeof(TCondition));
		    free(arrayCond);
		  }
		  arrayCond=arrayNew;
		} else {
		  iResult=-ENOMEM;
		  fprintf(stderr, "waitCondition: memory allocation failed\n");
		}
	      }
	      if (iResult>=0 && arrayCond) {
		if ((iResult=getHexNumberFromArg(arrayArg[i+1], &(arrayCond[iNofConditions].bitMask), 1))>=0) {
		  if ((iResult=getHexNumberFromArg(arrayArg[i+2], &(arrayCond[iNofConditions].pattern), 1))>=0) {
		    arrayCond[iNofConditions].type=iType;
		    if (setDebugOptionFlag(0)&DBG_CHECK_COMMAND) {
		      fprintf(stderr, "condition added: type=%d mask=%#x pattern=%#x\n", arrayCond[iNofConditions].type, arrayCond[iNofConditions].bitMask, arrayCond[iNofConditions].pattern); 
		    }
		    iNofConditions++;
		    i+=2; // skip 2 arguments since they already has been read
		  }
		}
	      }
	    } else {
	      fprintf(stderr, "waitCondition: missing parameter for condition\n");
	      iResult=-EINVAL;
	    }
	    pArgSwitch=NULL;
	    break;
	  case 't':
	    // read the timeout parameter
	    if (iNofArgs-i>2) {
	      if ((iResult=readTime(arrayArg+i+1, 2, &iTimeoutSec, &iTimeoutMusec))>=0) {
		if (setDebugOptionFlag(0)&DBG_CHECK_COMMAND) {
		  fprintf(stderr, "set time out: %d sec %d usec\n", iTimeoutSec, iTimeoutMusec);
		}
		i+=2; // skip one argument since this has already been read
	      } else {
		iTimeoutMusec=iDefaultTimeOut;
		iTimeoutSec=0;
		fprintf(stderr, "waitCondition: wrong format for timeout\n");
		iResult=-EINVAL;
	      }
	    } else {
	      fprintf(stderr, "waitCondition: missing value for timeout\n");
	      iResult=-EINVAL;
	    }
	    pArgSwitch=NULL;
	    break;
	  case '-':
	    // test the next letter during the next cycle (increment below!), so both 'c' and '-c' are accepted
	    break;
	  default:
	    if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
	      fprintf(stderr, "waitCondition: wrong argument %s (%d)\n", arrayArg[i], i);
	    }
	    iResult=-EINVAL;
	    pArgSwitch=NULL;
	  }
	  if (pArgSwitch) {
	    pArgSwitch++;
	  }
	}
      }

      // check cycle
      if (iNofConditions>0) {
	int iCount=1;
	unsigned int data=0;
	int iTimer=startTimer(0);
	if (iTimer<=0) {
	  fprintf(stderr, "waitCondition: can not open timer\n");
	}
	while (iResult==0) {
	  if ((iResult=rcuSingleRead(address, &data))>=0) {
	    iResult=0;
	    for (i=0; i<iNofConditions && iResult==0; i++) {
	      if (setDebugOptionFlag(0)&DBG_CHECK_COMMAND)
		fprintf(stderr, "checking condition #%d: type=%d, mask=%#x pattern=%#x ", i, arrayCond[i].type, arrayCond[i].bitMask, arrayCond[i].pattern);
	      if ((data&arrayCond[i].bitMask)==arrayCond[i].pattern) {
		if (arrayCond[i].type==e_fail) {
		  if (setDebugOptionFlag(0)&DBG_CHECK_COMMAND) {
		    fprintf(stderr, "condition \'fail\' matched");
		  }
		  iResult=-EIO;
		}
		else if (arrayCond[i].type==e_continue) {
		  if (setDebugOptionFlag(0)&DBG_CHECK_COMMAND) {
		    fprintf(stderr, "condition \'continue\' matched");
		  }
		  iResult=1;
		}
		else {
		  fprintf(stderr, "waitCondition internal error: invalid condition type");
		  iResult=-EBADMSG;
		}
	      } else {
		iResult=0; // keep the loop going
	      }
	      if (setDebugOptionFlag(0)&DBG_CHECK_COMMAND)
		fprintf(stderr, "\n");
	    }
	  }
	  // timeout condition, later a clock should be implemented
	  if (iResult==0) {
	    if (iTimer) {
	      iResult=getTimerValue(iTimer, &iSec, &iMusec);
	      if (iResult>0) iResult=0; // getTimerValue returns time in seconds, discard
	      if (setDebugOptionFlag(0)&DBG_CHECK_COMMAND) {
		fprintf(stderr, "checking timer: curr sec %d, curr usec %d\n", iSec, iMusec);
	      }
	      if (iResult>=0 && iSec>=iTimeoutSec && iMusec>=iTimeoutMusec) {
		iResult=-ETIMEDOUT;
	      }
	    } else {
	      if (++iCount>=iTimeoutMusec && iTimeoutMusec>0) {
		iResult=-ETIMEDOUT;
	      }
	    }
	    if (iResult==-ETIMEDOUT && setDebugOptionFlag(0)&DBG_CHECK_COMMAND)
	      fprintf(stderr, "time out \n");
	  } else {
	    if (setDebugOptionFlag(0)&DBG_CHECK_COMMAND)
	      fprintf(stderr, "terminate at iResult=%d\n", iResult);
	  }
	}
	if (iTimer>=0) stopTimer(iTimer);
      } else {
	fprintf(stderr, "could not find any condition to check, aborting\n");
      }
      if (arrayCond) {
	free(arrayCond);
      }
      stopSimulation();
    } else {
      fprintf(stderr, "waitCondition: invalid argument %p %d\n", arrayArg, iNofArgs);
      iResult=-EINVAL;
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

/* translate a command array and read data from file into a buffer, the buffer must be released by the caller
parameter:
  arrayArg - array of char pointers interpreted as command parameters
  nofArgs - size of the array
  ppBuffer - target to receive the data pointer
result:
  # 32 bit words in the data buffer if succeeded, data pointer in ppBuffer 
  <0 if failed
*/
int buildDataBufferFromFile(const char** arrayArg, int nofArgs, __u32** ppBuffer, int* pDataFormat, int iMode)
{
  int iResult=0;
  if (arrayArg!=NULL && nofArgs>=1 && ppBuffer!=NULL && pDataFormat!=NULL) {
    *ppBuffer=NULL;
    int iArg=0;
    int iDataFormat=4;
    int iNofWords=0;

    // the default data format
    if (iMode==2 || iMode==3) iDataFormat=2; // flash default 16 bit
    else iDataFormat=4; // msgbuffer default default, 32 bit

    // search for modifiers
    if (*(arrayArg[iArg])=='-') {
      // interprete the file modifier
      if (strcmp(arrayArg[iArg], "-c")==0) // char 
	iDataFormat=0;
      else if (strcmp(arrayArg[iArg], "-b10")==0) // 10 bit compressed
	iDataFormat=3;
      else if (strcmp(arrayArg[iArg], "-b1")==0) // 8 bit
	iDataFormat=1;
      else if (strcmp(arrayArg[iArg], "-b2")==0) // 16 bit
	iDataFormat=2;
      else if (strcmp(arrayArg[iArg], "-b4")==0) // 32 bit
	iDataFormat=4;
      else if (strcmp(arrayArg[iArg], "-b")==0) {// binary 
	if (iMode==2 || iMode==3)
	  iDataFormat=2; // flash default 16 bit
	else
	  iDataFormat=4; // msgbuffer default default, 32 bit
      }
      else {
	fprintf(stderr, "invalid file modifier %s\n", arrayArg[iArg]);
	iResult=-EINVAL;
      }
      if (setDebugOptionFlag(0)&DBG_FILE_CONVERT)
	fprintf(stderr, "got file modifier: data format %d\n", iDataFormat);
      iArg++;
    }
    if (iResult>=0 && nofArgs>iArg) {
      // search for the file name, try to open a file and read the data into a buffer
      int file=open(arrayArg[iArg], O_RDONLY);
      if (file>0) {
	int iFileSize=0;
	int iBufferSize=0;
	__u32* pData=NULL;
	char* pASCIIBuffer=NULL;
	if ((iFileSize=lseek(file, 0, SEEK_END))>=0) {
	  //printf("file size %d\n", iFileSize);
	  int iFileBufferSize=iFileSize+1; // the buffer must be at least one char bigger to ensure 0 termination
	  pASCIIBuffer=(char*)malloc(iFileBufferSize);
	  if (pASCIIBuffer) {
	    memset(pASCIIBuffer, 0, iFileBufferSize);
	    if (lseek(file, 0, SEEK_SET)==0) {
	      if (read(file, pASCIIBuffer, iFileSize)==iFileSize) {
		if (iDataFormat==0) {
		  // this is an ascii file
		  if ((pData=ConvertASCII2Bin(pASCIIBuffer, &iBufferSize, 4))!=NULL) {
		    free(pASCIIBuffer);
		    pASCIIBuffer=NULL;
		    iDataFormat=4;
		    iNofWords=iBufferSize/iDataFormat;
		    if (setDebugOptionFlag(0)&DBG_FILE_CONVERT)
		      printf("buffer with %d byte char data converted to %d words of size %d\n", iFileSize, iNofWords, iDataFormat);
		  } else {
		    if (setDebugOptionFlag(0)&DBG_FILE_CONVERT)
		      printf("conversion of char buffer failed\n");
		    iNofWords=0;
		  }
		} else {
		  // this is a binary file
		  pData=(__u32*)pASCIIBuffer;
		  iBufferSize=iFileSize;
		  if (iDataFormat==3) {
		    iNofWords=(iFileSize/4)*3 + (iFileSize%4)*8/10;
		  } else {
		    iNofWords=iFileSize/iDataFormat;
		  }
		}
		if (setDebugOptionFlag(0)&DBG_FILE_CONVERT) {
		  printf("%d word(s) of size %d read from file %s\n", iNofWords, iDataFormat, arrayArg[iArg]);
		}
		if (iArg+1<nofArgs) {
		  int iMaxWords=0;
		  //fprintf(stderr, "scanning count\n");
		  if (getDecNumberFromArg(arrayArg[iArg+1], &iMaxWords, 0)>=0) {
		    if (iMaxWords<iNofWords) {
		      iNofWords=iMaxWords;
		      if (setDebugOptionFlag(0)&DBG_FILE_CONVERT) {
			printf("%d word(s) designated for writing\n", iNofWords);
		      }
		    }
		  } else {
		    printf("enter a decimal number after the file name! parameter %s ignored\n", arrayArg[iArg+1]);
		  }
		}
		if (iNofWords>0) {
		  iResult=iNofWords;
		  *ppBuffer=pData;
		  *pDataFormat=iDataFormat;
		} else {
		  free(pData);
		  pData=NULL;
		}
	      } else {
		printf("error reading data from file %s\n", arrayArg[iArg]);
	      }
	    } else {
	      printf("error seeking to beginning of file %s\n", arrayArg[iArg]);
	    }
	  } else {
	    printf("internal error: memory allocation failed\n");
	  }
	} else {
	  printf("can not get size of file %s\n", arrayArg[iArg]);
	}
	close(file); 
      } else {
	iResult=-ENOENT;
	fprintf(stderr, "can not open file %s\n", arrayArg[iArg]);
      }
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

/**
 * execute the write command
 * there are 3 argument formats, the argument array is scanned and interpreted according to: 
 *  1. address data
 *  2. address count data
 *  3. address [file options] filename count
 * @param iMode function modes<br>
 *  0 normal msgbuffer write function<br>
 *  1 compare of the MIB, debugging feature to compare MIB with a file<br>
 *  2 flash write mode<br>
 *  3 flash verify<br>
 *  8 selectmap register write<br>
 */
int execWriteCmd(const char** arrayArg, int iNofArgs, int iMode)
{
  int iResult=0;
  int iVerbosity=0;
  //fprintf(stderr, "write command, %d arguments, mode %d\n", iNofArgs, iMode);

  /* the MIB compare mode requires at least the filename and optional specifiers */
  /* the MIB write and flash write modes require at least address and data */
  if ((iMode==1 && iNofArgs>=1) || iNofArgs>=2) { 
    int iTimer=0;
    if (g_profiling>0) {
      iTimer=startTimer(0); // timer of type ITIMER_REAL
    }
    unsigned int address=0;
    unsigned int data=0;
    __u32* pDataBuffer=NULL;
    int iDataFormat=0;
    int iNofWords=0;
    int iPos=0;

    int iEndianess=1; // 1 little, -1 big -> bytes get swapped 
    /* scan for the additional swap option   */ 
    if (iMode!=1 && iMode!=3 && iMode!=8 &&
	(strcmp(arrayArg[iPos], "--swap")==0 || strcmp(arrayArg[iPos], "-s")==0)) {
      iPos++;
      iEndianess=-1;
    }

    if (iMode==8 && (iResult=getDecNumberFromArg(arrayArg[iPos], &address, 0))>=0) {
      iPos++;
      if (getHexNumberFromArg(arrayArg[iPos], &data, 0)>=0) {
	// this is a selectmap register write operation
	// format: <reg no> 0x<data>
	iPos++;
	iNofWords=1;
	iDataFormat=4;
	iResult=smRegisterWrite(address, data);
      }
    } else if (iMode==1 || (iResult=getHexNumberFromArg(arrayArg[iPos++], &address, 0))>=0) {
    /* just skip the address scan for the MIB compare mode otherwize the address is always
       the first argument */
      //fprintf(stderr, "address %#x\n", address);
     
      if (iMode!=1 && getHexNumberFromArg(arrayArg[iPos], &data, 0)>=0) {
	// this is a single write operation
	// format: 0x<address> 0x<data>
	iPos++;
	iNofWords=1;
	iDataFormat=4;
	// only the rcu write has a dedicated function for one-word writing
	// the branch where to get depends on the pDataBuffer variable
	// set this to the data variable
	if (iMode==3 || iMode==2) pDataBuffer=&data; 
      } else if (iMode!=1 && getDecNumberFromArg(arrayArg[iPos], &iNofWords, 0)>=0) {
	iPos++;
	if (iNofWords==0) {
	  // this is a single write of data 0x0 abbreviated with 0
	  // format: 0x<address> 0
	  data=0;
	  iNofWords=1;
	} else if (iNofWords>0) {
	  // multiple write operation, one date word to several addresses
	  // get the data word and fill a buffer
	  // format: 0x<address> n 0x<data>
	  if (iNofArgs>iPos && getHexNumberFromArg(arrayArg[iPos], &data, 0)>=0) {
	    int iBufferSize=iNofWords*sizeof(__u32);
	    pDataBuffer=(__u32*)malloc(iBufferSize);
	    if (pDataBuffer) {
	      __u32* pTemp=pDataBuffer;
	      while (pTemp<pDataBuffer+iNofWords) {
		*pTemp=data;
		pTemp++;
	      }
	      iDataFormat=4;
	    } else {
	      iResult=-ENOMEM;
	      printf("internal error: memory allocation failed\n");
	    }
	  } else {
	    iResult=-EINVAL;
	  }
	}
      } else {
	// writing data from file
	// format: 0x<address> [filespecifier] <filename> [n]
	iResult=buildDataBufferFromFile(arrayArg+iPos, iMode==1?1:iNofArgs-iPos, &pDataBuffer, &iDataFormat, iMode);
	if (iResult>0) {
	  iNofWords=iResult;
	  if (iMode==1 && iNofArgs-iPos>2) {
	    if (strcmp(arrayArg[iPos+1], "-v")==0) {
	      if (getDecNumberFromArg(arrayArg[iPos+2], &iVerbosity, 0)<0)
		iVerbosity=0;
	    }
	  }
	}
      }

      // data buffers which have to be swapped are indicated with a negative value
      // quick hack, integrate this to the write functions of the dcscMsgBufferInterface later

/*       if (iEndianess<0) { */
/* 	if (pDataBuffer) { */
/* 	  //fprintf(stderr, "swapping ...\n"); */
/* 	  int iBufferSize=iNofWords*iDataFormat; */
/* 	  char* pTarget=(char*)malloc(iBufferSize); */
/* 	  if (pTarget) { */
/* 	    if (iDataFormat>1) { */
/* 	      // swap the 8 bit words */
/* 	      swab(pDataBuffer, pTarget, iBufferSize); */
/* 	      if (iDataFormat>2) { */
/* 		// additional swap of the 16 bit words */
/* 		__u16 tmp=0; */
/* 		__u16* pLsb=(__u16*)pTarget; */
/* 		__u16* pMsb=((__u16*)pTarget); pMsb++; */
/* 		int i=0; */
/* 		for (i=0; i<iNofWords; i++) { */
/* 		  //fprintf(stderr, "%#x <-> %#x\n", *pLsb, *pMsb); */
/* 		  tmp=*pLsb; */
/* 		  *pLsb=*pMsb; pLsb+=2; */
/* 		  *pMsb=tmp; pMsb+=2; */
/* 		} */
/* 	      } */
/* 	      if (pDataBuffer!=&data) free(pDataBuffer); */
/* 	      pDataBuffer=(__u32*)pTarget; */
/* 	    } */
/* 	  } else { */
/* 	    iResult=-ENOMEM; */
/* 	  } */
/* 	  //fprintf(stderr, "done\n"); */
/* 	} else { */
/* 	  // special case for swap of 32 bit single write */
/* 	  __u32 tmp=data; */
/* 	  char* pTgt=(char*)&data; */
/* 	  char* pSrc=((char*)&tmp)+3; */
/* 	  *pTgt++=*pSrc--; */
/* 	  *pTgt++=*pSrc--; */
/* 	  *pTgt++=*pSrc--; */
/* 	  *pTgt++=*pSrc--; */
/* 	} */
/*       } */

      if (iResult>=0) {
	if (pDataBuffer!=NULL) {
	  if (iResult>=0) {
	    //printf("multiple write: address=%#x %d word(s), word size %d\n", address, iNofWords, iDataFormat);
	    //printBufferHexFormatted((unsigned char*)pDataBuffer, iNofWords*iDataFormat, iDataFormat, 4, address, NULL);
	    //iResult=0;
	    if (iMode==0) {
	      iResult=rcuMultipleWrite(address, pDataBuffer, iNofWords, iDataFormat*iEndianess);
	    } else if (iMode==1) {
	      int iOffset=0;
	      int iSize=0;
	      fprintf(stderr, "checking message block, %d words\n", iNofWords);
	      do {
		iSize=dcscCheckMsgBlock(pDataBuffer+iOffset, iNofWords-iOffset, iVerbosity);
		if (iSize>0)
		  fprintf(stderr, "correct message block at offset %d size %d\n", iOffset, iSize);
		iOffset+=iSize;
	      } while (iSize>0 && iOffset<iNofWords);
	      iResult=iSize;
	      if (iResult==0)
		fprintf(stderr, "check failed, invalid format\n");
	      else if (iResult<0)
		fprintf(stderr, "check failed (%d)\n", iResult);
	    } else if (iMode==2) {
	      // flash write
	      iResult=rcuFlashWrite(address, pDataBuffer, iNofWords, iDataFormat*iEndianess);
	    } else if (iMode==3) {
	      // flash verify
	      int iBufferSize=iNofWords*sizeof(__u32);
	      __u32* pVerify=(__u32*)malloc(iBufferSize);
	      if (pVerify) {
		memset(pVerify, 0, iBufferSize);
		if ((iResult=rcuFlashRead(address, iNofWords, pVerify))>=0) {
		  int i=0;
		  for (i=0; i<iNofWords && iResult>=0; i++) {
		    if (iDataFormat==4) {
		      if (*(pVerify+i)!=*(pDataBuffer+i)) break;		     
		    } else if (iDataFormat==2) {
		      if (*((__u16*)(pVerify+i))!=*(((__u16*)pDataBuffer)+i)) break;		     
		    } else if (iDataFormat==1) {
		      if (*((__u8*)(pVerify+i))!=*(((__u8*)pDataBuffer)+i)) break;		     
		    }
		  }
		  if (i>=iNofWords) {
		    fprintf(stdout, "memory identical\n");
		  } else {
		    fprintf(stdout, "first difference at %#x\n", address+i);
		    printBufferHex((unsigned char*)(pVerify+i), 4, 4, "flash memory");
		    printBufferHex((unsigned char*)(pDataBuffer)+i*iDataFormat, iDataFormat, iDataFormat, "test file   ");
		  }
		} else {
		  fprintf(stderr, "verify failed, can not read from flash\n");
		}
		free(pVerify);
	      }
	    } else {
	      fprintf(stderr, "internal error, wrong mode to execWriteCmd (%d)\n", iMode);
	      iResult=-EIO;
	    }
	  } else {
	    fprintf(stderr, "internal error, can not proceed write command\n");
	  }
	  if (pDataBuffer!=&data) free(pDataBuffer);
	  pDataBuffer=NULL;
	} else if (iNofWords==1) {
	  //printf("single write: address=%#x data=%#x\n", address, data);
	  //iResult=0;
	  if (iMode==0) {
	    iResult=rcuSingleWrite(address, data);
	  } else {
	    fprintf(stderr, "internal error, wrong mode to execWriteCmd (%d)\n", iMode);
	    iResult=-EIO;
	  }
	} else if (iNofWords!=0) {
	  fprintf(stderr, "internal state error: supposed to write %d word(s) but no data buffer available\n", iNofWords);
	}
      }
    }
    if (iTimer>0) {
      printf("time : %ssec\n", getTimerValueString(iTimer));
      stopTimer(iTimer);
    }
  } else {
    fprintf(stderr, "too few arguments for write command\n");
    iResult=-EINVAL;
  }
  return iResult;
}

int ScanCoefficients(const char* pFormat, float* pfM, float* pfN, __u32* pMask)
{
  int iResult=0;
  if (pFormat && pfM && pfN && pMask) {
    int i=0;
    int j=-1;
    int iFormatSize=strlen(pFormat);
    char* pQuery=(char*)malloc(iFormatSize+1);
    if (pQuery) {
      strcpy(pQuery, pFormat);
      int iSpec=0;
      int iLastSpec=0;
      float* pfScan=NULL;
      int iBlanks=0;
      for (i=0; pFormat[i]!=0 && iResult==0; i++) {
	switch (pFormat[i]) {
	case '[': 
	  j=i+1;
	  break;
	case '*':
	  iSpec++;
	  // fall through intended
	case '+':
	  iSpec++;
	  // fall through intended
	case '-':
	  iSpec++;
	case '&':
	  iSpec++;
	case ']':
	  if (iLastSpec>0 && j>0 && j<i) {
	    if (j==i-1-iBlanks && iLastSpec==4 && iSpec==2) { // handling for signed multiplicators
	      iSpec=iLastSpec+1;
	    } else if (j<i-1) {
	      // read the value
	      if (iLastSpec>1) {
		// these are the floats
		if (iLastSpec>=4) pfScan=pfM;
		else pfScan=pfN;
		pQuery[i]=0; // terminate the format string
		iResult=getFloatNumberFromArg(&pQuery[j+1], pfScan, 1);
		pQuery[i]=pFormat[i]; // restore the format string
		if (iResult>0) iResult=0;  // just make sure to continue the for loop
		if (iLastSpec==2 || iLastSpec==5) *pfScan*=-1; // this is the minus specifier
	      } else {
		// the hexadecimal mask
		iResult=getHexNumberFromArg(&pQuery[j+1], pMask, 1);
	      }
	    } else {
	      fprintf(stderr, "ScanCoefficients: duplicate specifiers, %c overrides %c\n", pFormat[i], pFormat[j]);
	    }
	  }
	  if (iSpec>0) { // new specifier is starting
	    iLastSpec=iSpec;
	    iSpec=0;
	    j=i;
	  } else {
	    iResult=i+1; // return the length
	  }
	  iBlanks=0;
	  break;
	case ' ':
	  iBlanks++;
	  break;
	default:
	  iBlanks=0;
	  break;
	}
	if (j<0) {
	  fprintf(stderr, "ScanCoefficients: invalid format %s\n", &pFormat[i]);
	  break;
	}
	if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
	  fprintf(stderr, "ScanCoefficients: m=%f n=%f %s iResult=%d\n", *pfM, *pfN, &pFormat[i], iResult);
	}
      }
      free(pQuery);
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int printReadOutputFormatted(unsigned char *pBuffer, int iBufferSize, const char* pFormat, int iStartAddress, FILE* fp)
{
  int iResult=0;
  if (pBuffer && iBufferSize>0 && pFormat && fp) {
    int iFormatSize=strlen(pFormat);
    char* pWorkBuffer=(char*)malloc(iFormatSize+1);
    if (pWorkBuffer) {
      int iOffset=0;
      int iNofWords=iBufferSize/sizeof(__u32);
      int bBinary=0; // used for to check if binary and character format has been mixed, 0 virgin, -1 already char, 1 binary, 2 warning has been printed
      while (iOffset<iNofWords) {
	int i=0;
	int j=-1;
	int k=0;
	int bMissingNewline=1;
	strcpy(pWorkBuffer, pFormat);
	for (i=0; pFormat[i]!=0 && i<iFormatSize; i++) {
	  if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
	    fprintf(stderr, "printReadOutputFormatted: work buffer[i] %s\n", &pWorkBuffer[i]);
	    if (j>=0)
	      fprintf(stderr, "printReadOutputFormatted: work buffer[j] %s\n", &pWorkBuffer[j]);
	  }
	  switch (pFormat[i]) {
	  case '%':
	    if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
	      fprintf(stderr, "printReadOutputFormatted: i=%d j=%d %s\n", i, j, &pFormat[i]);
	    }
	    if (j>=0 && j<i) {
	      // print a warning if we are in binary mode
	      if (bBinary==1) {
		bBinary++;
		fprintf(stderr, "warning: your binary format string is mixed with other characters\n");
	      }
	      // flush the output
	      if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
		fprintf(stderr, "printReadOutputFormatted: flush work buffer i=%d j=%d %s\n", i, j, &pWorkBuffer[j]);
	      }
	      pWorkBuffer[i]=0;
	      if (bBinary==0) bBinary--; // to indicate that characters have been printed out already
	      fprintf(fp, &(pWorkBuffer[j]));
	      pWorkBuffer[i]=pFormat[i];
	    }
	    j=-1;
	    for (k=i+1; k<iFormatSize && j<0 ; k++) {
	      switch (pFormat[k]) {
	      case '%': // the '%' character, continue
		j=k-1;
		if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
		  fprintf(stderr, "printReadOutputFormatted: special character i=%d j=%d k=%d %s\n", i, j, k, &pFormat[k]);
		}
		break;
	      case '[': // the '[' character, continue
	      case ']': // the ']' character, continue
		j=k;
		if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
		  fprintf(stderr, "printReadOutputFormatted: special character i=%d j=%d k=%d %s\n", i, j, k, &pFormat[k]);
		}
		break;
	      case 'a': // print the address
		if (iStartAddress>=0) {
		  fprintf(fp, "0x%x", iStartAddress+iOffset);
		}
		j=k+1; // breaking condition and potential start of string
		if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
		  fprintf(stderr, "printReadOutputFormatted: print address i=%d j=%d k=%d %s\n", i, j, k, &pFormat[k]);
		}
		break;
	      case 'x': // print the value hex
	      case 'd': // print the value decimal
	      case 'f': // print the value float
		if (bBinary==1) {
		  bBinary++;
		  fprintf(stderr, "warning: you should not mix character and binary formats\n");
		}
		// fall through intended
	      case 'b': // print the value binary
		{
		  // coefficients for linear conversion
		  float fM=1; // the slope
		  float fN=0; // the offset
		  __u32 mask=0xffffffff;
		  int iCoeffInc=0; // number of characters of the weight definition, has to be added to the index later
		  int iWordSize=4;
		  if (pFormat[k]=='b') {
		    switch (pFormat[k+1]) {
		    case '1':
		      iWordSize/=2;
		      // fall through intended
		    case '2': 
		      iWordSize/=2;
		      // fall through intended
		    case '4':
		      iCoeffInc++;
		    }
		    if (bBinary<=0) {
		      if (bBinary<0) {
			bBinary=2; // indicate that the warning has been printed
			fprintf(stderr, "warning: you should not mix character and binary formats\n");
		      } else {
			bBinary=1; // to indicate that we are now in binary mode
		      }
		    }
		    if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT)
		      fprintf(stderr, "printReadOutputFormatted: print binary, word size %d\n", iWordSize);
		  }
		  if (pFormat[k+1+iCoeffInc]=='[') {
		    // scan for Coefficients
		    iResult=ScanCoefficients((const char*)&pFormat[k+1+iCoeffInc], &fM, &fN, &mask); 
		    if (iResult<0) {
		      fprintf(stderr, "printReadOutputFormatted: error scanning coefficients %s (%d)\n", &pFormat[k+1+iCoeffInc], iResult);  
		    } else {
		      iCoeffInc+=iResult;
		      iResult=0;
		    }
		  }
		  if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
		    fprintf(stderr, "printReadOutputFormatted: terminate work buffer i=%d j=%d k=%d %s\n", i, j, k, &pWorkBuffer[i]);
		  }
		  if (iOffset<iNofWords) {
		    // terminate the format string right behind this specifier
		    pWorkBuffer[k+1]=0;
		    __u32 value=*((__u32*)pBuffer+iOffset)&mask;
		    float fVal=value*fM + fN;
		    if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
		      fprintf(stderr, "printReadOutputFormatted: value=%u (%#x) fVal=%f fM=%f fN=%f\n", value, value, fVal, fM, fN);
		    }
		    if (pFormat[k]=='f') { // take the float value if 'f' specified otherwize the unsigned int
		      fprintf(fp, &(pWorkBuffer[i]), fVal);
		    } else {
		      if (fVal>=0 && fVal<=UINT_MAX) {
			if (value>INT_RO_MAX && (fM!=1.0 || fN!=0.0))
			  fprintf(stderr, "printReadOutputFormatted: conversion factors ignored, value exceeds limit %#x\n", INT_RO_MAX);
			else if (fM!=1.0 || fN!=0.0)
			  value = (__u32)fVal;
		      } else {
			fprintf(stderr, "printReadOutputFormatted: conversion error, coefficients do not fit data limits (val=%#x, m=%f, n=%f)\n", value, fM, fN);
		      }
		      if (pFormat[k]=='b') {
			fwrite((char*)&value, iWordSize, 1, fp);
		      } else {
			fprintf(fp, &(pWorkBuffer[i]), value);
		      }
		    }
		    // restore the original value of [k+1]
		    pWorkBuffer[k+1]=pFormat[k+1];
		    iOffset++;
		  }
		  k+=iCoeffInc; // set index behind the weight string
		  j=k+1; // breaking condition and potential start of string
		  if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
		    fprintf(stderr, "printReadOutputFormatted: print data i=%d j=%d k=%d %s\n", i, j, k, &pFormat[k]);
		  }
		}
		break;
	      }
	    }
	    if (j<0) {
	      fprintf(stderr, "printReadBufferFormatted: format error, no specifier found\n");
	    }
	    i=k-1;
	    break;
	  case '\\':
	    if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
	      fprintf(stderr, "printReadBufferFormatted: \\-character i=%d j=%d\n",i,j);
	    }
	    if (j>=0 && j<i) {
	      // flush the buffer
	      pWorkBuffer[i]=0;
	      fprintf(fp, "%s", &pWorkBuffer[j]);
	      pWorkBuffer[i]=pFormat[i];
	    }
	    // special handling for newline command
	    if (pFormat[i+1]=='n') {
	      if (j<0) {
		if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
		  fprintf(stderr, "printReadBufferFormatted: insert newline i=%d j=%d\n",i,j);
		}
	      }
	      bMissingNewline=0;
	      fprintf(fp, "\n");
	      i++;
	    }
	    j=-1;
	    break;
	  default:
	    if (j<0) j=i;
	  }
	}
	if (j < iFormatSize) {
	  // print remaining characters
	  fprintf(fp, &(pWorkBuffer[j]));
	  j=iFormatSize;
	}
	if (bMissingNewline==1 && bBinary<=0)
	  fprintf(fp, "\n");
      }
      fflush(fp);
      free(pWorkBuffer);
    } else {
      iResult=-ENOMEM;
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int execReadCmd(const char** arrayArg, int iNofArgs, int iMode)
{
  int iResult=0;
  unsigned int address=0;
  unsigned int data=0;
  int iNofWords=0;
  const char* pFormatSingleRead="%a: %#x"; // default format for rcu single read
  const char* pFormatMultiRead="%a: %#x %#x %#x %#x"; // default format for rcu multi read
  const char* pFormatFlash="%a: %#x[&0xffff] %#x[&0xffff] %#x[&0xffff] %#x[&0xffff]"; // default format for flash read
  const char* pFormat=NULL;
  FILE* fp=NULL;
  int i=0;
  if (iNofArgs>0 && arrayArg!=NULL) {
    if ((iMode==8 && getDecNumberFromArg(arrayArg[0], &address, 1)>=0 ) ||
	getHexNumberFromArg(arrayArg[0], &address, 1)>=0) {
      for (i=1; i<iNofArgs; i++) {
	if (*arrayArg[i]=='-') {
	  // identify the format options
	  switch (*(arrayArg[i]+1)) {
	  case 'o': // write output to file
	  case 'a': // append output to file
	    if (iNofArgs>i+1) {
	      const char * strMode = "w";
	      if (*(arrayArg[i]+1)=='a') strMode="a";
	      fp = fopen(arrayArg[++i], strMode);
	      if (fp==NULL)
		fprintf(stderr, "execReadCmd: can not open file %s for writing\n", arrayArg[i]);
	    } else {
	      fprintf(stderr, "execReadCmd: missing file name to %s option\n", arrayArg[i]);
	    }
	    break;
	  case 'f': // format string
	    if (iNofArgs>i+1) {
	      pFormat=arrayArg[++i];
	    } else {
	      fprintf(stderr, "execReadCmd: missing format string to %s option\n", arrayArg[i]);
	    }
	    break;
	  case 'b': // binary format
	    if (*(arrayArg[i]+2)=='1') // 8 bit format
	      pFormat="%b1";
	    else if (*(arrayArg[i]+2)=='2') // 16 bit format
	      pFormat="%b2";
	    else // 32 bit format
	      pFormat="%b4";
	    break;
	  default:
	    fprintf(stderr, "execReadCmd: unknown switch (\'%s\')\n", arrayArg[i]);
	  }
	} else {
	  if (iMode != 8 && getDecNumberFromArg(arrayArg[i], &iNofWords, 0)>=0) {
	  }
	}
      }
      if (!fp) fp=stdout;
      if (iNofWords>1) {
	int iBufferSize=iNofWords*sizeof(__u32);
	__u32* pData=(__u32*)malloc(iBufferSize);
	if (pData) {
	  memset(pData, 0, iBufferSize);
	  if (iMode==0) {
	    iResult=rcuMultipleRead(address, iNofWords, pData);
	    if (pFormat==0) pFormat=pFormatMultiRead;
	  } else if (iMode==2) {
	    iResult=rcuFlashRead(address, iNofWords, pData);
	    if (pFormat==0) pFormat=pFormatFlash;
	  } else
	    fprintf(stderr, "internal error, wrong mode to execReadCmd (%d)\n", iMode);
	  if (iResult>0)
	    printReadOutputFormatted((unsigned char*)pData, iBufferSize, pFormat, address, fp);
	    //printBufferHexFormatted((unsigned char*)pData, iBufferSize, 4, 4, address, NULL);
	  free(pData);
	} else {
	  fprintf(stderr, "execReadCmd: internal error: memory allocation failed\n");
	  iResult=-ENOMEM;
	}
      } else {
	if (pFormat==0) pFormat=pFormatSingleRead;
	  if (iMode==0) 
	    iResult=rcuSingleRead(address, &data);
	  else if (iMode==2)
	    iResult=rcuFlashRead(address, 1, &data);
	  else if (iMode==8)
	    iResult=smRegisterRead(address, &data);
	  else {
	    fprintf(stderr, "internal error, wrong mode to execReadCmd (%d)\n", iMode);
	    iResult=-EIO;
	  }
	if (iResult>0)
	  printReadOutputFormatted((unsigned char*)&data, 4, pFormat, address, fp);
	  //printBufferHexFormatted((unsigned char*)&data, 4, 4, 1, address, NULL);
      }
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int timedWait(int iWaitSec, int iWaitMusec)
{
  int iResult=0;
  // this is maybe overdoing it a little
  int iCurrSec=0;
  int iCurrMusec=0;
  int iTimer=startTimer(0); // timer of type ITIMER_REAL
  if (iTimer>0) {
    int iSleep=10;
    if (iWaitSec>0) iSleep=500000;
    else iSleep=iWaitMusec/4;
    do {
      usleep(iSleep);
      iResult=getTimerValue(iTimer, &iCurrSec, &iCurrMusec);
      //fprintf(stderr, "current timer val: %d sec  %d usec\n", iCurrSec, iCurrMusec);
    } while (iResult>=0 && (iCurrSec<iWaitSec || iCurrMusec<iWaitMusec));
    stopTimer(iTimer);
  } else {
    fprintf(stderr, "can not start timer, skip wait command\n");
    iResult=-EACCES;
  }
  return iResult;
}

/* termination handling
   called from SIGINT signal handler
 */
int g_bBatchProcessing=0;
int terminateBatchProcessing() {
  int iResult=0;
  if (g_bBatchProcessing>0) {
    iResult=1;
    g_bBatchProcessing=0;
#ifndef DCSC_TEST
    // for some strange reason this output interferes with the time() function in the echo command
    // this leads to a segfault on a redhat system although it works fine for the arm  
    fprintf(stderr, "\nterminating batch processing, please wait ...\n");
#endif //DCSC_TEST
  }
  return iResult;
}

int execBatch(const char** arrayArg, int iNofArgs)
{
  int iResult=0;
  if (arrayArg!=NULL && iNofArgs>0) {
    FILE* fp=fopen(arrayArg[0], "r");
    if (fp) {
      static int iInputBufferSize=100;
      char inputBuffer[iInputBufferSize];
      int iTimer=0;
      if (g_profiling>0) {
	iTimer=startTimer(0); // timer of type ITIMER_REAL
	if (iTimer==0)
	  fprintf(stderr, "can not start timer\n");
      }
      int iWaitSec=0;
      int iWaitMusec=0;
      int iLoops=1;
      int bInfinite=0;
      int i=0;
      /* the verbosity handling has to be revised
	 - local variable has to be initialized by the globel verbosity level
	 - defines instead of numbers must be used 
      */
      int iVerbosity=2; //VERBOSITY_HIGH
      for (i=1; i<iNofArgs; i++) {
	int bArgCheck=1;
	if (*arrayArg[i]=='-' && *(arrayArg[i]+1)!='l') {
	  // identify  options
	  switch (*(arrayArg[i]+1)) {
	  case 's': // silent -> verbosity=0
	    iVerbosity=0; // VERBOSITY_NONE
	    break;
	  case 'v': // verbosity
	    if ((bArgCheck=(iNofArgs>i+1))==1) {
	      if (getDecNumberFromArg(arrayArg[++i], &iVerbosity, 1)<0) {
	      }
	    }
	    break;
	  case '1':
	  case 'i': // ininite loop
	    iLoops=-1;
	    break;
	  case 'w': // read sleep parameters 
	    if ((bArgCheck=(iNofArgs>i+2))==1) {
	      if (readTime(&arrayArg[i+1], 2, &iWaitSec, &iWaitMusec)>=0) {
	      } else {
		fprintf(stderr, "execBatch: wrong argument format for \'%s\' switch, ignored\n", arrayArg[i]);
	      }
	      i+=2;
	    }
	    break;
	  default:
	    fprintf(stderr, "execBatch: unknown switch (\'%s\')\n", arrayArg[i]);
	  }
	} else {
	  if (*arrayArg[i]=='-' && *(arrayArg[i]+1)=='l') {
	    if ((bArgCheck=(iNofArgs>i+1))==1) i++;
	  }
	  // scan for the number of loops
	  if (getDecNumberFromArg(arrayArg[i], &iLoops, 0)<0) {
	    fprintf(stderr, "execBatch: invalid argument, loop count expected (%s)\n", arrayArg[i]);
	    iLoops=1;
	  } 
	}
	if (!bArgCheck) {
	  fprintf(stderr, "execBatch: missing argument for \'%s\' switch\n", arrayArg[i]);
	  iResult=-EINVAL;
	  i=iNofArgs;
	}
      }
      if (iResult>=0) {
	if (iLoops<0) {
	  bInfinite=1;
	  if (iVerbosity>=1/*VERBOSITY_LOW*/)
	    fprintf(stderr,"entering infinite loop, use CTRL-C to terminate\n");
	}
	g_bBatchProcessing=1;
	while ((iLoops!=0) && iResult>=0) {
	  fseek(fp, 0, SEEK_SET);
	  while (fgets(inputBuffer, iInputBufferSize, fp)!=NULL && (iResult>=0 || iResult==-ETIMEDOUT)) {
	    //fprintf(stderr, "next line: (%d) %s\n", inputBuffer[0], inputBuffer);
	    if (inputBuffer[0]>0) {
	      removePrecAndTrailingSpecChars(inputBuffer);
	      char* pFound=strstr(inputBuffer, arrayArg[0]);
	      if (inputBuffer[0]=='b' && pFound!=NULL && (pFound[strlen(arrayArg[0])]==0 || pFound[strlen(arrayArg[0])]==' ') ) { // just to avoid endless iteration
		if (iVerbosity>=2/*VERBOSITY_HIGH*/)
		  fprintf(stderr,"skipping command %s\n", inputBuffer);
	      } else if (inputBuffer[0]>15 && inputBuffer[0]!='#') {
		if (iVerbosity>=2/*VERBOSITY_HIGH*/)
		  fprintf(stderr,"executing: %s\n", inputBuffer);
		iResult=executeCommandLine(inputBuffer);
		if (iVerbosity>=3/*VERBOSITY_DEBUG*/)
		  fprintf(stderr, "  result: %d\n", iResult);
		if (iResult==-EINTR) // this was a terminated command, reset result
		  iResult=0;
	      }
	    } else {
	      break;
	    }
	  }
	  if (iResult>=0 && (iWaitSec>0 || iWaitMusec>0)) {
	    iResult=timedWait(iWaitSec, iWaitMusec);
	  }
	  if (bInfinite==0) iLoops--;
	  // terminate on CTRL-C (SIGINT)
	  if (g_bBatchProcessing==0) iLoops=0; 
	}
	if (iResult<0)
	  switch (iResult) {
	  case -ETIMEDOUT: break;
	  case -EIO: 
	    if (inputBuffer[0]=='?') {
	      fprintf(stderr, "batch processing aborted, check condition failed\n");
	      break;
	    }
	  default:
	    fprintf(stderr, "batch processing aborted, error %d\n", iResult);
	  }
	if (iTimer>0) {
	  fprintf(stderr, "total time: %ssec\n", getTimerValueString(iTimer));
	  stopTimer(iTimer);
	}
      }
      fclose(fp);
    } else {
      fprintf(stderr, "execBatch: can not open file %s\n", arrayArg[0]);
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

TArgDef driverCtrlArgs[] = {
  {"i","info",{eFctIndex, {(void*)printDriverInfo}},{(void*)1},ARGDEF_BREAK},
  {"l","lock",{eFctIndex, {(void*)dcscLockCtrl}},{(void*)eLock},ARGDEF_BREAK},
  {"u","unlock",{eFctIndex, {(void*)dcscLockCtrl}},{(void*)eUnlock},ARGDEF_BREAK},
  {"r","reset",{eFctIndex, {(void*)dcscLockCtrl}},{(void*)eDeactivateLock},ARGDEF_BREAK},
  {"a","activate",{eFctIndex, {(void*)dcscLockCtrl}},{(void*)eActivateLock},ARGDEF_BREAK},
  {NULL,"seize",{eFctIndex, {(void*)dcscLockCtrl}},{(void*)eSeize},ARGDEF_BREAK},
  {NULL,"release",{eFctIndex, {(void*)dcscLockCtrl}},{(void*)eRelease},ARGDEF_BREAK},
  {NULL,"debug",{eHex, {NULL}},{NULL},ARGDEF_BREAK},
  {"","",{eUnknownType, {NULL}},{NULL},ARGDEF_TERMINATE},
};

TFctArg driverCtrlDesc = {
  driverCtrlArgs,
  NULL,
  NULL
};

int driverCtrlCmds(TArgDef* pDef, void* pUser, FILE* pOut)
{
  /* this function is only necessary as long as the eFctHexArgs is not implemented
   */
  int iResult=0;
  if (pDef) {
    int iInt=0;
    if (ARGPROC_EXISTS(mrShellPrimGetHex(pDef, "debug", (unsigned int*)&iInt))) { // flags only 16 bit, can use same variable
      iResult=dcscDriverDebug(iInt);
    }
  }
  return iResult;
}

int executeMainCommands(const char* currentArg, const char** arrayArg, int iNofArgs, void* pUser, FILE* pOut)
{
  int iResult=0;
  int i=0;
  if (iNofArgs>0 && arrayArg) {
    if (setDebugOptionFlag(0)&DBG_ARGUMENT_CONVERT) {
      fprintf(stderr, "executeCommandArgs: %d argument(s): ", iNofArgs);
      for (i=0; i<iNofArgs; i++) {
	fprintf(stderr, "%p (%s) ", arrayArg[i], arrayArg[i]);
      }
      fprintf(stderr, "\n");
    }
    switch (*arrayArg[0]) {

    // print help
    // h          - main help
    // hd         - debug help
    // hd <level> - extended debug help
    // hr         - read command help
    // hb         - batch command help
    case 'h':
      if (*(arrayArg[0]+1)=='d') {
	int iLevel=0;
	if (iNofArgs>1) getDecNumberFromArg(arrayArg[1], &iLevel, 0);
	printDebugHelp(iLevel);
      } else if (*(arrayArg[0]+1)=='r') {
	printReadHelp();
      } else if (*(arrayArg[0]+1)=='b') {
	printBatchProcHelp();
      }
      else
	printHelp();
      break;

    // switching of debug flags
    case '+':
    case '-':
      {
	unsigned int iDbgOption=0;
	int iDbgFlag=1; // controls printing of the debug flags after the command interpretation 
	if (*arrayArg[0]=='-' && *(arrayArg[0]+1)!=0) {
	  printf("warning: you should check the command syntax, \n\'-\' is intended for switching debug flags\ntry \'%s\' instead of \'%s\'!\n", arrayArg[0]+1, arrayArg[0]);
	} else {
	  if (iNofArgs>1) {
	    // check for 'profile' keyword
	    if (strcmp(arrayArg[1], CMD_STRING_PROFILE)==0) {
	      iDbgFlag=0;// supress message on debug flag status below
	      if (*arrayArg[0]=='+') {
		g_profiling=1;
		printf("switching on profiling\n");
	      } else {
		g_profiling=0;
		printf("switching off profiling\n");
	      }
	      // check for 'timerdbg' keyword
	    } else if (strcmp(arrayArg[1], CMD_STRING_MRT_DEBUG)==0) {
	      iDbgFlag=0;// supress message on debug flag status below
	      if (iNofArgs>2 && getHexNumberFromArg(arrayArg[2], &iDbgOption, 0)>=0) {
		if (*arrayArg[0]=='+') {
		  iResult=setMRTimerDebugFlag(iDbgOption);
		} else {
		  iResult=clearMRTimerDebugFlag(iDbgOption);
		}
		printf("timer debug flags: %#x\n", iResult);
	      } else {
		iResult=-EINVAL;
	      }
	      // read the flag
	    } else if (getHexNumberFromArg(arrayArg[1], &iDbgOption, 0)>=0) { 
	      if (*arrayArg[0]=='+')
		iResult=setDebugOptionFlag((int)iDbgOption);
	      else
		iResult=clearDebugOptionFlag((int)iDbgOption);
	    }
	  } else if (*arrayArg[0]=='-') {
	    iResult=setDebugOptions(0);
	  } else if (*arrayArg[0]=='+') {
	    iResult=setDebugOptions(DBG_DEFAULT);
	  }
	  if (iResult>=0 && iDbgFlag>0)
	    printf("debug options: 0x%x\n", iResult);
	}
      }
      break;

    // command, substituted with w address 0x0
    case 'c':
      if (iNofArgs>1) {
	if (*(arrayArg[0]+1)==0 || strcmp(arrayArg[0], "command")==0) {
	const char* tempArgs[2];
	tempArgs[0]=arrayArg[1];
	tempArgs[1]="0x0";
	iResult=execWriteCmd(tempArgs, 2, 0);
	} else if ( strcmp(arrayArg[0], "checkmsgbuf")==0) {
	iResult=execWriteCmd(arrayArg+1, iNofArgs-1, 1);
	}
      }
      break;

    // the write and the wait command command
    case 'w':
      if (iNofArgs>1) {
	if (*(arrayArg[0]+1)==0 || strcmp(arrayArg[0], "write")==0) {
	  // the write command
	  iResult=execWriteCmd(arrayArg+1, iNofArgs-1, 0);
	} else if (strcmp(arrayArg[0], "wait")==0) {
	  // the wait command
	  int iWaitSec=0;
	  int iWaitMusec=0;
	  if (iNofArgs>=3) {
	    if ((iResult=readTime(arrayArg+1, 2, &iWaitSec, &iWaitMusec))>=0) {
	      //fprintf(stderr, "wait: %d sec  %d usec\n", iWaitSec, iWaitMusec);
	      iResult=timedWait(iWaitSec, iWaitMusec);
	    }
	  } else {
	    iResult=-EINVAL;
	  }
	} else {
	  iResult=-EINVAL;
	}
      }
      else
	iResult=-EINVAL;
      break;

    // the read command
    case 'r':
      if (iNofArgs>1)
	iResult=execReadCmd(arrayArg+1, iNofArgs-1, 0);
      else
	iResult=-EINVAL;
      break;

    // batch processing
    case 'b':
      if (iNofArgs>1)
	iResult=execBatch(arrayArg+1, iNofArgs-1);
      else
	iResult=-EINVAL;
      break;

    // waiting for conditions
    case '?':
      iResult=waitCondition(arrayArg+1, iNofArgs-1);
      break;

      /* the old memory check, deprecated
	  case 'm':
	  if (sscanf(pCmdLine+1, " 0x%x 0x%x", &address, &data)==2) {
	  memTestAddressBus(address, data);
	  memTestDevice(address, data);
	  } else {
	  printf("wrong format, try 'h' for help\n");
	  }
	  break;
	*/

    // echo: print a message
    case 'e':
      {
	int i=0;
	FILE* fp=stdout; // make sure to change down as well if you change this default
	for (i=1; i<iNofArgs; i++) {
	  const char* pArg=NULL;
	  if (*arrayArg[i]=='-') {
	    switch (*(arrayArg[i]+1)) {
	    case 'o': // write output to file
	    case 'a': // append output to file
	      if (iNofArgs>i+1) {
		const char * strMode = "w";
		if (*(arrayArg[i]+1)=='a') strMode="a";
		fp = fopen(arrayArg[++i], strMode);
		if (fp==NULL)
		  fprintf(stderr, "can not open file %s for writing\n", arrayArg[i]);
	      } else {
		fprintf(stderr, "missing file name to %s option\n", arrayArg[i]);
	      }
	      break;
	    case 't': // timestamp
	      {
		time_t currentTime;
		time(&currentTime);
		const int iMaxChars=30;
		char strTime[iMaxChars+1];
		memset(strTime, 0, iMaxChars+1);
		strcpy(strTime, "blabla");
		if (strftime(strTime, iMaxChars, "%b %d %H:%M:%S %Y", localtime(&currentTime))>0) {
		  fprintf(fp, " %s", strTime);
		}
	      }
	      break;
	    default:
	      pArg=arrayArg[i]+1;
	    }
	  } else {
	    pArg=arrayArg[i];
	  }
	  if (pArg) {
	    fprintf(fp, " %s", pArg);
	  }
	}
	if (fp) {
	  fprintf(fp, "\n");
	  if (fp!=stdout)
	    fclose(fp);
	}
      }
      break;
    //
    default:
      iResult=-ENOENT;
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int printFlashHelp() {
  printf("  access to the RCU flash memory:\n");
  printf("  keyword \'flash\' followed by\n");
  printf("   enable  (e): enable access, this disables \'selectmap\' and \'message buffer\'\n");
  printf("   disable (d): disable access, switches back to \'message buffer\'\n");
  printf("   reset      : reset the flash\n");
  printf("   ctrldcs    : control via dcs board firmware (v2.2 or higher)\n");
  printf("   ctrlactel  : control via actel firmware (compatibility mode)\n");
  printf("   read    (r): read from the flash (format and options as rcu read)\n");
  printf("   write   (w): write to the flash (format and options as rcu write)\n");
  printf("   verify  (v): verify flash memory with file (format and options write)\n");
  printf("   id         : read the flash id\n");
  printf("   erase sector <address>        : erase sector <address>\n");
  printf("   erase sector <address> <count>: erase <count> sectors\n");
  printf("   erase all  :                     erase the whole flash\n");
  
  //  printf("  \n");
  return 0;
}

int execFlashWriteCmd(const char* currentArg, const char** arrayArg, int iNofArgs, void* pUser, FILE* pOut) 
{
  if (arrayArg && iNofArgs>0)
    return execWriteCmd(arrayArg+1, iNofArgs-1, 2);
  return -EINVAL;
}

int execFlashVerifyCmd(const char* currentArg, const char** arrayArg, int iNofArgs, void* pUser, FILE* pOut) 
{
  if (arrayArg && iNofArgs>0)
    return execWriteCmd(arrayArg+1, iNofArgs-1, 3);
  return -EINVAL;
}

int execFlashReadCmd(const char* currentArg, const char** arrayArg, int iNofArgs, void* pUser, FILE* pOut) 
{
  if (arrayArg && iNofArgs>0)
    return execReadCmd(arrayArg+1, iNofArgs-1, 2);
  return -EINVAL;
}

int execFlashEraseall()
{
  return rcuFlashErase(-1, 0);
}

int execFlashErase(TArgDef* pDef, void* pUser, FILE* pOut)
{
  int iResult=0;
  if (pDef) {
    int startsec=0;
    int count=1;
    int localRes=0;
    //    PrintArgumentDefinition(pDef, 1);
    if ((localRes=mrShellPrimGetInt(pDef, "all", &startsec))>=0 && ARGPROC_EXISTS(localRes) && startsec==1) {
      //fprintf(stderr, "rcuFlashErase all\n");
      iResult=rcuFlashErase(-1, 0);
      startsec=-1;
    } else {
      if ((localRes=mrShellPrimGetHex(pDef, "sector", &startsec))>=0 && ARGPROC_EXISTS(localRes)) {
	if ((localRes=mrShellPrimGetInt(pDef, "_count", &count))<0 || !(ARGPROC_EXISTS(localRes))) count=1;
	//fprintf(stderr, "rcuFlashErase startsec=%#x, count=%d\n", startsec, count);
	iResult=rcuFlashErase(startsec, count);
      }
    }
  }
  return iResult;
}

int execSmWriteReg(const char* currentArg, const char** arrayArg, int iNofArgs, void* pUser, FILE* pOut) 
{
  if (arrayArg && iNofArgs>0)
    return execWriteCmd(arrayArg+1, iNofArgs-1, 8);
  return -EINVAL;
}

int execSmReadReg(const char* currentArg, const char** arrayArg, int iNofArgs, void* pUser, FILE* pOut) 
{
  if (arrayArg && iNofArgs>0)
    return execReadCmd(arrayArg+1, iNofArgs-1, 8);
  return -EINVAL;
}

int printSelectmapHelp() {
  printf("  access to the RCU FPGA via the select map interface:\n");
  printf("  keyword \'selectmap\' or \'sm\' followed by\n");
  printf("   enable        : enable access, this disable the flash and the msg buffer\n");
  printf("   disable       : disable access\n");
  printf("   read-reg  (rr): read register of the selectmap interface\n");
  printf("                   read-reg <reg no>\n");
  printf("   write-reg (wr): write register of the selectmap interface\n");
  printf("                   write-reg <reg no> 0x<value>\n");
  //  printf("  \n");
  return 0;
}

int printFirmwareHelp() {
  printf("  DCS board firmware command group:\n");
  printf("  keyword \'firmware\' or \'fw\' followed by\n");
  printf("   reset        (r) :  triggers the reset sequence\n");
  printf("   read-reg     (rr):  read firmware register\n");
  printf("   write-reg    (wr):  write firmware register\n");
  printf("   enable-comp  (ec):  enable use of compressed data words\n");
  printf("   disable-comp (dc):  enable use of compressed data words\n");
  //  printf("  \n");
  return 0;
}

int execRegWriteCmd(const char* currentArg, const char** arrayArg, int iNofArgs, void* pUser, FILE* pOut) 
{
  int regNo=0;
  int value=0;
  int iResult=-EINVAL;
  if (arrayArg && iNofArgs>2) {
    if ((getDecNumberFromArg(arrayArg[1], &regNo, 1)>=0) &&
	(getHexNumberFromArg(arrayArg[2], &value, 1)>=0)) {
      if ((iResult=msgBufWriteRegister(regNo, value))<0) {
	fprintf(stderr, "failed to write register %2d: %d\n", regNo, iResult);
      }
    }
  }
  return iResult;
}

int execRegReadCmd(const char* currentArg, const char** arrayArg, int iNofArgs, void* pUser, FILE* pOut) 
{
  int regNo=0;
  int iResult=-EINVAL;
  if (arrayArg && iNofArgs>1) {
    if (getDecNumberFromArg(arrayArg[1], &regNo, 1)>=0) {
      if ((iResult=msgBufReadRegister(regNo))>=0) {
	fprintf(stdout, "register %2d: %#x\n", regNo, iResult);
	fflush(stdout);
      }
    }
  }
  return iResult;
}

int ctrlRegStatus() {
  int iReg=rcuBusControlCmd(eReadCtrlReg);
  if (iReg>=0) {
    fprintf(stdout, "control register: %#x\n", iReg);
  } else {
    fprintf(stderr, "ctrlRegStatus (%s) error: can not read control register (%d)\n", __FILE__, iReg);
  }
  return 0;
}

TFctMode scanModeDefault = {SCANMODE_FORCE_TERMINATION, NULL, NULL};

TArgDef selectmapCmds[] = {
  {"e","enable",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eEnableSelectmap},ARGDEF_TERMINATE},
  {"d","disable",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eDisableSelectmap},ARGDEF_TERMINATE},
  {"wr","write-reg",{eFctUserScan, {(void*)execSmWriteReg}},{NULL},ARGDEF_TERMINATE},
  {"rr","read-reg",{eFctUserScan, {(void*)execSmReadReg}},{NULL},ARGDEF_TERMINATE},
  {"*",NULL,{eFctNoArg, {(void*)printSelectmapHelp}},{NULL},ARGDEF_TERMINATE},
  {"","",{eUnknownType, {NULL}},{NULL},ARGDEF_TERMINATE},
};

TArgDef flashEraseParams[] = {
  {NULL,"all",{eBool, {(void*)0}},{(void*)NULL},ARGDEF_BREAK},
  {"sec","sector",{eHex, {(void*)0}},{(void*)NULL},0},
  {NULL,"_count",{eInteger, {(void*)0}},{(void*)NULL},ARGDEF_RESUME|ARGDEF_KEYWORDLESS},
  {"","",{eUnknownType, {NULL}},{NULL},ARGDEF_TERMINATE},
};

TFctArg flashEraseDesc = {
  flashEraseParams,
  NULL,
  NULL
};

TArgDef flashCmds[] = {
  {"e","enable",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eEnableFlash},ARGDEF_TERMINATE},
  {"d","disable",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eDisableFlash},ARGDEF_TERMINATE},
  {NULL,"reset",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eResetFlash},ARGDEF_TERMINATE},
  {NULL,"id",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eFlashID},ARGDEF_TERMINATE},
  {NULL,"ctrldcs",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eFlashCtrlDCS},ARGDEF_TERMINATE},
  {NULL,"ctrlactel",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eFlashCtrlActel},ARGDEF_TERMINATE},
  {"w","write",{eFctUserScan, {(void*)execFlashWriteCmd}},{NULL},ARGDEF_TERMINATE},
  {"r","read",{eFctUserScan, {(void*)execFlashReadCmd}},{NULL},ARGDEF_TERMINATE},
  {"v","verify",{eFctUserScan, {(void*)execFlashVerifyCmd}},{NULL},ARGDEF_TERMINATE},
  {NULL,"erase",{eFctArgDef, {(void*)execFlashErase}},{(void*)&flashEraseDesc},ARGDEF_TERMINATE},
  {NULL,"eraseall",{eFctNoArg, {(void*)execFlashEraseall}},{NULL},ARGDEF_TERMINATE},
  {"*",NULL,{eFctNoArg, {(void*)printFlashHelp}},{NULL},ARGDEF_TERMINATE},
  {"","",{eUnknownType, {NULL}},{NULL},ARGDEF_TERMINATE},
};

TArgDef firmwareCmds[] = {
  {"r","reset",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eResetFirmware},ARGDEF_TERMINATE},
  {"wr","write-reg",{eFctUserScan, {(void*)execRegWriteCmd}},{NULL},ARGDEF_TERMINATE},
  {"rr","read-reg", {eFctUserScan, {(void*)execRegReadCmd}}, {NULL},ARGDEF_TERMINATE},
  {"ec","enable-comp",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eEnableCompression},ARGDEF_TERMINATE},
  {"dc","enable-comp",{eFctIndex, {(void*)rcuBusControlCmd}},{(void*)eDisableCompression},ARGDEF_TERMINATE},
  {"*",NULL,{eFctNoArg, {(void*)printFirmwareHelp}},{NULL},ARGDEF_TERMINATE},
  {"","",{eUnknownType, {NULL}},{NULL},ARGDEF_TERMINATE},
};

TArgDef ctrlRegCmds[] = {
  {"","",{eUnknownType, {NULL}},{NULL},ARGDEF_TERMINATE},
};

TArgDef mainCommands[] = {
  {"sm","selectmap",{eFctRemaining, {(void*)selectmapCmds}},{(void*)&scanModeDefault},ARGDEF_TERMINATE},
  {NULL,"flash",{eFctRemaining, {(void*)flashCmds}},{(void*)&scanModeDefault},ARGDEF_TERMINATE},
  {"fw","firmware",{eFctRemaining, {(void*)firmwareCmds}},{(void*)&scanModeDefault},ARGDEF_TERMINATE},
  {"rcr","read-ctrlreg",{eFctNoArg, {(void*)ctrlRegStatus}},{NULL},ARGDEF_TERMINATE},
  {"d","driver",{eFctArgDef, {(void*)driverCtrlCmds}},{(void*)&driverCtrlDesc},ARGDEF_TERMINATE},
  {"i","info",{eFctNoArg, {(void*)printInfo}},{NULL},ARGDEF_TERMINATE},
  {"#",NULL,{eBool, {NULL}},{NULL},ARGDEF_TERMINATE},
  {NULL,"*",{eFctUserScan, {(void*)executeMainCommands}},{NULL},ARGDEF_TERMINATE},
  {"","",{eUnknownType, {NULL}},{NULL},ARGDEF_TERMINATE},
};

int executeCommandArgs(int iNofArgs, const char** arrayArg)
{
  int iResult=0;
  TArgDef* pDef=mrShellPrimCloneDef(mainCommands);
  if (pDef) {
    iResult=ScanArguments((const char**)arrayArg, iNofArgs, 0, pDef, NULL);
    /* 	fprintf(stderr, "deleting definition clone\n"); */
    free(pDef);
  }
  return iResult;
}

int executeCommandLine(char* pCmdLine)
{
  int iResult=0;
  static int iArraySize=15;
  char* arrayArg[iArraySize];
  int iNofArgs=0;
  int iDebugFlags=setDebugOptionFlag(0);
  if (iArraySize>=(iNofArgs=buildArgumentsFromCommandLine(pCmdLine, arrayArg, iArraySize, 1, iDebugFlags))) {
    iResult=executeCommandArgs(iNofArgs, (const char**)arrayArg);
    if (iResult==-EINTR) iResult=0; // -EINTR was used to indicated termination
    if (iResult<0 && (iDebugFlags&DBG_ARGUMENT_CONVERT))
      fprintf(stderr, "executeCommandArgsFailed (%d)\n", iResult);
    if (iResult==-EINVAL)
      printf("wrong format, try 'h' for help\n");
    else if (iResult==-ENOENT)
      printf("wrong operation, try 'h' for help\n");
  } else {
    fprintf(stderr, "argument list restricted to %d arguments\n", iArraySize);
    iResult=-E2BIG;
  }
  return iResult;
}
