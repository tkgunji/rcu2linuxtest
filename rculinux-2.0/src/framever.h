// $Id: framever.h,v 1.5 2008/02/27 07:47:54 richter Exp $

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

/** @file       framever.h
    @author     Dominik Fehlker
    @date
    @brief      The frameverifier for the Xilinx FPGA */

/**
 * @defgroup framever Frame Verifier
 * The Frameverifier - framever
 *
 *
 * a utility to verify frames from the Xilinx memory against the ones and the flash
 * and the ones stored on a drive. In case of an error, and Errorfile is created
 * which contains the the positions of the bitflips. Also the errors are counted
 * for every frame, every cycle and the total number of errors during all cycles.
 * For a complete run a logfile log.txt will be created, containing these information.
 * <!---
 * usage: framever frames.txt 20
 *                     ^      ^- optional number of cycles, if not given a standard is used
 *                     '-------- File containing the frames in a way like
 *                                block;major;minor; - for example "0;5;3;"     
 * --->
 * @ingroup sm_tools Selectmap tools
 */

/**
 * do the step-by-step frame verification
 * 
 * @return              the errorcode
 * @ingroup sm_tools
 */
int step();


/**
 * reads out the R_ErrCnt register of the Xilinx
 * which holds the number of errors found by readback and
 * verification of Xilinx configuration memory
 * 
 * @return              the read data
 * @ingroup sm_tools
 */
__u32 getErrorcounterReg();


/**
 * reads out the RB_Err_FrameNumber register of the Xilinx
 * which holds the last frame number with an error as given
 * by the sequence stored in the flash memory
 * 
 * @return              returns the read data
 * @ingroup sm_tools
 */
__u32 getLastErrorFramenumber();


/**
 * reads out the RB_FrameNumber register of the Xilinx
 * which holds the number of the last frame being verified
 * as given by the sequence stored in the flash memory
 * 
 * @return              returns the read data
 * @ingroup sm_tools
 */
__u32 getLastFramenumber();


/**
 * reads the RB_numOfCycles register of the Xilinx
 * which holds the number of times a complete Frame by Frame
 * readback of all frames has been done
 * 
 * @return              returns the read data
 * @ingroup sm_tools
 */
__u32 getNumberOfCycles();


/**
 * reads the error register of the Xilinx
 * 
 * @return              returns the read data
 * @ingroup sm_tools
 */
__u32 readErrReg();


/**
 * reads the status register of the Xilinx
 * 
 * @return              returns the read data
 * @ingroup sm_tools
 */
__u32 readStatusReg();


/**
 * clears the error register of the Xilinx
 * 
 * @return              -1 if something goes wrong
 * @ingroup sm_tools
 */
int clearErrReg();


/**
 * stupid, but works for now
 * @ingroup sm_tools
 */
int charToInt(char zeichen);


/**
 * resets the controller, overwrites the flash cache memory and the selectmap memory,
 * clears the status and error register.
 * 
 * @return      an negative errorcode, 0 if successful
 * @ingroup sm_tools
 * 
 */
int init();


/**
 * Clears and opens the logfile and write a header to it, containing a timestamp
 * 
 * @return      not used
 * @ingroup sm_tools
 * 
 */
int writeHeaderToLogfile();


/** 
 * Takes 16 bits,  counts and returns the occuring "1"s.
 * 
 * @param bitfield      16 bits of which the "1"s should be counted
 * @return              the number of ones in the 16 bit bitfield
 * @ingroup sm_tools
 */
int analyze16bit(__u16 bitfield);

//void getMeaningOfErrReg(int value);
