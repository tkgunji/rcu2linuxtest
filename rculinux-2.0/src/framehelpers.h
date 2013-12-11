// $Id: framehelpers.h,v 1.2 2008/02/27 07:47:54 richter Exp $

/****************************************************************************
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2006
** This file has been written by Dominik Fehlker
** Please report bugs to dfehlker@htwm.de
**
** Permission to use, copy, modify and distribute this software and its  
** documentation strictly for non-commercial purposes is hereby granted  
** without fee, provided that the above copyright notice appears in all  
** copies and that both the copyright notice and this permission notice  
** appear in the supporting documentation. The authors make no claims    
** about the suitability of this software for any purpose. It is         
** provided "as is" without express or implied warranty.                 
**/

/** @file       framehelpers.h
    @author     Dominik Fehlker
    @date
    @brief      Helpfunctions for programs that have to deal with Frames
*/

#include <linux/types.h> // for the __u32 and __u16 types

/**
 * The bits 16 to 31 from an 32 bit address are returned as an 16 bit address.
 * 
 * @return the upper 16 Bits (16 to 31)
 * @ingroup sm_tools
 */
__u16 getUpperAddress(__u32 u32address);


/**
 * The bits 0 to 15 from an 32 bit address are returned as an 16 bit address.
 * 
 * @return the lower 16 bits (0 to 15)
 * @ingroup sm_tools
 */
__u16 getLowerAddress(__u32 u32address);


/**
 * checks the filesize of a file with the given path
 *
 * @return the filesize in Bytes
 * @ingroup sm_tools
 */
int getFileSize(char *path);


/**
 * 
 * calculates the stop address for initial config and scrubbing after the formula:
 * stopaddress = (startaddress + (filesize / 2) - 1)
 * if the firmware has the version 1.2 then 0x100h is added to the stopaddress.
 * 
 * @param filesize	the filesize of the framefile to program
 * @param startaddress	the startaddress where programming has begun
 * @return the stopaddress
 * 
 * @ingroup sm_tools
 */
int calculateStopAddress(int filesize, int startaddress);


/**
 * if state is different from flash, then the flash state will be entered.
 *
 * @param oldstate	the state before entering flash state
 * @return not used
 * @ingroup sm_tools
 */
int enterFlashState(int oldstate);


/**
 * checks the state of the RCU BUS and returns this state
 * 
 * @return the current state of the RCU Bus (1 means selectmap, 2 flash and 3 MsgBuffer)
 * @ingroup sm_tools
 */
int getBusState();


/**
 * If the oldstate was something different then flash, restoreBusState switches
 * back to this state. if oldstate was already flash, then nothing is done.
 *
 * @param oldstate	the old state of the Bus
 * @return not used
 * @ingroup sm_tools
 */
int restoreBusState(int oldstate);

/**
 * takes a line from the frames file and extracts the frame numbers
 * block, major and minor and write them in the 3 element int array pointed
 * to by the pointer pframeaddr.
 * In case something not useful is read, block, major and minor are -1
 * 
 * @param line          a line to be fragmented
 * @param *pframaddr	the 3 element int array which holds block, major and minor
 * @ingroup sm_tools
 */

int getFrameAddressFromLine(char line[255], int *pframeaddr);


/** 
 * very propably useless - check that later
 * reads an character and returns an int
 *
 * @return the integer value according to the character, everything else than 0-9 returns -1
 * @ingroup sm_tools
 */
int charToInt(char zeichen);

/** 
 * used for reading the framesfile
 *
 * takes an character array with up to 255 characters and seperates
 * them using ";" into Block-, Major- and Minornumber.
 * Calculates the address31_16 and address15_0 for the globale variables
 * as well as the w_frameX.X.X.hex and r_frame.X.X.X.hex filenames.
 * Example line for frames.txt:
 * 0;5;3;
 * 
 * comments can be made using a leading "#"
 * @ingroup sm_tools
 */
int getLinesnumberFromFile(char *filename);

/**
 * used for reading the header and footer files.
 *
 * reads a 4 digit hex value from the given char array
 * if it finds the variables "$frame_addr31_16" or "$frame_addr15_0"
 * the address values from the given unsigned short array with two elements
 * is used.
 *
 * 
 * @param line		the line where the hex value shall be read from
 * @param pAddresses	the 2 element unsigned short array which holds add31_16 and addr15_0
 * @return      the hexadecimal value
 * @ingroup sm_tools
 */
int getHexValueFromLine(char line[255], unsigned short *pAddresses);


