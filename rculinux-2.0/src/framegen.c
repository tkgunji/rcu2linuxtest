// $Id: framegen.c,v 1.7 2008/02/27 07:47:54 richter Exp $

/****************************************************************************
*
* Framegenerator - Framegen
*
* a utility to read out frames from the Xilinx memory.
* The program uses a frames file wich contains all the frames to be read out,
* and readheaderfiles and readfooterfiles to read a frame from the memory.
* 
* It then generates raw framefiles which contain just the data, also readframefiles
* wich contain in addition the readheader and readfooter and writeframefiles
* where a writeheader and a writefooter is wrapped around the data.
*
*
* written by Dominik Fehlker, dfehlker@htwm.de
******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dcscMsgBufferInterface.h"
#include "framegen.h"
//#include "framehelpers.h"




int commit(){
   int result;
   result = 0;

   result = (rcuSingleWrite(0xb002, 0x4));
   if(result<0)
	printf("rcuSingleWrite failed: 0xb002 0x4");
   //printf("w 0xb002 0x4\n");
return result;
}


int main(int argc, char **argv){
   __u32 *pSrc;
   __u16 *pTgt, *pTgtoutput;
   FILE *frames;
   FILE *readheader;
   FILE *writeheader;
   FILE *readfooter;
   FILE *writefooter;
   char line[256];
   char frameaddressline[256];
   int command;
   char shortline[4];
   int data[212];
   int result, block, major, minor;
   FILE *writefile;
   int FrameAddr[3];
   int *pFrameAddr;
   int n, readcounter, writecounter, rawcounter;
   char readframefilename[64];
   char writeframefilename[64];
   char rawframefilename[64];
   unsigned short address3116;
   unsigned short address150;
   unsigned short addresses[2];
   unsigned short *pAddresses;
   __u16 readarray[256];
   __u16 writearray[256];
   pFrameAddr = &FrameAddr[0];
   block = -1;
   major = -1;
   minor = -1;
   pAddresses = &addresses[0];


  if(argc != 6) {
	perror("usage: framegen <frames> <read_header> <read_footer> <write_header> <write_footer>\n");	
	return EXIT_FAILURE;
   }
   //open frames.txt for reading
   if((frames=fopen(argv[1],"r")) < 0){
	printf("Could not open %s for reading!\n",argv[1]);
	return EXIT_FAILURE;
   }
   //Open read_header.txt for reading
   if((readheader=fopen(argv[2], "r")) < 0){
	printf("Error opening %s!",argv[2]);
	return EXIT_FAILURE;
   }
   //Open read_footer for reading
   if((readfooter=fopen(argv[3], "r")) < 0){
	printf("Error opening %s!",argv[3]);
	return EXIT_FAILURE;
   }
   //Open write_header for reading
   if((writeheader=fopen(argv[4], "r")) < 0){
	printf("Error opening %s!", argv[4]);
	return EXIT_FAILURE;
   }
   //Open write_footer for reading
   if((writefooter=fopen(argv[5], "r")) < 0){
	printf("Error opening %s!",argv[5]);
	return EXIT_FAILURE;
   }
   readcounter = 0;
   writecounter = 0;
   rawcounter = 0;

   //enable writing to the RCU
   initRcuAccess(NULL);

   if(frames!=NULL){
	//if there is something to readout
	while(fgets(frameaddressline,255,frames)!=NULL){
	
	    pFrameAddr = &FrameAddr[0];
	    //Frameaddress and filename for the hex file are build here
	    getFrameAddressFromLine(frameaddressline, pFrameAddr);
	    pFrameAddr = &FrameAddr[0];
	    block = FrameAddr[0];
	    major = FrameAddr[1];
	    minor = FrameAddr[2];
	    sprintf(writeframefilename,"w_frame%d.%d.%d.hex",block,major,minor);	    
            sprintf(readframefilename,"r_frame%d.%d.%d.hex",block,major,minor);
	    sprintf(rawframefilename,"frame%d.%d.%d.hex",block,major,minor);
	    //printf("Major number to go for:%#x\n", major);
  	    address3116 = (block<<9)|(major<<1)|(minor>>7);
	    //printf("address3116:%#x\n", address3116);
	    address150 = (minor<<9);
 	    addresses[0] = address3116;
	    addresses[1] = address150;
	    //printf("address3116: %#x , address150: %#x \n", address3116, address150);

	    if(block != -1){
		//printf("start reading data for %s.\n",readframefilename);

		    //if there is a read header available, read all its contents and execute
		    while(fgets(line, 255, readheader)!= NULL){
			    pAddresses = &addresses[0];
			    command = getHexValueFromLine(line, pAddresses);
			    //printf("decoded command: %#x\n", command);
			    pAddresses = &addresses[0];
			//while(command == -1){	
			//    command = getHexValueFromLine(line);
			//}

			//printf("w 0xb103 0x%x\n",command);
			

			//while read_header is processed, put commands in array
			// to write it later to the hex file	
			readarray[readcounter] = command;
			readcounter++;
		
			//write Data
			if(rcuSingleWrite(0xb103,command)<0)
				printf("rcuSingleWrite failed: 0xb103 0x%x",command );
				
//printf("rcuSingleWrite : 0xb103 0x%x\n",command );


			//trigger command
			if(commit()<0){
			    printf("rcuSingleWrite failed: 0xb002 0x4");
			    return EXIT_FAILURE;
			}
		    }
		rewind(readheader);

		if(readheader != NULL){
			if((writefile=fopen(readframefilename,"w+b")) < 0){
		    	   printf("Error opening %s for ascii writing!",readframefilename);
		    	   return EXIT_FAILURE;
			}
		//write readheaderarray to hex file
		fwrite(readarray,sizeof(__u16),readcounter,writefile);
		readcounter=0;
		fclose(writefile);

		}

		writecounter=0;
		if(writeheader != NULL){
			if((writefile=fopen(writeframefilename,"w+b")) < 0){
		    	   printf("Error opening %s for ascii writing!",writeframefilename);
		    	   return EXIT_FAILURE;
			}

			while(fgets(line, 255, writeheader)!=NULL){

			        pAddresses = &addresses[0];
			        command = getHexValueFromLine(line, pAddresses);
			        pAddresses = &addresses[0];
			    //while(command == -1){	
			    //    command = getHexValueFromLine(line);
			    //}
		    		writearray[writecounter] = command;
		    		writecounter++;
			}
		}

		//write readheaderarray to hex file
		fwrite(writearray,sizeof(__u16),writecounter,writefile);
		writecounter=0;
		fclose(writefile);
		rewind(writeheader);
		
		//Number of words to read in to Memory on Actel
		if(rcuSingleWrite(0xb103,0x1a8)<0)
				printf("rcuSingleWrite failed: 0xb103 0x1a8");
		//printf("w 0xb103 0x1a8\n");

		if(rcuSingleWrite(0xb002,0x8)<0)
				printf("rcuSingleWrite failed: 0xb002 0x8");	
		//printf("w 0xb002 0x8\n");
		
		
		//if there is a read footer, process it
		if(readfooter != NULL){
		    while(fgets(line, 255, readfooter)!=NULL){
			
			    pAddresses = &addresses[0];
			    command = getHexValueFromLine(line, pAddresses);
			    pAddresses = &addresses[0];
			//while(command == -1){	
			//    command = getHexValueFromLine(line);
			//}
			//printf("w 0xb103 0x%x\n",command);
			
			readarray[readcounter] = command;
			readcounter++;

			if(rcuSingleWrite(0xb103,command)<0)
				printf("rcuSingleWrite failed: 0xb103 0x%x", command);
			
			//trigger command
			if(commit()<0){
			    printf("rcuSingleWrite failed: 0xb002 0x4");
			    return EXIT_FAILURE;
			}
		    }
		rewind(readfooter);

		}
	
		//read the data

	
		result = rcuMultipleRead(0xb4d4, 212, data);
		printf("read %d bytes from frame %d - %d - %d.\n",result, block, major, minor);
		/*
		pSrc = (__u32*)&data[0];
		pTgtoutput = pTgt = (__u8*)malloc(212*sizeof(__u16));

		for(n=0;n<212;n++){

		    *pTgt=*(((__u8*)pSrc)+1);
		    *(pTgt+1)=*((__u8*)pSrc);
		    //printf("src=%#x,lsb=%#x,msb=%#x,n=%d\n",*pSrc,*pTgt,*(pTgt+1),n);
		    pTgt+=2;
		    pSrc++;
		}
		*/
		
		pSrc = (__u32*)&data[0];
		pTgtoutput = pTgt = (__u16*)malloc(212*sizeof(__u16));

		for(n=0;n<212;n++){
		    *pTgt=*((__u16*)pSrc);
		    
		    pTgt++;
		    pSrc++;
		}
	
		
		//write data to readframe file
		if((writefile=fopen(readframefilename,"a+b")) < 0){
		    printf("Error opening %s for ascii writing!",readframefilename);
		    return EXIT_FAILURE;
		}
		
		fwrite(pTgtoutput, sizeof(__u16), 212, writefile);
	//	fwrite(data, sizeof(__u16), 212, writefile);
		fclose(writefile);

		//write data to writeframe file
		if((writefile=fopen(writeframefilename,"a+b")) < 0){
		    printf("Error opening %s for ascii writing!",writeframefilename);
		    return EXIT_FAILURE;
		}
		
		fwrite(pTgtoutput, sizeof(__u16), 212, writefile);
		fwrite(pTgtoutput, sizeof(__u16), 212, writefile);
		//fwrite(data, sizeof(__u16), 212, writefile);
		//fwrite(data, sizeof(__u16), 212, writefile);
		fclose(writefile);
			
		//write data to rawframe file
		if((writefile=fopen(rawframefilename,"w+b")) < 0){
		    printf("Error opening %s for writing!",rawframefilename);
		    return EXIT_FAILURE;
		}
		
		fwrite(pTgtoutput, sizeof(__u16), 212, writefile);
	//	fwrite(data, sizeof(__u16), 212, writefile);
		fclose(writefile);


		free(pTgtoutput);

		//write read footer to hex file
		if((writefile=fopen(readframefilename,"a+b")) < 0){
		    printf("Error opening %s for ascii writing!",readframefilename);
		    return EXIT_FAILURE;
		}
		
		fwrite(readarray,sizeof(__u16),readcounter,writefile);
		fclose(writefile);
		rewind(readfooter);			
		readcounter = 0;
		
		//read writefooter and write it binary
		if(writefooter != NULL){
			if((writefile=fopen(writeframefilename,"a+b")) < 0){
		    	   printf("Error opening %s for ascii writing!",writeframefilename);
		    	   return EXIT_FAILURE;
			}
			while(fgets(line, 255, writefooter)!=NULL){	
			        pAddresses = &addresses[0];
			        command = getHexValueFromLine(line, pAddresses);
			        pAddresses = &addresses[0];
			    //while(command == -1){	
			    //    command = getHexValueFromLine(line);
			    //}
		    		writearray[writecounter] = command;
		    		writecounter++;
			}
		fclose(writefile);
		}
		
		if((writefile=fopen(writeframefilename,"a+b")) < 0){
		    printf("Error opening %s for ascii writing!",writeframefilename);
		    return EXIT_FAILURE;
		}
		
		fwrite(writearray,sizeof(__u16),writecounter,writefile);
		fclose(writefile);
		rewind(writefooter);			
		writecounter = 0;
	    }
	readframefilename[0]='\0';
	writeframefilename[0]='\0';
	}
   }

   printf("Closing opened files\n");
   fclose(frames);
   fclose(readheader);
   fclose(readfooter);
   fclose(writeheader);
   fclose(writefooter);
   
   releaseRcuAccess();

   return EXIT_SUCCESS;
}
