// $Id: rcu_flash.c,v 1.5 2006/06/15 08:33:01 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter, Johan Alme, Ketil Roed
** Matthias.Richter@ift.uib.no
** Johan.Alme@ift.uib.no
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

#include <linux/fs.h>      // basic file structures and methods
#include <linux/types.h>   // basic data types
#include <linux/errno.h>   // error codes
#include <linux/slab.h>    // kmalloc, kmfree
#include <asm/io.h>        // ioremap, ...
#include <asm/uaccess.h>   // user space memory access (e.g. copy_to_user)
#include <linux/ioport.h>  // detecting and reserving system resources
#include <asm/system.h>
#include <linux/module.h>
#ifdef ARM_UCLIB
#include <linux/mtd/mtd.h> // ioctl cmds and structs for eraseall command
#endif //ARM_UCLIB
#include "dcs_driver.h"
#include "rcu_flash.h"
#include "mrKernLogging.c"

/*
 * default major no of the driver if used standalone
 */
static int rcuf_majorID=151; /* fixed major no */

// the default values can be changed at driver load time
static int rcuf_size   = 0x400;

#ifndef DCSC_TEST
// the bus lines are directly set via registers
#define RCUF_ADDR_WIDTH  32
static int rcuf_addrlines_physaddr  = 0x80000090; // address bus

#define RCUF_DATA_WIDTH   8
static int rcuf_data_out_physaddr  = 0x80000030; // data bus
static int rcuf_data_in_physaddr  = 0x80000080; // data bus

#define RCUF_CTRL_WIDTH   8
static int rcuf_bus_ctrl_physaddr  = 0x80000070; // strobe signal
/* control lines
 * bit 7: direct bus access if (high active)
 * bit 6: flash (0) /selectmap (1)
 * bit 5: reset (high active) not yet used
 * bit 4: not used for flash
 * bit 3: out enable (low active)
 * bit 2: not used for flash
 * bit 1: write enable (low active)
 * bit 0: chip enable (low active)
 */
#define RCU_BUS_DIRECT_ACCESS_ACTIVATE 0x80
#define RCU_BUS_SELECTMAP_ACTIVATE     0x40
#define RCU_BUS_FLASH_RESET            0x20
#define RCU_BUS_FLASH_OE_INACTIVE      0x08
#define RCU_BUS_FLASH_WE_INACTIVE      0x02
#define RCU_BUS_FLASH_CE_INACTIVE      0x01
#define RCU_BUS_FLASH_INACTIVE_LINES   RCU_BUS_FLASH_OE_INACTIVE | RCU_BUS_FLASH_WE_INACTIVE | RCU_BUS_FLASH_CE_INACTIVE

#define RCU_BUS_READ_STROBE            0xa9
#define RCU_BUS_WRITE_STROBE           0xaa
#define RCU_BUS_FLASH_ACCESS           0xaf
#define RCU_BUS_DISABLE_DIRECT_ACCESS  0x2f

#define RCUF_START_FLASH_ACCESS() writel(RCU_BUS_FLASH_ACCESS, rcuf_virtctrl)
#define RCUF_STOP_FLASH_ACCESS()  writel(RCU_BUS_DISABLE_DIRECT_ACCESS, rcuf_virtctrl)
#define RCUF_SET_ADDRESS(addr)    writel(addr,rcuf_virtaddr)
#define RCUF_SET_DATA(data)       writeb(data,rcuf_virtdtao)
#define RCUF_GET_DATA()           readb(rcuf_virtdtai)
#define RCUF_READ_STROBE()        writel(RCU_BUS_READ_STROBE, rcuf_virtctrl)
#define RCUF_WRITE_STROBE()       writel(RCU_BUS_WRITE_STROBE, rcuf_virtctrl)
#define RCUF_STROBE_END()         writel(RCU_BUS_FLASH_ACCESS, rcuf_virtctrl)

// the pointers used after remapping of the resources
static u32* rcuf_virtaddr  = ((u32 *) 0x0); // address lines
static u32* rcuf_virtdtao  = ((u32 *) 0x0); // data lines write
static u32* rcuf_virtdtai  = ((u32 *) 0x0); // data lines read
static u32* rcuf_virtctrl  = ((u32 *) 0x0); // control lines

#else //DCSC_TEST
// test memory for the driver simulation
static u32* rcuf_virtbase = ((u32 *) 0x0);

static u32 rcuf_vaddr_sim    = 0;
static u32 rcuf_vdata_sim    = 0;

#define RCUF_START_FLASH_ACCESS()
#define RCUF_STOP_FLASH_ACCESS()
#define RCUF_SET_ADDRESS(addr) rcuf_vaddr_sim = addr
#define RCUF_SET_DATA(data) rcuf_vdata_sim = data
#define RCUF_GET_DATA() rcuf_vdata_sim
#define RCUF_READ_STROBE() rcuf_simulate_read(1)
#define RCUF_WRITE_STROBE() rcuf_simulate_write(1)
#define RCUF_STROBE_END()

#endif //DCSC_TEST

/*
 * define parameters which might be changed at driver load
 */
MODULE_PARM(rcuf_majorID, "i");            // the major id for the installation of the driver
MODULE_PARM(rcuf_size, "i");               // size of the flash in byte
#ifndef DCSC_TEST
MODULE_PARM(rcuf_addrlines_physaddr, "i");          // address bus
MODULE_PARM(rcuf_data_out_physaddr, "i");           // data bus write
MODULE_PARM(rcuf_data_in_physaddr, "i");            // data bus read
MODULE_PARM(rcuf_bus_ctrl_physaddr, "i");           // strobe
#endif //DCSC_TEST

MODULE_AUTHOR("Matthias Richter, Johan Alme, Ketil Roed");
MODULE_DESCRIPTION("rcu flash memory driver");
MODULE_LICENSE("GPL");

/******************************************************************************************************
 ******************************************************************************************************
 ******************************************************************************************************
 *********   P A R T 1 : internal functions                                                   *********
 ******************************************************************************************************
 ******************************************************************************************************/

/**************************************************************************************************
 * some helper functions to check the address space
 */
// very basic MEMTEST
// size in u8, test in u32
static int rcuf_memtest(u32 begin, u32 size) {
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

static int rcuf_writevalue (u32 begin, u32 size, u32 value) {
   int i=0;
   u32 offs=0;
   for (i=0;i<size;i+=4){
     writel(value,begin+i);
     offs++;
   }
   return i;
}

#ifndef DCSC_TEST
/**************************************************************************************************
 * basic internal functions for the real flash
 */

/* erase the flash memory
 * parameters:
 *  begin: start offset
 *  size:  size of the buffer in BYTE
 */
static int rcu_flash_erase (u32 begin, u32 size) 
{
  int iResult=size;
  mrlogmessage(LOG_INFO, KERN_INFO "flash erase not yet implemented");
  return iResult;
}

#else //!DCSC_TEST
/**************************************************************************************************
 * basic internal functions for the simulated driver
 * see function and parameter description above 
 */

static int rcu_flash_erase (u32 offset, u32 size) 
{
  int iResult=0;
  if (rcuf_virtbase!=NULL) {
    u32 address=(u32)rcuf_virtbase+offset;
    iResult=rcuf_writevalue(address, size, 0);
  } else {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "erase: write access denied");
    iResult=-EFAULT;
  }
  return iResult;
}

static int rcuf_simulate_read(int wordsize) 
{
  int iResult=0;
  if (rcuf_virtbase!=NULL) {
    u8* address=(u8*)rcuf_virtbase+rcuf_vaddr_sim;
    switch (wordsize) {
    case 4:
      rcuf_vdata_sim = *((u32*)address); //readl(address);
      iResult=wordsize;
      break;
    case 2:
      rcuf_vdata_sim = *((u16*)address); //readw(address);
      iResult=wordsize;
      break;
      break;
    case 1:
      rcuf_vdata_sim = *address; //readb(address);
      break;
    default:
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: invalid wordsize %d", wordsize);
      iResult=-EFAULT;
    }
  } else {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: write access denied");
    iResult=-EFAULT;
  }
  return iResult;
}

static int rcuf_simulate_write(int wordsize) 
{
  int iResult=0;
  if (rcuf_virtbase!=NULL) {
    u8* address=(u8*)rcuf_virtbase+rcuf_vaddr_sim;
    switch (wordsize) {
    case 4:
      //writel(rcuf_vdata_sim,address);
      *((u32*)address)=rcuf_vdata_sim;
      iResult=wordsize;
      break;
    case 2:
      {
	u16 u16data=rcuf_vdata_sim&0xffff;
	//writew(u16data,address);
	*((u16*)address)=u16data;
	iResult=wordsize;
      }
      break;
    case 1:
      {
	u8 u8data=rcuf_vdata_sim&0xff;
	//writeb(u8data,address);
	*((u8*)address)=u8data;
	iResult=wordsize;
      }
      break;
    default:
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: invalid wordsize %d", wordsize);
      iResult=-EFAULT;
    }
  } else {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: write access denied");
    iResult=-EFAULT;
  }
  return iResult;
}
#endif //DCSC_TEST


/**************************************************************************************************
 * basic internal functions
 */

/* read from memory
 * the readl and readb functions are provided by the system and must be used to read from 
 * io memory initialized by ioremap (see init_module)
 * parameters:
 *  begin: start offset
 *  size:  size of the buffer in BYTE
 *  buff:  pointer to buffer that receives data
 */
static int rcu_flash_read (u32 address, u32 size, u32* buff) 
{
  int iResult=0;
  int i=0;
  u32 wordoffs=0;
  if ( 1 
#ifndef DCSC_TEST
      && rcuf_virtaddr!=NULL && rcuf_virtdtai!=NULL && rcuf_virtctrl!=NULL
#endif //DCSC_TEST
      ) {
    /* later optimization
       for (i=0;i<size/4 && iResult>=1;i++){// do the multibles of 4
       RCUF_SET_ADDRESS(address+i*4);
       iResult=RCUF_READ_STROBE32();
       *(buff+wordoffs)=RCUF_SET_DATA32();
       RCUF_STROBE_END();
       wordoffs++;
       }
    */
    i*=4;
    u32 byteoffs=0; 
    for (;i<size && iResult>=0;i++) { // do the remaining bytes
      RCUF_SET_ADDRESS(address+i);
      /*iResult=*/RCUF_READ_STROBE();
      *(((char*)(buff+wordoffs))+byteoffs)=RCUF_GET_DATA();
      RCUF_STROBE_END();
      byteoffs++;
    }
    iResult=i;
  } else {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: driver not initialized");
    iResult=-EFAULT;
  }
  return iResult;
}

/* write to memory
 * the writel and writeb functions are provided by the system and must be used to write to 
 * io memory initialized by ioremap (see init_module)
 * parameters:
 *  begin: start offset
 *  size:  size of the buffer in BYTE
 *  buff:  pointer to buffer to write
 */
static int rcu_flash_write (u32 address, u32 size, u32* buff) 
{
  int iResult=0;
  int i=0;
  u32 wordoffs=0;
  if ( 1 
#ifndef DCSC_TEST
       && rcuf_virtaddr!=NULL && rcuf_virtdtao!=NULL && rcuf_virtctrl!=NULL
#endif //DCSC_TEST
       ) {
    /* later optimization
       for (i=0;i<size/4 && iResult>=1;i++){// do the multibles of 4
       RCUF_SET_ADDRESS(address+i*4);
       RCUF_SET_DATA32(*(buff+wordoffs));
       iResult=RCUF_WRITE_STROBE32();
       RCUF_STROBE_END();
       wordoffs++;
       }
    */
    i*=4;
    u32 byteoffs=0; 
    for (;i<size && iResult>=0;i++) { // do the remaining bytes
      RCUF_SET_ADDRESS(address+i);
      RCUF_SET_DATA(*(((char*)(buff+wordoffs))+byteoffs));
      /**iResult=*/RCUF_WRITE_STROBE();
      RCUF_STROBE_END();
      byteoffs++;
    }
    iResult=i;
  } else {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: driver not initialized");
    iResult=-EFAULT;
  }
  return iResult;
}


/******************************************************************************************************
 ******************************************************************************************************
 ******************************************************************************************************
 *********   P A R T 2 : file io interface                                                    *********
 ******************************************************************************************************
 ******************************************************************************************************
 *********                                                                                    *********
 *********   driver initialization and cleanup                                                *********
 *********                                                                                    *********
 ******************************************************************************************************/

/*
 * method prototypes see below for implementations
 */
static loff_t rcuf_llseek(struct file* filp, loff_t off, int ref);
static int rcuf_read(struct file* filp, char* buf, size_t count, loff_t* f_pos);
static int rcuf_write(struct file* filp, const char* buf, size_t count, loff_t* f_pos);
static int rcuf_open(struct inode* inode, struct file* filp);
static int rcuf_close(struct inode* inode, struct file* filp);
static int rcuf_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg);

/*
 * file_operation structure to define the methods for the char device
 * the methods are going to be called from the system
 * pointers to the methods are provided at driver initialization via the file_operations structure
 */
struct file_operations rcuf_fops = {
  llseek:  rcuf_llseek,
  read:    rcuf_read,
  write:   rcuf_write,
  ioctl:   rcuf_ioctl,
  open:    rcuf_open,
  release: rcuf_close,
};

/******************************************************************************************************
 *********                                                                                    *********
 *********   initialisation for real memory                                                   *********
 *********                                                                                    *********
 ******************************************************************************************************/

#ifndef DCSC_TEST
/*
 * cleanup of the memory for the real driver
 */
void cleanupRealBuffers()
{
  // put the cleanup for the flash memory in here
  if (rcuf_virtaddr) iounmap((void *)rcuf_virtaddr);
  if (rcuf_virtdtao) iounmap((void *)rcuf_virtdtao);
  if (rcuf_virtdtai) iounmap((void *)rcuf_virtdtai);
  if (rcuf_virtctrl) iounmap((void *)rcuf_virtctrl);
}

/*
 * initialization of the memory for the real driver
 */
int initRealBuffers()
{
  int iResult=0;
  // put the initialization of the flash memory in here
  rcuf_virtaddr = (u32*) ioremap_nocache((u32)rcuf_addrlines_physaddr,RCUF_ADDR_WIDTH);
  rcuf_virtdtao = (u32*) ioremap_nocache((u32)rcuf_data_out_physaddr,RCUF_DATA_WIDTH);
  rcuf_virtdtai = (u32*) ioremap_nocache((u32)rcuf_data_in_physaddr,RCUF_DATA_WIDTH);
  rcuf_virtctrl = (u32*) ioremap_nocache((u32)rcuf_bus_ctrl_physaddr,RCUF_CTRL_WIDTH);
  mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Remapped ADDRLINES from %p to %p",rcuf_addrlines_physaddr, rcuf_virtaddr);
  mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Remapped DATA OUT  from %p to %p",rcuf_data_out_physaddr, rcuf_virtdtao);
  mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Remapped DATA IN   from %p to %p",rcuf_data_in_physaddr, rcuf_virtdtai);
  mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Remapped BUS CTRL  from %p to %p",rcuf_bus_ctrl_physaddr, rcuf_virtctrl);
  if (rcuf_virtaddr && rcuf_virtdtao && rcuf_virtdtai && rcuf_virtctrl){
  } else {
    mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "error accessing bus");
    iResult=-EFAULT;
  }
  return iResult;
}

/******************************************************************************************************
 *********                                                                                    *********
 *********   initialisation for simulated memory                                              *********
 *********                                                                                    *********
 ******************************************************************************************************/
#else //DCSC_TEST
/*
 * cleanup of the buffers for the simulated driver
 */
void cleanupSimulatedBuffers()
{
  if (rcuf_virtbase) kfree(rcuf_virtbase);
}

/*
 * initialization of the buffers for the simulated driver
 */
int initSimulatedBuffers()
{
  int iResult=0;
  int iNofErr=0;
  mrlogmessage(LOG_INFO, KERN_INFO "this is a test version");
  rcuf_virtbase  = (u32*) kmalloc(rcuf_size, GFP_KERNEL);
  if (rcuf_virtbase) mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Allocated simulated flash memory at 0x%p %d bytes", rcuf_virtbase, rcuf_size);
  if (rcuf_virtbase){
    iNofErr=rcuf_memtest( (u32) rcuf_virtbase , rcuf_size);
    rcuf_writevalue((u32)rcuf_virtbase , rcuf_size, 0);
    mrlogmessage(LOG_INTERNAL|LOG_DBG, MR_KERN_DEBUG "Test memory: %d errors",iNofErr);
  } else {
    mrlogmessage(LOG_INTERNAL|LOG_ERROR, KERN_ERR "Allocation of memory failed");
  }

  return iResult;
}
#endif //DCSC_TEST

/******************************************************************************************************
 *********                                                                                    *********
 *********   driver initialisation                                                            *********
 *********                                                                                    *********
 ******************************************************************************************************/

/*
 * internal init_module 
 * called from the system during insmod command in case of a stand-alone driver
 * or from the main driver in case of a composite driver
 */
int rcuf_init_module(void) {
  // TODO check for stuff to cleanup in case of error and undesired termination
  int iResult=0;

  // register the driver
  iResult = register_chrdev(rcuf_majorID, "rcu flash memory driver", &rcuf_fops);
  if (iResult < 0) {
    mrlogmessage(LOG_ERROR, KERN_ERR "module init: can't register driver");
  } else {
    mrlogmessage(LOG_INFO, KERN_INFO "Module init (compiled "__DATE__", "__TIME__") version %d.%d", RCUFLASH_MAJOR_VERSION_NUMBER, RCUFLASH_MINOR_VERSION_NUMBER);
    if (rcuf_majorID == 0) { /* dynamic allocation of major number*/
      rcuf_majorID = iResult; 
      mrlogmessage(LOG_INFO, KERN_INFO "dynamic allocation of major number: %d", rcuf_majorID);
    } else {
      mrlogmessage(LOG_INFO, KERN_INFO "register rcu flash memory driver at major no. %d", rcuf_majorID);
    }
    mrAddLogPrefix("rcu flash: ", __FILE__);
#ifndef DCSC_TEST
    iResult=initRealBuffers();
#else //!DCSC_TEST
    iResult=initSimulatedBuffers();
#endif //DCSC_TEST
  }
  return iResult;
}

#ifndef DCSC_COMPOSITE
/*
 * init_module called from the system during insmod command
 * wrapper to the init function, will only be preset in the case
 * of a stand-alone driver
 */
int init_module(void) {
  return rcuf_init_module();
}
#endif //DCSC_COMPOSITE

/*
 * internal cleanup_module 
 * called from the system during insmod command in case of a stand-alone driver
 * or from the main driver in case of a composite driver
 */
void rcuf_cleanup_module(void) { 
  mrlogmessage(LOG_INFO, KERN_INFO "Module exit");

#ifndef DCSC_TEST
  cleanupRealBuffers();
#else //DCSC_TEST
  cleanupSimulatedBuffers();
#endif //DCSC_TEST

  int iResult=unregister_chrdev(rcuf_majorID, "rcu flash memory driver");
  if (iResult<0){
    mrlogmessage(LOG_ERROR, KERN_ERR "unregister returned error %d", iResult);
  }
}

#ifndef DCSC_COMPOSITE
/*
 * cleanup_module called from the system during rmmod command
 * wrapper to the init function, will only be preset in the case
 * of a stand-alone driver
 */
void cleanup_module(void) { 
  rcuf_cleanup_module();
}
#endif //DCSC_COMPOSITE


/******************************************************************************************************
 *********                                                                                    *********
 *********   implementation of driver access methods                                          *********
 *********                                                                                    *********
 ******************************************************************************************************/

/*
 * seek funtion to move the internal file pointer 
 */
static loff_t rcuf_llseek(struct file* filp, loff_t off, int ref)
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

/*
 * file read funtion
 * reads 'count' bytes from the flash into the buffer 'buf' and moves the file pointer
 * returns the new file position to f_pos if provided
 */
static int rcuf_read(struct file* filp, char* buf, size_t count, loff_t* f_pos)
{
  int iResult=0;
  if (filp==NULL) {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: invalid file pointer");
    return -EFAULT;
  }
  if (buf && count>=0){
    u32 u32Offset=filp->f_pos;
    u32 u32Count=count;
    // check for space
    if (u32Offset + count > rcuf_size ) {
      u32Count=rcuf_size-u32Offset;
      mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "read: reading %d bytes(s) from position %d exceeds memory size of %d", count, u32Offset, rcuf_size);
    }
    mrlogmessage(LOG_READ_WRITE|LOG_DBG, MR_KERN_DEBUG "read: %d Byte(s) at position %d", u32Count, u32Offset);
    iResult=rcu_flash_read(u32Offset, u32Count, (u32*)buf);
    if (iResult==(int)u32Count) {
      filp->f_pos += iResult;
      if (f_pos) *f_pos=filp->f_pos;
      else mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "read: invalid parameter f_pos, skipping position increment");
    } else if (iResult>=0) {
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: read failure, %d bytes read", iResult);
    } else {
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: read failure, error %d", iResult);
    }
  } else {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "read: invalid parameter");
    iResult=-EINVAL;
  }
  return iResult;
}

/*
 * file write funtion
 * writes 'count' bytes from the buffer 'buf' to the flash and moves the file pointer
 * returns the new file position to f_pos if provided
 */
static int rcuf_write(struct file* filp, const char* buf, size_t count, loff_t* f_pos)
{
  int iResult=0;
  if (filp==NULL) {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: invalid file pointer");
    return -EFAULT;
  }
  if (buf && count>=0){
    u32 u32Offset=filp->f_pos;
    u32 u32Count=count;
    // check for space
    if (u32Offset + count > rcuf_size ) {
      u32Count=rcuf_size-u32Offset;
      mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "write: writing %d bytes(s) from position %d exceeds memory size of %d", count, u32Offset, rcuf_size);
    }
    mrlogmessage(LOG_READ_WRITE|LOG_DBG, MR_KERN_DEBUG "write: %d Byte(s) at position %d", u32Count, u32Offset);
    iResult=rcu_flash_write(u32Offset, u32Count, (u32*)buf);
    if (iResult==(int)u32Count) {
      filp->f_pos+=iResult;
      if (f_pos) *f_pos=filp->f_pos;
      else mrlogmessage(LOG_READ_WRITE|LOG_WARNING, KERN_WARNING "write: invalid parameter f_pos, skipping position increment");
    } else if (iResult>=0) {
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: write failure, partial written (%d bytes)", iResult);
    } else {
      mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: write failure, error %d", iResult);
    }
  } else {
    mrlogmessage(LOG_READ_WRITE|LOG_ERROR, KERN_ERR "write: invalid parameter");
    iResult=-EINVAL;
  }
  return iResult;
}

/*
 * file open funtion
 * initialisation of the file access instances
 * multiple instances can be open
 */
static int rcuf_open(struct inode* inode, struct file* filp)
{
  int iResult=0;
#ifdef DCSC_TEST
  if (rcuf_virtbase==NULL)
#else //!DCSC_TEST
  if (rcuf_virtaddr==NULL || rcuf_virtdtao==NULL || rcuf_virtdtai==NULL || rcuf_virtctrl==NULL)
#endif //DCSC_TEST
  {
    mrlogmessage(LOG_OPEN_CLOSE|LOG_ERROR, KERN_CRIT "open: panic, resource address(es) not initialized");
    iResult=-EFAULT;
  } else {
    mrlogmessage(LOG_OPEN_CLOSE|LOG_DBG, MR_KERN_DEBUG "open");
    // put initialisation in here
  }
  return iResult;
}

/*
 * file close funtion
 * cleanup of local parameters for the file access instances
 * multiple instances can be open, dont cleanup global structures! 
 */
static int rcuf_close(struct inode* inode, struct file* filp)
{
  int iResult=0;
  mrlogmessage(LOG_OPEN_CLOSE|LOG_DBG, MR_KERN_DEBUG "close");
  // put more cleanup in here
  return iResult;
}

/*
 * file io control funtion
 * special control of the driver through command ids which have to be unique
 */
static int rcuf_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
  int iResult=0;
  if (filp==NULL || inode==NULL) {
    mrlogmessage(LOG_IOCTL|LOG_ERROR, KERN_ERR "ioctl: invalid file/inode pointer");
    return -EFAULT;
  }
  switch (cmd) {
  case IOCTL_GET_VERSION:
    {
      if (arg!=0) {
	char versionstring[VERSION_STRING_SIZE];
	int iLen=snprintf(versionstring, sizeof(versionstring), "%d.%d - %s", RCUFLASH_MAJOR_VERSION_NUMBER, RCUFLASH_MINOR_VERSION_NUMBER, RELEASETYPE);
	if (iLen>0) {
	  copy_to_user((void*)arg, versionstring, iLen);
	} else {
	  copy_to_user((void*)arg, "", 1);
	}
      }
      iResult=(RCUFLASH_MAJOR_VERSION_NUMBER << 16) + RCUFLASH_MINOR_VERSION_NUMBER;
    }
    break;
  case IOCTL_GET_VERS_STR_SIZE:
    iResult=VERSION_STRING_SIZE;
    break;
  case IOCTL_SET_DEBUG_LEVEL:
    if (arg>0xffff)
      mrlogmessage(LOG_IOCTL|LOG_WARNING, KERN_WARNING "ioctl: log flags are 16 bit wide, skip MSBs of argument");
    arg&=0xffff;
    //    mrSetLogFilter((u16)arg);
    mrSetLogFilter((int)arg);
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
  case IOCTL_RCUF_GET_FLASH_SIZE:
    *((u32*)arg)=rcuf_size;
    iResult=rcuf_size;
    break;
#ifdef ARM_UCLIB
  /* the driver implements the  MEMGETINFO and MEMERASE controls
   * to support the eraseall utility
   * not all the functionality of the mtd device is supported
   */
  case MEMGETINFO:
    {
      struct mtd_info mtd; 
      memset(&mtd, 0, sizeof(struct mtd_info));
      mtd.size=rcuf_size;
      mtd.erasesize=mtd.size;
      if (copy_to_user((struct mtd_info *)arg, &mtd,
		       sizeof(struct mtd_info_user)))
	iResult=-EFAULT;
    }
    break;
  case MEMERASE:
    {
      struct erase_info *erase;

      if((filp->f_mode & 2)) {
	erase=kmalloc(sizeof(struct erase_info),GFP_KERNEL);
	if (!erase)
	  iResult = -ENOMEM;
	else {
	  memset (erase,0,sizeof(struct erase_info));
	  // map the user space structure to the kernel space erase structure
	  if (copy_from_user(&erase->addr, (u_long *)arg,
			     2 * sizeof(u_long))) {
	    iResult = -EFAULT;
	  } else {
	    // put the erase function in here
	    mrlogmessage(LOG_IOCTL|LOG_DBG, KERN_DEBUG "ioctl: erase flash from %#x %d bytes", erase->addr, erase->len);
	    iResult=rcu_flash_erase(erase->addr, erase->len);
	  }
	  if (iResult==erase->len) {
	    iResult=0; // eraseall command expects 0 for success
	  } else if (iResult>=0) {
	    mrlogmessage(LOG_IOCTL|LOG_WARNING, KERN_WARNING "ioctl: flash partially erased %d Byte ", iResult);
	  } else {
	    mrlogmessage(LOG_IOCTL|LOG_ERROR, KERN_ERR "ioctl: error erasing flash (%d)", iResult);
	  }
	  kfree(erase);
	}
      } else {
	iResult = -EPERM;
      }
      break;
    }
#endif //ARM_UCLIB
  }
  return iResult;
}

