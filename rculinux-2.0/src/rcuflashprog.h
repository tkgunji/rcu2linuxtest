// $Id: rcuflashprog.h,v 1.4 2008/02/27 07:47:55 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2006
** This file has been written by Dominik Fehlker,
** dfehlker@htwm.de
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
/** @file   rcuflashprog.h
    @author Dominik Fehlker
    @date   
    @brief   Tool for programming the RCU Flash memory. */

/** 
 * @defgroup rcuflashprog RCU Flash Programmer
 * 
 * @ingroup sm_tools Selectmap tools
 */


/*
 * All things needed to program an initial configuration file the flash
 * is done here. All Parameters for this are read from the configuration file.
 * 
 * @return not used
 * @ingroup sm_tools
 */
int doInit(char *conffilename, int BB_FLASH);
/**
 * Everything needed for a complete scrubbing is done here. All necessary
 * Information is read out from the given configfile.
 * 
 * @return not used
 * @ingroup sm_tools
 */
int doScrubbing(char *conffilename, int BB_FLASH);

/**
 * The programming of the readframes and writeframes is done here.
 * Therefore the framesfile given in the configuration file is read out,
 * and all the given frames there are written to the flash.
 * 
 * @return not used
 * @ingroup sm_tools
 */
int doFlashFrame(char *conffilename, int BB_FLASH);

/**
 * The frameaddress is given with the block, major and minor in an int array
 * as the first parameter. the second parameter is the filename which contains
 * an "$" somewhere which is substituted with the frameaddress in this way:
 * "<block>.<major>.<minor>"
 *
 * @ingroup sm_tools
 */
void buildFrameFilename(int *pFrameaddr, char *pnewfilename);
