// $Id: dcs_driver.c,v 1.15 2006/05/30 10:07:37 richter Exp $

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

#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>

#include <linux/fs.h>      // basic file structures and methods
#include <linux/types.h>   // basic data types
#include <linux/sched.h>   // task schedule
#include <linux/errno.h>   // error codes
#include <linux/slab.h>    // kmalloc, kmfree
#include <asm/io.h>        // ioremap, ...
//#include <asm/uaccess.h>   // user space memory access (e.g. copy_to_user)
//#include <linux/poll.h>
#include <linux/mm.h>      // virtual memory mapping
#include <linux/ioport.h>  // detecting and reserving system resources
#include <linux/spinlock.h>
#include <asm/system.h>
#include <linux/module.h>
#include "dcs_driver.h"
#include "version.h"
#include "mrKernLogging.h"


#define DCSC_TEST
/*
 * default major no of the driver
 */
int dcsc_majorID=150; /* fixed major no */



/*
 * default hardware addresses
 */
//base addresses and interrupt request numbers for the devices
//#define INCLUDE_EXCALIBUR_H // read the buffer configuration from file
#ifdef INCLUDE_EXCALIBUR_H
/* this was just an attempt to directly use the include file from the firmware design
 * but this caused problems due to requiring other include files and I got really messed up
 * may be this will be usefull in future but I doubt this
 */
#include "excalibur.h"
static u32* msgbuf_in_physaddr  = ((u32 *) na_MSGiBuffer_base);
static u32* msgbuf_out_physaddr = ((u32 *) na_MSGoBuffer_base);
static u32* regfile_physaddr    = ((u32 *) na_RegFile_base);
static int dcsc_msgbuf_in_size   = na_MSGiBuffer_size;
static int dcsc_msgbuf_out_size  = na_MSGoBuffer_size;
static int dcsc_regfile_size     = na_RegFile_size;
#else //INCLUDE_EXCALIBUR_H
// the default values can be changed at driver load time
static int dcsc_msgbuf_in_size   = 0x400;
static int dcsc_msgbuf_out_size  = 0x400;
static int dcsc_regfile_size     = 0x10;
static u32* msgbuf_in_physaddr  = ((u32 *) 0x80000400);
static u32* msgbuf_out_physaddr = ((u32 *) 0x80000800);
static u32* regfile_physaddr    = ((u32 *) 0x80000060);
#endif //INCLUDE_EXCALIBUR_H

static u32* msgbuf_in_virtbase=NULL;
static u32* msgbuf_out_virtbase=NULL;
static u32* regfile_virtbase=NULL;

static char g_logfilename[LOG_FILE_NAME_MAX_LENGTH]="";        


/*
 * define parameters which might be changed at driver load
 */
//MODULE_PARM(dcsc_majorID, "i");            // the major id for the installation of the driver
module_param(dcsc_majorID, int, S_IRUSR | S_IWUSR);
module_param(dcsc_msgbuf_in_size, int, S_IRUSR | S_IWUSR);
module_param(dcsc_msgbuf_out_size, int, S_IRUSR | S_IWUSR);// size of the MRB in byte
module_param(dcsc_regfile_size, int, S_IRUSR | S_IWUSR);   // size of the REG buffer in byte
module_param(msgbuf_in_physaddr, uint, S_IRUSR | S_IWUSR);  // physical address of the MIB in FPGA firmware
module_param(msgbuf_out_physaddr, uint, S_IRUSR | S_IWUSR); // physical address of the MRB in FPGA firmware
module_param(regfile_physaddr, uint, S_IRUSR | S_IWUSR);    // physical address of the REG buffer in FPGA firmware

/*
MODULE_PARM(dcsc_msgbuf_in_size, "i");     // size of the MIB in byte
MODULE_PARM(dcsc_msgbuf_out_size, "i");    // size of the MRB in byte
MODULE_PARM(dcsc_regfile_size, "i");       // size of the REG buffer in byte
MODULE_PARM(msgbuf_in_physaddr, "i");      // physical address of the MIB in FPGA firmware
MODULE_PARM(msgbuf_out_physaddr, "i");     // physical address of the MRB in FPGA firmware
MODULE_PARM(regfile_physaddr, "i");        // physical address of the REG buffer in FPGA firmware
*/

/*
 * access modes not yet used
 * in ACCESS_ALL mode the driver is forseen to export the 3 buffers sequentially after each other
 * MIB -> MRB -> REG
 * other modes are intended to support separated access via different minor driver numbers
 */
static int iAccessMode=ACCESS_ALL;

MODULE_AUTHOR("Matthias Richter");
MODULE_DESCRIPTION("dcs-card register driver");
MODULE_LICENSE("GPL");

/**************************************************************************************************
 * some helper functions to check the address space
 */
// very basic MEMTEST
// size in u8, test in u32
static int memtest(u32 begin, u32 size) {
  u32 i,rd;
  int ret=0;
  int success=0;
  for (i=0;i<size;i+=4) {
    writel(i+1, begin + i);
  }

  for (i=0;i<size;i+=4) {
    rd=readl(begin + i);
    if (  rd != i+1 ) {
      //mrlogmessage(LOG_SELFTEST|LOG_WARNING, KERN_WARNING "Read fail @ 0x%08X: read 0x%08X, expected 0x%08X",begin+i,rd,i);
      ret++;
    } else {
      success++;
      //mrlogmessage(LOG_SELFTEST|LOG_DBG, MR_KERN_DEBUG "Read succeeded @ 0x%08X: read 0x%08X, expected 0x%08X",begin+i,rd,i+1);
    }
  }
  //mrlogmessage(LOG_SELFTEST|LOG_DBG, MR_KERN_DEBUG " %d read cycles succeeded", success);
  return ret;
}

static int dcs_comparevalue(u32 begin, u32 size, u32 value) {
  u32 i,rd;
  int ret=0;
  for (i=0;i<size;i+=4) {
    rd=readl(begin + i);
    if (rd != value ) {
      //mrlogmessage(LOG_SELFTEST|LOG_WARNING, KERN_WARNING " compare failed @ 0x%08X: read 0x%08X, expected 0x%08X",begin+i,rd,value);
      ret++;
    } else {
      //mrlogmessage(LOG_SELFTEST|LOG_DBG, KERN_INFO " compare succeeded @ 0x%08X: read 0x%08X, expected 0x%08X",begin+i,rd,value);
    }
  }
  return ret;
}

static int dcs_writevalue (u32 begin, u32 size, u32 value) {
   int i;
   u32 offs=0;
   for (i=0;i<size;i+=4){
     writel(value,begin+i);
     offs++;
   }
   return 0;
}

/**************************************************************************************************
 * basic internal functions
 */

/* read from memory
 * the readl and readb functions are provided by the system and must be used to read from 
 * io memory initialized by ioremap (see init_module)
 * parameters:
 *  begin: start address
 *  size:  size of the buffer in BYTE
 *  buff:  pointer to buffer that receives data
 */
static int dcs_read (u32 begin, u32 size, u32* buff) 
{
  int i;
  u32 wordoffs=0;
  u32 byteoffs=0; 
  for (i=0;i<size/4;i++){ // do the multiples of 4 
    *(buff+wordoffs) = readl(begin+i*4);
    wordoffs++;    
  }
  i*=4;
  for (;i<size;i++) { // do the remaining bytes
    *(((char*)(buff+wordoffs))+byteoffs)=readb(begin+i);
    byteoffs++;
  }
  return 0;
}

/* write to memory
 * the writel and writeb functions are provided by the system and must be used to write to 
 * io memory initialized by ioremap (see init_module)
 * parameters:
 *  begin: start address
 *  size:  size of the buffer in BYTE
 *  buff:  pointer to buffer to write
 */
static int dcs_write (u32 begin, u32 size, u32* buff) 
{
  int i;
  u32 wordoffs=0;
  u32 byteoffs=0; 

  for (i=0;i<size/4;i++){// do the multibles of 4
    writel(*(buff+wordoffs),begin+i*4);
    wordoffs++;
  }
  i*=4;
  for (;i<size;i++) { // do the remaining bytes
    writeb(*(((char*)(buff+wordoffs))+byteoffs), begin+i);
    byteoffs++;
  }
  return 0;
}

/* find the buffer for the access region 
the access region appears to the user as a solid block containing the write buffer of dcsc_msgbuf_in_size bytes, 
followed by the read message buffer of dcsc_msgbuf_out_size and the register buffer of dcsc_regfile_size bytes
parameter: size of the buffer, <0 in case of an error
 offset       - position in the access region
 iAccessMode  - access mode, see defines
return: 
 ppBuffer     - receives pointer to buffer
 pPosition    - receives position in the buffer
 ppBufferName - receives string describing the buffer
 */
u32 findBufferForAddress(loff_t offset, int iAccessMode,  u32** ppBuffer, u32* pPosition, const char** ppBufferName)
{
  u32 u32Size=0, u32Pos=0;
  u32* pu32Buffer=NULL;
  const char* pBufferName="";
  if (offset < dcsc_msgbuf_in_size) {
    if (iAccessMode&ACCESS_IN_BUFFER /*|| iAccessMode==ACCESS_REGIONS*/) {
      u32Pos=offset;
      pu32Buffer=msgbuf_in_virtbase;
      u32Size=dcsc_msgbuf_in_size;
      pBufferName="write msg buffer";
    } else {
      //mrlogmessage(LOG_FLAG_ALL, KERN_WARNING "");
      u32Size=-EFAULT;
    }
  } else if (offset >= dcsc_msgbuf_in_size && offset < dcsc_msgbuf_in_size+dcsc_msgbuf_out_size) {
    if (iAccessMode&ACCESS_OUT_BUFFER /*|| iAccessMode==ACCESS_REGIONS*/) {
      u32Pos=offset-dcsc_msgbuf_in_size;
      pu32Buffer=msgbuf_out_virtbase;
      u32Size=dcsc_msgbuf_out_size;
      pBufferName="read msg buffer";
    } else {
      //mrlogmessage(LOG_FLAG_ALL, KERN_WARNING "");
      u32Size=-EFAULT;
    }
  } else if (offset >= dcsc_msgbuf_in_size + dcsc_msgbuf_out_size && offset < dcsc_msgbuf_in_size + dcsc_msgbuf_out_size + dcsc_regfile_size) {
    if (iAccessMode&ACCESS_REGFILE /*|| iAccessMode==ACCESS_REGIONS*/) {
      u32Pos=offset - dcsc_msgbuf_in_size - dcsc_msgbuf_out_size;
      pu32Buffer=regfile_virtbase;
      u32Size=dcsc_regfile_size;
      pBufferName="register buffer";
    } else {
      //mrlogmessage(LOG_FLAG_ALL, KERN_WARNING "");
      u32Size=-EFAULT;
    }
  } else
    u32Size=-ESPIPE;
  if (u32Size>=0) {
    if (u32Size<=u32Pos)
      u32Size-=u32Pos;
    else
      mrlogmessage(LOG_FLAG_ALL, KERN_ERR "buffer position exceeds buffer size");
    if (ppBuffer) *ppBuffer=pu32Buffer;
    if (pPosition) *pPosition=u32Pos;
    else if (ppBuffer) mrlogmessage(LOG_FLAG_ALL, KERN_WARNING "return variable missing for position in buffer, check your call");
    if (ppBufferName) *ppBufferName=pBufferName;
  }
  return u32Size;
}

/******************************************************************************************************
 * driver lock
 */

/* global variables
 */
wait_queue_head_t g_waitQueue;  // the wait queue for the concurrenting processes
int g_iLockReset=0;             // an emergency back door, all released processes terminate lockDriver()
int g_iLock=0;                  // the lock itself
int g_iLockPid=0;               // the pid of the locking process
int g_iSeizeCode=0;            // the master lock code
spinlock_t g_spinlock;
int g_iLockInitialized=0;

int InitDriverLock(void)
{
  int iResult=0;
  init_waitqueue_head(&g_waitQueue);
  spin_lock_init(&g_spinlock);
  g_iLockReset=0;
  g_iLock=0;
  g_iLockPid=0;
  g_iSeizeCode=0;
  g_iLockInitialized=1;
  return iResult;
}

int lockActivate(void)
{
  int iResult=0;
  g_iLockReset=0;
  return iResult;
}

int lockReset(void)
{
  int iResult=0;
  g_iLockReset=1; // set the flag so that the processes do not try to lock again
  g_iLock=0;
  g_iLockPid=0;
  g_iSeizeCode=0;
  if (g_iLockInitialized) {
    wake_up(&g_waitQueue); // wake up all processes
  }
  return iResult;
}

/**
 * spin-lock protected check of the lock status variable.
 * If the lock is free it will be set. If the lock is owned by the
 * current process the lock is incremented. The driver can be
 * <i>master</i> locked by an application which causes the lock to
 * return <i>permission denied</i> for requests by other applications.
 * @param iSeizeCode   master lock code given by the application
 * @param iTimeOut      time out (currently not used)
 * @return 1 if free and now set, 0 if already blocked<br>
 *         -ETIMEDOUT   if time out<br>
 *         -EPERM       driver in master lock
 */
int checkAndLock(int iSeizeCode, int pid)
{
  int iResult=0;
  if (g_iLockInitialized) {
    //mrlogmessage(LOG_DRIVER_LCK|LOG_DBG, MR_KERN_DEBUG "checkAndLock: process %d, iSeizeCode %d, g_iSeizeCode %d", pid, iSeizeCode, g_iSeizeCode);    
    spin_lock(&g_spinlock);
    if (g_iSeizeCode>0 && g_iSeizeCode!=iSeizeCode) {
      mrlogmessage(LOG_DRIVER_LCK|LOG_DBG, MR_KERN_DEBUG "checkAndLock: driver seized by other application");    
      iResult=-EPERM;
    } else if ((iResult=(g_iLock==0))) {
      g_iLock++;
      g_iLockPid=pid;
    } else if ((iResult=(g_iLockPid==pid))) {
      g_iLock++;
    }
    spin_unlock(&g_spinlock);
  }
  return iResult;
}

/**
 * Try to lock the driver, go to sleep if already locked.
 * After the process got woken up it tries again to lock.
 * @param iSeizeCode   master lock code given by the application
 * @param iTimeOut      time out (currently not used)
 * @return 1 if free and now set, 0 if already blocked<br>
 *         -ETIMEDOUT   if time out<br>
 *         -EPERM       driver in master lock
 */
int lockDriver(int iSeizeCode, int iTimeOut)
{
  int iResult=0;
  if (g_iLockInitialized) {
    int iPid=0;
    if (current) iPid=current->pid;
    while (g_iLockReset==0 && (iResult=checkAndLock(iSeizeCode, iPid))==0) {
      mrlogmessage(LOG_DRIVER_LCK|LOG_DBG, MR_KERN_DEBUG "lockDriver: process %d, timeout %d put to sleep", iPid, iTimeOut);
      interruptible_sleep_on(&g_waitQueue);
      mrlogmessage(LOG_DRIVER_LCK|LOG_DBG, MR_KERN_DEBUG "lockDriver: process %d woken up", iPid);
    }
    if (iResult>=0) {
      mrlogmessage(LOG_DRIVER_LCK|LOG_DBG, MR_KERN_DEBUG "lockDriver: process %d", iPid);
    }
  }
  return iResult;
}

/**
 * Unlock the driver and wake up all sleeping processes.
 */
int unlockDriver(void)
{
  int iResult=0;
  if (g_iLockInitialized) {
    int iPid=0;
    if (current) iPid=current->pid;
    mrlogmessage(LOG_DRIVER_LCK|LOG_DBG, MR_KERN_DEBUG "unlockDriver: process %d", iPid);
    spin_lock(&g_spinlock);
    if (g_iLockPid==iPid) {
      if ((--g_iLock)==0) {
	g_iLockPid=0;
	wake_up(&g_waitQueue); // wake up all processes
      } else if (g_iLock<0) {
	g_iLock=0;
	mrlogmessage(LOG_DRIVER_LCK|LOG_ERROR, KERN_ERR "underflow while unlocking for process %d", iPid);
      }
    } else {
      mrlogmessage(LOG_DRIVER_LCK|LOG_WARNING, KERN_WARNING "process %d has no lock on driver", iPid);
      iResult=-ENOLCK;
    }
    spin_unlock(&g_spinlock);
  }
  return iResult;
}

/**
 * Lock the driver to a single application.
 * The normal lock works for the different treads of that application,
 * but no other application has the right to access.
 * @param iSeizeCode     a code to identify the master application
 * @return >=0 if success, neg. error code if failed<br>
 *         -EPERM driver seized by another application
 */
int seizeDriver(int iSeizeCode)
{
  int iResult=0;
  if ((iResult=lockDriver(iSeizeCode, 0))>=0) {
    if (g_iSeizeCode==0) {
      mrlogmessage(LOG_DRIVER_LCK|LOG_INFO, KERN_INFO "driver seized by process %d", current->pid);
      g_iSeizeCode=iSeizeCode;
    } else if (g_iSeizeCode==iSeizeCode) {
      mrlogmessage(LOG_DRIVER_LCK|LOG_WARNING, KERN_WARNING "driver already seized by application %d", current->pid);
    } else {
      // we should never get here
      mrlogmessage(LOG_DRIVER_LCK|LOG_ERROR, KERN_ERR "driver already seized by another application", current->pid);
    }
    unlockDriver();
  }
  return iResult;
}



/**
 * Release the driver which was locked to a single application.
 * The is the pendant to @ref seizeDriver.
 * @param iSeizeCode   master lock code given by the application
 * @return >=0 if success, neg. error code if failed
 */
int releaseDriver(int iSeizeCode)
{
  int iResult=0;
  if ((iResult=lockDriver(iSeizeCode, 0))>=0) {
    if (g_iSeizeCode==0) {
      mrlogmessage(LOG_DRIVER_LCK|LOG_ERROR, KERN_ERR "driver not seized by process %d", current->pid);
      iResult=-ENOLCK;
    } else if (g_iSeizeCode==iSeizeCode) {
      mrlogmessage(LOG_DRIVER_LCK|LOG_INFO, KERN_INFO "seized driver released by application %d", current->pid);
      g_iSeizeCode=0;
    } else {
      // we should never get here
      mrlogmessage(LOG_DRIVER_LCK|LOG_ERROR, KERN_ERR "driver is seized by another application", current->pid);
    }
    unlockDriver();
  }
  return iResult;
}

/******************************************************************************************************
 * driver initialization and cleanup
 */

/*
 * method prototypes see below for implementations
 */
static loff_t dcsc_llseek(struct file* filp, loff_t off, int ref);
static int dcsc_read(struct file* filp, char* buf, size_t count, loff_t* f_pos);
static int dcsc_write(struct file* filp, const char* buf, size_t count, loff_t* f_pos);
static int dcsc_open(struct inode* inode, struct file* filp);
static int dcsc_close(struct inode* inode, struct file* filp);
static int dcsc_mmap(struct file *filp, struct vm_area_struct *vma);
static int dcsc_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg);

/*
 * file_operation structure to define the methods for the char device
 * the methods are going to be called from the system
 * pointers to the methods are provided at driver initialization via the file_operations structure
 */
struct file_operations dcsc_fops = {
  llseek:  dcsc_llseek,
  read:    dcsc_read,
  write:   dcsc_write,
  ioctl:   dcsc_ioctl,
  mmap:    dcsc_mmap,
  open:    dcsc_open,
  release: dcsc_close,
};

#ifndef DCSC_TEST
/*
 * cleanup of the buffers for the real driver
 */
void cleanupRealBuffers(void)
{
  if (regfile_virtbase) iounmap((void *)regfile_virtbase);
  if (msgbuf_out_virtbase) iounmap((void *)msgbuf_out_virtbase);
  if (msgbuf_in_virtbase) iounmap((void *)msgbuf_in_virtbase);
}

/*
 * initialization of the buffers for the real driver
 */
#define TEST_PATTERN 0x13071972
int initRealBuffers(void)
{
  int iResult=0;
  int iNofErr=0;
  if (dcsc_msgbuf_in_size>0) {
    msgbuf_in_virtbase  = (u32*) ioremap_nocache((u32)msgbuf_in_physaddr,dcsc_msgbuf_in_size);
    mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Remapped MSGBUF_IN from 0x%p to 0x%p",msgbuf_in_physaddr, msgbuf_in_virtbase);
  }
  if (dcsc_msgbuf_out_size>0) {
    msgbuf_out_virtbase = (u32*) ioremap_nocache((u32)msgbuf_out_physaddr,dcsc_msgbuf_out_size);
    mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Remapped MSGBUF_OUT from 0x%p to 0x%p",msgbuf_out_physaddr, msgbuf_out_virtbase);
  }
  if (dcsc_regfile_size) {
    regfile_virtbase    = (u32*) ioremap_nocache((u32)regfile_physaddr,dcsc_regfile_size);
    mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Remapped REGFILE from 0x%p to 0x%p",regfile_physaddr, regfile_virtbase);
  }
  if (regfile_virtbase && msgbuf_in_virtbase && msgbuf_out_virtbase){

    // test the MIB, the MRB is readonly and can not be tested
    writel(0x40, regfile_virtbase); // set bit 6 in the control register which enables the MIB to be read/write
    iNofErr=memtest((u32) msgbuf_in_virtbase , dcsc_msgbuf_in_size);
    if (iNofErr==0) {
      //mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "first test succeeded - buffer size = %d errors = %d", dcsc_msgbuf_in_size, iNofErr);
      dcs_writevalue((u32)msgbuf_in_virtbase, dcsc_msgbuf_in_size, TEST_PATTERN);
      if ((iNofErr=dcs_comparevalue((u32)msgbuf_in_virtbase, dcsc_msgbuf_in_size, TEST_PATTERN))==0) {
	writel(0x00, regfile_virtbase); // now set the buffer to write only
	iNofErr=dcs_comparevalue((u32)msgbuf_in_virtbase, dcsc_msgbuf_in_size, TEST_PATTERN);

	if (iNofErr==dcsc_msgbuf_in_size) { // the test should fail now
	  iNofErr=0;
	} else {
	  mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "some of the locations are still read/writeable, but the buffer is supposed to be write only ");
	  if (iNofErr>0)
	    iNofErr=0;
	}
      } else {
	mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "second test of MIB failed - buffer size = %d errors = %d", dcsc_msgbuf_in_size, iNofErr);
      }
    } else {
      mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "first test of MIB failed - buffer size = %d errors = %d", dcsc_msgbuf_in_size, iNofErr);
    }
    if (iNofErr>0) {
      mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "memtest finished with %d error(s), eventually check your parameters", iNofErr);
      iResult=-EIO;
    } else {
      mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "message buffer found and tested(test pattern 0x%x)", TEST_PATTERN);
    }
  } else if ((regfile_virtbase==NULL && dcsc_regfile_size>0) ||
	     (msgbuf_in_virtbase==NULL && dcsc_msgbuf_in_size>0) ||
	     (msgbuf_out_virtbase==NULL && dcsc_msgbuf_out_size>0) 
	     ){
    mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "can not remap hardware resources");
    iResult=-EIO;
  }
  if (iResult<0) {
    mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "can not initialize memory areas");
    cleanupRealBuffers();
  }
  return iResult;
}

#else //DCSC_TEST
/*
 * cleanup of the buffers for the simulated driver
 */
void cleanupSimulatedBuffers(void)
{
  if (msgbuf_in_virtbase) kfree(msgbuf_in_virtbase);
  if (msgbuf_out_virtbase) kfree(msgbuf_out_virtbase);
  if (regfile_virtbase) kfree(regfile_virtbase);
}

/*
 * initialization of the buffers for the simulated driver
 */
int initSimulatedBuffers(void)
{
  int iResult=0;
  int iNofErr=0;
  mrlogmessage(LOG_INFO, KERN_INFO "this is a test version");
  if (dcsc_msgbuf_in_size>0)
    msgbuf_in_virtbase  = (u32*) kmalloc(dcsc_msgbuf_in_size, GFP_KERNEL);
  if (dcsc_msgbuf_out_size>0)
    msgbuf_out_virtbase = (u32*) kmalloc(dcsc_msgbuf_out_size, GFP_KERNEL);
  if (dcsc_regfile_size)
    regfile_virtbase    = (u32*) kmalloc(dcsc_regfile_size, GFP_KERNEL);
  if (msgbuf_in_virtbase) mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Allocated MIB at 0x%p %d bytes", msgbuf_in_virtbase, dcsc_msgbuf_in_size);
  if (msgbuf_out_virtbase) mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Allocated MRB at 0x%p %d bytes", msgbuf_out_virtbase, dcsc_msgbuf_out_size);
  if (regfile_virtbase) mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Allocated REG at 0x%p %d bytes", regfile_virtbase, dcsc_regfile_size);
  if (msgbuf_in_virtbase){
    iNofErr=memtest( (u32) msgbuf_in_virtbase , dcsc_msgbuf_in_size);
    dcs_writevalue((u32)msgbuf_in_virtbase , dcsc_msgbuf_in_size, 0);
    mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Test MIB: %d errors",iNofErr);
  } else {
    mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "Allocation of MIB failed");
  }

  if (msgbuf_out_virtbase){
    iNofErr=memtest( (u32) msgbuf_out_virtbase , dcsc_msgbuf_out_size);
    dcs_writevalue((u32)msgbuf_out_virtbase , dcsc_msgbuf_out_size, 0);
    mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Test MRB: %d errors",iNofErr);
  } else {
    mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "Allocation of MRB failed");
  }

  if (regfile_virtbase){
    iNofErr=memtest( (u32) regfile_virtbase , dcsc_regfile_size);
    dcs_writevalue((u32)regfile_virtbase , dcsc_regfile_size, 0);
    mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Test REG: %d errors",iNofErr);
  } else {
    mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "Allocation of REG failed");
  }
  return iResult;
}
#endif //DCSC_TEST

/*
 * init_module called from the system during insmod command
 */
int init_module(void) {
  // TODO check for stuff to cleanup in case of error and undesired termination
  int iResult=0;

  // originally the driver sizes of the buffers were checked at driver load. This
  // has been removed here in order to have a multi-purpose driver which allows
  // access to arbitrary 3 buffers.
  // The buffer sizes are check in the dcscMsgBufferInterface anyway.

  // register the driver
  iResult = register_chrdev(dcsc_majorID, "dcsc", &dcsc_fops);
  if (iResult < 0) {
    mrlogmessage(LOG_ERROR, KERN_ERR "module init: can't register driver");
  } else {
    mrlogmessage(LOG_INFO, KERN_INFO "Module init (compiled "__DATE__", "__TIME__") version %d.%d - %s", DRIVER_MAJOR_VERSION_NUMBER, DRIVER_MINOR_VERSION_NUMBER, RELEASETYPE);
    if (dcsc_majorID == 0) { /* dynamic allocation of major number*/
      dcsc_majorID = iResult; 
      mrlogmessage(LOG_INFO, KERN_INFO "dynamic allocation of major number: %d", dcsc_majorID);
    } else {
      mrlogmessage(LOG_INFO, KERN_INFO "register dcscard driver at major no. %d", dcsc_majorID);
    }
#ifndef DCSC_TEST
    iResult=initRealBuffers();
#else //!DCSC_TEST
    iResult=initSimulatedBuffers();
#endif //DCSC_TEST
    InitDriverLock();
  }

  return iResult;
}

/*
 * init_module called from the system during rmmod command
 */
void cleanup_module(void) { 
  mrlogmessage(LOG_INFO, KERN_INFO "Module exit");

#ifndef DCSC_TEST
  cleanupRealBuffers();
#else //DCSC_TEST
  cleanupSimulatedBuffers();
#endif //DCSC_TEST

  unregister_chrdev(dcsc_majorID, "dcsc");
}

/******************************************************************************************************
 * implementation of driver access methods defined above
 */

static loff_t dcsc_llseek(struct file* filp, loff_t off, int ref)
{
  loff_t lPosition=0;
  if (filp){
    // by now the ref parameter is ignored, seek always from beginning
    lPosition=off;
    mrlogmessage(LOG_LLSEEK|LOG_DBG, MR_KERN_DEBUG "llseek: position=%lld, offset=%lld, new position=%lld", filp->f_pos, off, lPosition);
  } else {
    mrlogmessage(LOG_LLSEEK|LOG_ERROR, KERN_ERR "llseek: invalid file pointer");
    lPosition=-EFAULT;
  }
  if (lPosition>=0) filp->f_pos=lPosition;
  return lPosition;
}

static int dcsc_read(struct file* filp, char* buf, size_t count, loff_t* f_pos)
{
  int iResult=0;
  const char* pBufferName="<not specified name>";
  u32 lPos=0;
  u32 u32Offset=0;
  u32 u32Count=0;
  u32* pu32Source=NULL;

  if (filp==NULL) {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: invalid file pointer");
    return -EFAULT;
  }

  lPos=filp->f_pos;

  if (filp && buf && count>=0 && lPos>=0){
    //mrlogmessage(LOG_READ_WRITE|LOG_DBG, MR_KERN_DEBUG "read: count=%d f_pos=0x%08x", count, lPos);
    mrlogmessage(LOG_INFO, MR_KERN_DEBUG "read: count=%d f_pos=0x%08x", count, lPos);
    if (iAccessMode==ACCESS_ALL) {
      if (lPos < dcsc_msgbuf_in_size) {
	if (count + lPos <= dcsc_msgbuf_in_size) {
	  u32Count=count;
	} else {
	  u32Count=dcsc_msgbuf_in_size;
	  mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "read: read access exceeds size of msg_in buffer, truncate");
	}
	u32Offset=lPos;
	pu32Source=msgbuf_in_virtbase;
	pBufferName="write msg buffer";
      } else if (lPos >= dcsc_msgbuf_in_size && lPos < dcsc_msgbuf_in_size+dcsc_msgbuf_out_size) {
	if (count + lPos - dcsc_msgbuf_in_size <= dcsc_msgbuf_out_size) {
	  u32Count=count;
	} else {
	  u32Count=dcsc_msgbuf_out_size;
	  mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "read: read access exceeds size of msg_out buffer, truncate");
	}
	u32Offset=lPos - dcsc_msgbuf_in_size;
	pu32Source=msgbuf_out_virtbase;
	pBufferName="read msg buffer";
      } else if (lPos >= dcsc_msgbuf_in_size + dcsc_msgbuf_out_size && lPos < dcsc_msgbuf_in_size + dcsc_msgbuf_out_size + dcsc_regfile_size) {
	if (count + lPos - dcsc_msgbuf_in_size - dcsc_msgbuf_out_size <= dcsc_regfile_size) {
	  u32Count=count;
	} else {
	  mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "read: read access exceeds size of register buffer, read maximum");
	  u32Count=dcsc_regfile_size;
	}
	u32Offset=lPos - dcsc_msgbuf_in_size - dcsc_msgbuf_out_size;
	pu32Source=regfile_virtbase;
	pBufferName="register buffer";
      }
    } else {
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: access mode not yet implemented");
    }
    if (pu32Source!=NULL) {
      // to be deleted u32Offset*=sizeof(size_t);
      u32Offset/=sizeof(u32);
      pu32Source+=u32Offset;
      //mrlogmessage(LOG_READ_WRITE|LOG_DBG, MR_KERN_DEBUG "read: reading %d byte(s) from %s at 0x%8p to buffer 0x%8p", u32Count, pBufferName, pu32Source, buf);
      //mrlogmessage(LOG_INFO, MR_KERN_DEBUG "read: reading %d byte(s) from %s at 0x%8p to buffer 0x%8p", u32Count, pBufferName, pu32Source, buf);
      mrlogmessage(LOG_INFO, KERN_INFO "read: reading %d byte(s) from %s at 0x%x to buffer 0x%x", u32Count, pBufferName, pu32Source, buf);
      dcs_read((u32)pu32Source, u32Count, (u32*)buf);
      iResult=(int)u32Count;
      if (f_pos) *f_pos=filp->f_pos+iResult;
      else mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "read: invalid parameter f_pos, skipping position increment");
    } else {
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: read access denied");
      iResult=-EFAULT;
    }
  } else {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: invalid parameter");
    iResult=-EINVAL;
  }
  return iResult;
}

static int dcsc_write(struct file* filp, const char* buf, size_t count, loff_t* f_pos)
{
  int iResult=0;
  const char* pBufferName="<not specified name>";
  u32 lPos=0;
  u32 u32Offset=0;
  u32 u32Count=0;
  u32* pu32Target=NULL;

  if (filp==NULL) {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: invalid file pointer");
    return -EFAULT;
  }
  lPos=filp->f_pos;
  if (filp && buf && count>=0 && lPos>=0){
    //mrlogmessage(LOG_READ_WRITE|LOG_DBG, MR_KERN_DEBUG "write: count=%d f_pos=0x%08x", count, lPos);
    mrlogmessage(LOG_INFO, KERN_INFO "write: count=%d f_pos=0x%08x", count, lPos);
    //mrlogmessage(LOG_INFO, MR_KERN_DEBUG "write: count=%d f_pos=0x%08x", count, lPos);
    if (iAccessMode==ACCESS_ALL) {
      if (lPos < dcsc_msgbuf_in_size) {
	if (count + lPos <= dcsc_msgbuf_in_size) {
	  u32Offset=lPos;
	  u32Count=count;
	  pu32Target=msgbuf_in_virtbase;
	  pBufferName="write msg buffer";
	} else {
	  mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "write: write access exceeds size of msg_in buffer");
	}
      } else if (lPos >= dcsc_msgbuf_in_size && lPos < dcsc_msgbuf_in_size+dcsc_msgbuf_out_size) {
	if (count + lPos - dcsc_msgbuf_in_size <= dcsc_msgbuf_out_size) {
	  u32Offset=lPos - dcsc_msgbuf_in_size;
	  u32Count=count;
	  pu32Target=msgbuf_out_virtbase;
	  pBufferName="read msg buffer";
	} else {
	  mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "write: write access exceeds size of msg_out buffer");
	}
      } else if (lPos >= dcsc_msgbuf_in_size + dcsc_msgbuf_out_size && lPos < dcsc_msgbuf_in_size + dcsc_msgbuf_out_size + dcsc_regfile_size) {
	if (count + lPos - dcsc_msgbuf_in_size - dcsc_msgbuf_out_size <= dcsc_regfile_size) {
	  u32Offset=lPos - dcsc_msgbuf_in_size - dcsc_msgbuf_out_size;
	  u32Count=count;
	  pu32Target=regfile_virtbase;
	  pBufferName="register buffer";
	} else {
	  mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "write: write access exceeds size of register buffer");
	}
      }
    } else {
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: access mode not yet implemented");
    }
    if (pu32Target!=NULL) {
      // to be deleted u32Offset*=sizeof(size_t);
      u32Offset/=sizeof(u32);
      pu32Target+=u32Offset;
      //mrlogmessage(LOG_READ_WRITE|LOG_DBG, MR_KERN_DEBUG "write: writing %d byte(s) from buffer 0x%8p to %s at 0x%8p", u32Count, (u32*)buf, pBufferName, pu32Target);
      //  mrlogmessage(LOG_INFO, MR_KERN_DEBUG "write: writing %d byte(s) from buffer 0x%8p to %s at 0x%8p", u32Count, (u32*)buf, pBufferName, pu32Target);
      mrlogmessage(LOG_INFO, KERN_INFO "write: writing %d byte(s) from buffer 0x%8p to %s at 0x%8p", u32Count, (u32*)buf, pBufferName, pu32Target);
      dcs_write((u32)pu32Target, u32Count, (u32*)buf);
      iResult=(int)u32Count;
      if (f_pos) *f_pos=filp->f_pos+iResult;
      else mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "write: invalid parameter f_pos, skipping position increment");
    } else {
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: write access denied");
	iResult=-EFAULT;
    }
  } else {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: invalid parameter");
    iResult=-EINVAL;
  }
  return iResult;
}

static int dcsc_open(struct inode* inode, struct file* filp)
{
  int iResult=0;
  if ((regfile_virtbase==NULL && dcsc_regfile_size>0) ||
      (msgbuf_in_virtbase==NULL && dcsc_msgbuf_in_size>0) ||
      (msgbuf_out_virtbase==NULL && dcsc_msgbuf_out_size>0) 
      ){
    mrlogmessage(LOG_OPEN_CLOSE|LOG_ERROR, KERN_CRIT "open: panic, some base addresses not initialized");
    iResult=-EFAULT;
  } else {
    mrlogmessage(LOG_OPEN_CLOSE|LOG_INFO, MR_KERN_DEBUG "open Device!");
  }
  return iResult;
}

static int dcsc_close(struct inode* inode, struct file* filp)
{
  int iResult=0;
  mrlogmessage(LOG_OPEN_CLOSE|LOG_DBG, MR_KERN_DEBUG "close");
  return iResult;
}

static int dcsc_mmap(struct file* filp, struct vm_area_struct* vma)
{
  int iResult=0;
  mrlogmessage(LOG_MMAP|LOG_DBG, MR_KERN_DEBUG "mmap: not yet implemented");
  return iResult;
}

static int dcsc_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
  int iResult=0;
  if (filp==NULL || inode==NULL) {
    mrlogmessage(LOG_IOCTL|LOG_ERROR, KERN_ERR "ioctl: invalid file/inode pointer");
    return -EFAULT;
  }
  if (cmd>=IOCTL_READ_REG && cmd<IOCTL_READ_REG+dcsc_regfile_size) {
    int iReg=cmd-IOCTL_READ_REG;
    iResult=*(((char*)regfile_virtbase)+iReg);
    mrlogmessage(LOG_IOCTL|LOG_DBG, KERN_INFO "ioctl: read register %d = %#x", iReg, iResult);
  } else if (cmd>=IOCTL_WRITE_REG && cmd<IOCTL_WRITE_REG+dcsc_regfile_size) {
    int iReg=cmd-IOCTL_READ_REG;
    if (arg>=256) {
      mrlogmessage(LOG_IOCTL|LOG_INFO, MR_KERN_DEBUG "ioctl: attempt to write register %d with argument larger than one byte, msbs will be ignored", iReg);
      arg&=0xff;
    }
    *(((char*)regfile_virtbase)+iReg)=arg;
  } else switch (cmd) {
  case IOCTL_GET_MSGBUF_IN_SIZE:
    *((u32*)arg)=dcsc_msgbuf_in_size;
    iResult=dcsc_msgbuf_in_size;
    break;
  case IOCTL_GET_MSGBUF_OUT_SIZE:
    *((u32*)arg)=dcsc_msgbuf_out_size;
    iResult=dcsc_msgbuf_out_size;
    break;
  case IOCTL_GET_REGFILE_SIZE:
    *((u32*)arg)=dcsc_regfile_size;
    iResult=dcsc_regfile_size;
    break;
  case IOCTL_GET_VERSION_V02:
  case IOCTL_GET_VERSION:
    {
      if (arg!=0) {
	char versionstring[VERSION_STRING_SIZE];
	int iLen=snprintf(versionstring, sizeof(versionstring), "%d.%d - %s", DRIVER_MAJOR_VERSION_NUMBER, DRIVER_MINOR_VERSION_NUMBER, RELEASETYPE);
	int iRet = 0;
	if (iLen>0) {
	  iRet = copy_to_user((void*)arg, versionstring, iLen);
	} else {
	  iRet = copy_to_user((void*)arg, "", 1);
	}
      }
      iResult=(DRIVER_MAJOR_VERSION_NUMBER << 16) + DRIVER_MINOR_VERSION_NUMBER;
    }
    break;
  case IOCTL_GET_VERS_STR_SIZE:
    iResult=VERSION_STRING_SIZE;
    break;
  case IOCTL_SET_DEBUG_LEVEL:
    if (arg>0xffff)
      mrlogmessage(LOG_IOCTL|LOG_WARNING, KERN_WARNING "ioctl: log flags are 16 bit wide, skip MSBs of argument");
    arg&=0xffff;
    g_logflags=arg;
    break;
  case IOCTL_SET_LOG_FILE:
    if (arg>0) {
      if (strlen((char*)arg)<sizeof(g_logfilename)-1) {
	sprintf(g_logfilename, (char*)arg);
	mrlogmessage(LOG_IOCTL|LOG_DBG, MR_KERN_DEBUG "ioctl (0x%x): set logfile name to \'%s\'", cmd, g_logfilename);
      } else {
	mrlogmessage(LOG_IOCTL|LOG_ERROR, KERN_ERR "iotcl (0x%x): file name to long, maximum name length %d", cmd, sizeof(g_logfilename));
	iResult=-EINVAL;
      }
    } else {
      mrlogmessage(LOG_IOCTL|LOG_ERROR, KERN_ERR "iotcl (0x%x): invalid argument", cmd);
      iResult=-EINVAL;
    }
    break;
  case IOCTL_LOCK_DRIVER:
    iResult=lockDriver(arg, 0);
    break;
  case IOCTL_UNLOCK_DRIVER:
    //iResult=unlockDriver(arg);
    iResult=unlockDriver();
    break;
  case IOCTL_SEIZE_DRIVER:
    iResult=seizeDriver(arg);
    break;
  case IOCTL_RELEASE_DRIVER:
    iResult=releaseDriver(arg);
    break;
  case IOCTL_RESET_LOCK:
    iResult=lockReset();
    break;
  case IOCTL_ACTIVATE_LOCK:
    iResult=lockActivate();
    break;
  case IOCTL_TEST_MSGBUF_IN:
  case IOCTL_TEST_MSGBUF_OUT:
    mrlogmessage(LOG_IOCTL|LOG_INFO, KERN_INFO "ioctl: command not implemented");
    iResult=-ENOSYS;
    break;
  default:
    mrlogmessage(LOG_IOCTL|LOG_DBG, MR_KERN_DEBUG "ioctl: unknown command 0x%x", cmd);
  }
  return iResult;
}

