/****************************************************************************
*
* The bitswapper was written to swap a file which was read back from the
* RCU Flash memory in an 8 bit manner. This means when the read back file
* shows an AA99 5566 pattern, this will be swapped to 99AA 6655.
*
* In this way it is possible to read the RCU configuration from the RCU
* Flash memory and generate the RCU bitfile from it, to program other RCUs
* with the same configuration.
*
*
* written by Dominik Fehlker, Dominik.Fehlker@uib.no
******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/types.h>
#include <sys/stat.h>
//#include "dcscMsgBufferInterface.h"
#include <linux/errno.h>
#include <fcntl.h>



int main(int argc, char **argv){
   __u8 *pSrc;
   __u8 *pTgt, *pTgtoutput;
   int filesize, n;
   FILE *inputfile, *outputfile;
   filesize = 0;



  if(argc != 3) {
	printf("usage: bitswapper <inputfile> <outputfile>\n");	
	return EXIT_FAILURE;
   }
  
   filesize = getFileSize(argv[1]);
   printf("filesize= %d for file %s\n", filesize, argv[1]);

   //open inputfile for reading
   if((inputfile=fopen(argv[1],"r")) < 0){
	printf("Could not open %s for reading!\n",argv[1]);
	return EXIT_FAILURE;
   }


  
   pSrc = (__u8*)malloc(filesize*sizeof(__u8));
   printf("Allocated source memory area\n");
   pTgtoutput = pTgt = (__u8*)malloc(filesize*sizeof(__u8));
   printf("Allocated  target memory area\n");  


   fread(pSrc, filesize, 1, inputfile);

   swab(pSrc, pTgt, filesize);

   printf("before: %#x, after: %#x: \n",*pSrc, *pTgt);
   

   //open the outputfile for writing
   if((outputfile=fopen(argv[2],"a+b")) < 0){
     printf("Error opening %s for writing!\n",argv[2]);
     return EXIT_FAILURE;
   }
   
   //write the swapped data to the file
   fwrite(pTgtoutput, sizeof(__u8),filesize , outputfile);

   free(pSrc);
   free(pTgtoutput);

   fclose(outputfile);
   fclose(inputfile);

   return EXIT_SUCCESS;
}




int getFileSize(char *path){
   unsigned long size;
   size_t filesize;
   struct stat statbuf;
   size = -1;


   if ((lstat(path, &statbuf)) == -1 ){
        perror("Pfadfehler");
        return EXIT_FAILURE;
   }

   //read the filesize
   //fstat(initconfig, &statbuf);
   filesize = statbuf.st_size;
   size = (int)filesize;

return size;
}


