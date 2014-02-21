/*
 * sample.c - The simplest loadable kernel module.
 * Intended as a template for development of more
 * meaningful kernel modules.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
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


/*
 * Driver verbosity level: 0->silent; >0->verbose
 */
static int sample_debug = 1;

/*
 * User can change verbosity of the driver
 */
module_param(sample_debug, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(sample_debug, "Sample driver verbosity level");

/*
 * Service to print debug messages
 */
#define d_printk(level, fmt, args...)				\
  if (sample_debug >= level) printk(KERN_INFO "%s: " fmt,	\
				    __func__, ## args)

/*
 * Device major number
 */
static uint sample_major = 177;

/*
 * User can change the major number
 */
module_param(sample_major, uint, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(sample_major, "Sample driver major number");

/*
 * Device name
 */
static char * sample_name = "sample2";

/*
 * Device access lock. Only one process can access the driver at a time
 */

static int sample_lock = 0;

/*
 * Device "data"
 */
static char sample_str[] = "This is the simplest loadable kernel module!!!! sample2.ko\n";
static char *sample_end;

/*
 * address list for the commnication to s2m-mos
 */
/// fabric_1 registers
static u32* apbbus1_in_physaddr  = ((u32 *) 0x50000000);
static int apbbus1_in_loc   = 0x50000000;
static int apbbus1_in_size   = 0x10000000;
static u32* apbbus1_in_virtbase=NULL;

module_param(apbbus1_in_size, int, S_IRUSR | S_IWUSR);
module_param(apbbus1_in_physaddr, uint, S_IRUSR | S_IWUSR);

/// fabric_0 registers
static u32* apbbus0_in_physaddr  = ((u32 *) 0x30000000);
static int apbbus0_in_loc   = 0x30000000;
static int apbbus0_in_size   = 0x10000000;
static u32* apbbus0_in_virtbase=NULL;

module_param(apbbus0_in_size, int, S_IRUSR | S_IWUSR);
module_param(apbbus0_in_physaddr, uint, S_IRUSR | S_IWUSR);

/// radmon 
static u32* spi_in_physaddr  = ((u32 *) 0x40001000);
static int spi_in_loc  = 0x40001000;
static int spi_in_size   = 0x1000;
static u32* spi_in_virtbase=NULL;

module_param(spi_in_size, int, S_IRUSR | S_IWUSR);
module_param(spi_in_physaddr, uint, S_IRUSR | S_IWUSR);


/// i2c
static u32* i2c_in_physaddr  = ((u32 *) 0x40002000);
static int i2c_in_loc  = 0x40002000;
static int i2c_in_size   = 0x1000;
static u32* i2c_in_virtbase=NULL;

module_param(i2c_in_size, int, S_IRUSR | S_IWUSR);
module_param(i2c_in_physaddr, uint, S_IRUSR | S_IWUSR);

/*
 * internal functions 
 */
static int memtest(u32 begin, u32 size) {
  u32 i,rd;
  int ret=0;
  int success=0;
  for (i=0;i<size;i+=4) {
    d_printk(0, "memery test writing (addr = 0x%x, data=0x%x)\n", begin+i*16, i+1);
    writel(i+1, begin + i*16);
  }
  for (i=0;i<size;i+=4) {
    rd=readl(begin + i*16);
    d_printk(0, "memery test reading (addr = 0x%x, data=0x%x)\n", begin+i*16, rd);
    if (  rd != i+1 ) {
      ret++;
    } else {
      success++;
    }
  }
  return ret;
}

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

static int dcs_write (u32 begin, u32 size, u32* buff)
{

  int i;
  u32 wordoffs=0;
  u32 byteoffs=0;

  for (i=0;i<size/4;i++){// do the multibles of 4  
    writel((u32*)(buff+wordoffs),(u32)(begin+i*4));
    wordoffs++;
  }
  i*=4;
  for (;i<size;i++) { // do the remaining bytes
    writeb((u32*)(((char*)(buff+wordoffs))+byteoffs), (u32)begin+i);
    byteoffs++;
  }

  return 0;
}


/*
 * Device open
 */
static int sample_open(struct inode *inode, struct file *file)
{
  int ret = 0;

  /*
   * One process at a time
   */
  if (sample_lock ++ > 0) {
    ret = -EBUSY;
    goto Done;
  }
  
  /*
   * Increment the module use counter
   */
  try_module_get(THIS_MODULE);
  
  /*
   * Do open-time calculations
   */
  sample_end = sample_str + strlen(sample_str);
  
 Done:
  d_printk(2, "lock=%d\n", sample_lock);
  return ret;
}

/*
 * Device close
 */
static int sample_release(struct inode *inode, struct file *file)
{
  /*
   * Release device
   */
  sample_lock = 0;
  
  /*
   * Decrement module use counter
   */
  module_put(THIS_MODULE);
  
  d_printk(2, "lock=%d\n", sample_lock);
  return 0;
}

/* 
 * Device read
 */
static ssize_t sample_read(struct file *filp, char *buffer,
			   size_t length, loff_t * offset)
{
  const char* pBufferName="<not specified name>";
  unsigned int len = 0;
  int ret = 0;
  u32 u32Count=0;
  u32* pu32Source=NULL;
  u32 lPos=0;
  u32 u32Offset= 0;

  lPos=filp->f_pos;
  /*
   * Check that the user has supplied a valid buffer
   */
  if (! access_ok(0, buffer, length)) {
    ret = -EINVAL;
    goto Done;
  }

  if (filp && buffer && length>=0 && lPos>=0){

    //if(lPos <  apbbus0_in_physaddr){
    //d_printk(0, "read: access mode not yet implemented.. address is not ready in use");
    if(lPos>=apbbus0_in_loc && lPos<apbbus0_in_loc+apbbus0_in_size){
      u32Offset= lPos - apbbus0_in_loc;
      pu32Source = apbbus0_in_virtbase;
      pBufferName="FPGA Fabric FIC 0";
    }else if(lPos>=apbbus1_in_loc && lPos<apbbus1_in_loc+apbbus1_in_size){
      u32Offset= lPos - apbbus1_in_loc; 
      pu32Source = apbbus1_in_virtbase;
      pBufferName="FPGA Fabric FIC 1";
    }else if(lPos>=spi_in_loc && lPos<spi_in_loc+spi_in_size){
      u32Offset= lPos - spi_in_loc; 
      pu32Source = spi_in_virtbase;
      pBufferName="SPI address space";
    }else if(lPos>=i2c_in_loc && lPos<i2c_in_loc+i2c_in_size){
      u32Offset= lPos - i2c_in_loc; 
      pu32Source = i2c_in_virtbase;
      pBufferName="I2C address space";
    }

    if(pu32Source!=NULL){
      u32Offset<<=4; 
      u32Offset/=sizeof(u32);
      pu32Source += u32Offset;
      u32Count = length;

      dcs_read((u32)pu32Source, u32Count, (u32*)buffer);
      d_printk(0, "read data=0x%x (address = 0x%x)  IPos=0x%x\n", buffer, pu32Source, lPos);
      /////////////////////////////////
      len = (int)u32Count;
      ret = len;
    }
    
  }

 Done:
  d_printk(0, "length=%d,len=%d,ret=%d\n", length, len, ret);
  return ret;
}

/* 
 * Device write
 */
static ssize_t sample_write(struct file *filp, const char *buffer,
			    size_t length, loff_t * offset)
{
  
  int iResult=0;
  u32 lPos=0;
  u32 u32Offset=0;
  u32 u32Count= 0;
  u32* pu32Target=NULL;
  const char* pBufferName="<not specified name>";

  if (filp==NULL) {
    return -EFAULT;
  }
  lPos=filp->f_pos;
  
  if (filp && buffer && length>=0 && lPos>=0){
    if(lPos>=apbbus0_in_loc && lPos<apbbus0_in_loc+apbbus0_in_size){
      u32Offset= lPos - apbbus0_in_loc;
      pu32Target = apbbus0_in_virtbase;
      pBufferName="FPGA Fabric FIC 0";
    }else if(lPos>=apbbus1_in_loc && lPos<apbbus1_in_loc+apbbus1_in_size){
      u32Offset= lPos - apbbus1_in_loc; 
      pu32Target = apbbus1_in_virtbase;
      pBufferName="FPGA Fabric FIC 1";
    }else if(lPos>=spi_in_loc && lPos<spi_in_loc+spi_in_size){
      u32Offset= lPos - spi_in_loc; 
      pu32Target = spi_in_virtbase;
      pBufferName="SPI address space";
    }else if(lPos>=i2c_in_loc && lPos<i2c_in_loc+i2c_in_size){
      u32Offset= lPos - i2c_in_loc; 
      pu32Target = i2c_in_virtbase;
      pBufferName="I2C address space";
    }

    if(pu32Target!=NULL){
      u32Offset<<=4; 
      u32Offset/=sizeof(u32);
      pu32Target += u32Offset;
      u32Count= length;

      d_printk(0, "write data=0x%x (address = 0x%x, length=%d)\n", (u32*)buffer, pu32Target, u32Count);
      dcs_write((u32)pu32Target, u32Count,  (u32*)buffer);
      
      iResult=(int)u32Count;
    }
  }

  d_printk(3, "length=%d\n", length);
  return iResult;
}


/*
 * Device seek
 */
static loff_t sample_seek(struct file* filp, loff_t off, int ref)
{
  loff_t lPosition=0;
  if (filp){
    lPosition=off;
  }else{
    lPosition=-EFAULT;
  }
  if (lPosition>=0) filp->f_pos=lPosition;

  d_printk(0, "filp->f_pos = %d\n", lPosition);

  return lPosition;
}


/*
 * Device operations
 */
static struct file_operations sample_fops = {
  .llseek = sample_seek,
  .read = sample_read,
  .write = sample_write,
  .open = sample_open,
  .release = sample_release
};

/*
 * cleanup of the buffers for the real driver                                                         
 */
void cleanupRealBuffers(void)
{
  if (apbbus1_in_virtbase) iounmap((void *)apbbus1_in_virtbase);
  if (apbbus0_in_virtbase) iounmap((void *)apbbus0_in_virtbase);
  if (spi_in_virtbase) iounmap((void *)spi_in_virtbase);
  if (i2c_in_virtbase) iounmap((void *)i2c_in_virtbase);
}
int initRealBuffers(void)
{
  int iResult=0;
  int iNofErr=0;

  apbbus1_in_virtbase  = (u32*) ioremap_nocache((u32)apbbus1_in_physaddr,apbbus1_in_size);
  d_printk(0, "Remapped MSGBUF_IN from 0x%p to 0x%p",apbbus1_in_physaddr, apbbus1_in_virtbase);
  if(apbbus1_in_virtbase==NULL && apbbus1_in_size>0 ){
    iResult=-EIO;
  }

  apbbus0_in_virtbase  = (u32*) ioremap_nocache((u32)apbbus0_in_physaddr,apbbus0_in_size);
  d_printk(0, "Remapped MSGBUF_IN from 0x%p to 0x%p",apbbus0_in_physaddr, apbbus0_in_virtbase);
  if(apbbus0_in_virtbase==NULL && apbbus0_in_size>0 ){
    iResult=-EIO;
  }

  spi_in_virtbase  = (u32*) ioremap_nocache((u32)spi_in_physaddr,spi_in_size);
  d_printk(0, "Remapped MSGBUF_IN from 0x%p to 0x%p",spi_in_physaddr, spi_in_virtbase);
  if(spi_in_virtbase==NULL && spi_in_size>0 ){
    iResult=-EIO;
  }

  i2c_in_virtbase  = (u32*) ioremap_nocache((u32)i2c_in_physaddr,i2c_in_size);
  d_printk(0, "Remapped MSGBUF_IN from 0x%p to 0x%p",i2c_in_physaddr, i2c_in_virtbase);
  if(i2c_in_virtbase==NULL && i2c_in_size>0 ){
    iResult=-EIO;
  }
  return iResult;
}



static int __init sample_init_module(void)
{
  int ret = 0;
  
  /*
   * check that the user has supplied a correct major number
   */
  if (sample_major == 0) {
    printk(KERN_ALERT "%s: sample_major can't be 0\n", __func__);
    ret = -EINVAL;
    goto Done;
  }
  
  /*
   * Register device
   */
  ret = register_chrdev(sample_major, sample_name, &sample_fops);
  if (ret < 0) {
    printk(KERN_ALERT "%s: registering device %s with major %d "
	   "failed with %d\n",
	   __func__, sample_name, sample_major, ret);
    goto Done;
  }
  
  ret = initRealBuffers();
  if(ret < 0){
    printk(KERN_ALERT "%s: initRealBuffers failed "
	   "failed with %d\n",
	   __func__, ret);
    goto Done;
  }

 Done:
  d_printk(1, "name=%s,major=%d\n", sample_name, sample_major);
  
  return ret;
}
static void __exit sample_cleanup_module(void)
{
  /*
   * Unregister device
   */
  cleanupRealBuffers();
  unregister_chrdev(sample_major, sample_name);
  
  d_printk(1, "%s\n", "clean-up successful");
}

module_init(sample_init_module);
module_exit(sample_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Taku Gunji, Taku.Gunji@cern.ch");
MODULE_DESCRIPTION("Sample device driver for Rcu2");
