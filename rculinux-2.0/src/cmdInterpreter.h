// $Id: cmdInterpreter.h,v 1.5 2005/11/01 14:27:49 richter Exp $

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

#ifndef _CMDINTERPRETER_H
#define _CMDINTERPRETER_H

#define UINT_MAX 0xffffffff
/* the limit for conversion pf the read output is set to 16bit unsigned
 * there is some strange behaviour in the float processing for higher values
 * e.g. unsigned int iHex=0xaaaaaaaa; float fVal=iHex;
 * gives fVal=2863311616.00 instead of 2863311530
 * unsigned int iHex=0xffffff; float fVal=iHex*2 + 1;
 * fVal=33554432.00 instead of 33554431
 */
#define INT_RO_MAX 0xffff

int executeCommandArgs(int iNofArgs, const char** arrayArg);
int executeCommandLine(char* pCmdLine);
int terminateBatchProcessing();

int printHelp();
int printInfo();

#endif // _CMDINTERPRETER_H
