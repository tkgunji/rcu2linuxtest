// $Id: framegen.h,v 1.5 2006/06/21 13:00:43 dominik Exp $

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

/** @file   framegen.h
    @author Dominik Fehlker
    @date   
    @brief  The framegenerator for Xilinx FPGA. */

/** 
 * @defgroup framegen Frame Generator
 * The Framegenerator framegen can be used to read out frames from the Xilinx
 * which will be stored with read and write headers and footers.
 * 
 * All the frames that should be read out have to be given in a ascii text file
 * in the first parameter. One Frame per line in this format: "0;5;3;". Comments
 * are allowed with leading "#".
 * 
 * The second parameter to be given is the readheader file, every line contains
 * a value to be written to the Xilinx, the address has to be inserted as variables
 * "$frame_addr31_16" and "$frame_addr15_0". These addresses will be generated from
 * the framesfile.
 * 
 * third parameter is the readfooter, 4th the writeheader and 5th the writefooter.
 * 
 * For every frame, 3 Files will be generated:  w_frameX.X.X.hex, r_frame.X.X.X.hex and 
 * frameX.X.X.hex where
 * one contains the readheader with the address instead of the variable, then the data
 * and the read_footer, the other containing the writeheader, 2 times the data and the writefooter and 
 * the 3rd one containing just the data.
 * 
 * All generated files will be stored in the working directory.
 * 
 * @ingroup sm_tools Selectmap tools
 */


/**
 * executes a previously written command
 * 
 * @return	the errorcode
 * @ingroup sm_tools
 */
int commit();

