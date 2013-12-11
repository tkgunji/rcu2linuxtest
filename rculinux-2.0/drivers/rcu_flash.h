// $Id: rcu_flash.h,v 1.3 2006/05/02 16:48:27 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter, Johan Alme, Ketil Roed
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

#ifndef __RCU_FLASH_H
#define __RCU_FLASH_H

// driver version
#define RCUFLASH_MAJOR_VERSION_NUMBER 0
#define RCUFLASH_MINOR_VERSION_NUMBER 1

// ioctl commands
// define specific ioctl commands here
#define IOCTL_RCUF_GET_FLASH_SIZE      0x100 // get the size of the flash

#endif //__RCU_FLASH_H
