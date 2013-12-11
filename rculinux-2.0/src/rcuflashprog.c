// $Id: rcuflashprog.c,v 1.6 2008/02/27 07:47:54 richter Exp $

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

/** @file       rcuflashprog.c
    @author     Dominik Fehlker
    @date
    @brief      Programming tool for the RCU flash memory
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> //for usleep
#include "dcscMsgBufferInterface.h"
#include "framever.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <rcuflashprog.h>
//#include <framehelpers.h>

#define FALSE 0
#define TRUE 1


static int BB_FLASH;

int main(int argc, char **argv){
   FILE *config;
   int init, scrubbing, flashframe,erase;
   int result, oldstate;
   int frameaddr[3];
   char line[255];
   
   BB_FLASH = TRUE;
   result = 0; 
   init = FALSE;
   scrubbing = FALSE;
   flashframe = FALSE;
    erase = FALSE;
   // test for the number of parameters
   if((argc != 2)) {
        perror("usage: rcuflashprog <config>\n");
        return EXIT_FAILURE;
   }

   //open config for reading
   if((config=fopen(argv[1],"r")) < 0){
        printf("Could not open %s for reading!\n",argv[1]);
        return EXIT_FAILURE;
   }
   if(config!=NULL){

      //enable writing to the RCU
      result = initRcuAccess(NULL);
      if(result < 0){
      	printf("Error during initialization of RCU Access: %d\n", result);
	return EXIT_FAILURE;
      }
   

      // write stop code to avoid automatic configuration
        result = rcuSingleWrite(0xb112, 0xa);
        if(result < 0){
          printf("Writing stop code failed");
          return EXIT_FAILURE;
         }
 

   
      //get the current state of the bus
      oldstate = getBusState();
   

      //change to the Flash state
      result = enterFlashState(oldstate);
      if(result < 0){
	printf("Error while changing State to Flash: %d\n", result);
	return EXIT_FAILURE;
      }
   
      
  
       

        //if there is something to readout
        while(fgets(line,255,config)!=NULL){

	   if(strncmp(line, "enable_erase = true", 19) == 0){
		erase = TRUE;
		printf("found enable_erase = true\n");
	   }
           if(strncmp(line, "enable_frfiles = true",21) == 0){
		flashframe = TRUE;
		printf("found enable_frfiles = true\n");
	   }
	   if(strncmp(line, "enable_scfile = true", 20) == 0){
		scrubbing = TRUE;
		printf("found enable_scfile = true\n");
	   }
	   if(strncmp(line, "enable_icfile = true", 20) == 0){
		init = TRUE;
		printf("found enable_icfile = true\n");
	   }
	}
	rewind(config);
	fclose(config);
   
	
         if (erase == TRUE){
          //erase the complete Flash
          result = rcuFlashErase(-1, 0);
          if(result < 0){
     	  printf("Error erasing the Flash: %d", result);
	  return EXIT_FAILURE;
          }
        }
        if(init == TRUE)
	  doInit(argv[1],BB_FLASH);
		
	if(scrubbing == TRUE)
	  doScrubbing(argv[1],BB_FLASH);

	if(flashframe == TRUE)
	  doFlashFrame(argv[1],BB_FLASH);  

 
      //restore the old state of the Bus
      result = restoreBusState(oldstate);
      if(result < 0){
 	printf("Error restoring the old Bus State: %d\n", result);
	return EXIT_FAILURE;
      }


      //release Access to the RCU
      result = releaseRcuAccess();
      if(result < 0){
	printf("Error during release of RCU Access: %d\n", result);
	return EXIT_FAILURE;
      } 
   }


return EXIT_SUCCESS;
}

int doFlashFrame(char *conffilename,int BB_FLASH){
   FILE *config, *framesfile, *readfile, *test;
   char path[128],
	readframefilename[128],
	rawframefilename[128],
	rawreadframefilename[128],
	rawwriteframefilename[128],
        rawrawframefilename[128],
	preadframefilename,
	writeframefilename[128],
	pwriteframefilename,
   	framesfilename[128],
	startaddrline[64],
	frameoffsetline[64],
	readheadersizeline[64],
	readfootersizeline[64],
	temp[128],
	line[255];
   char offset[3];	
   char *ptemp;
   int result,
	i,
	startaddresshex,
	readheadersizehex,
	readfootersizehex,
	frameoffsethex;

   //array for holding the block, major and minor
   int frameaddr[3];
   int *pFrameaddr;
	
   __u32 startaddress,
	unused,
	readheadersize,
	*pData,
 	*preadData,
	*pwriteData,
	readfootersize,
	address;

  __u16 line1, line2, line3,
	frameoffset,
	frameoffsetorig,
 	readframefilesize,
        rawframefilesize,
	writeframefilesize,
	startaddressupper,
	startaddresslower,
	numberofframes;	
  __u32 frame_addr, frame_info_addr;



   pData = malloc(sizeof(__u32));
   i = 0;
   result = 0;
   readframefilesize = 0;
   rawframefilesize = 0;
   writeframefilesize = 0;
   startaddresshex = 0;
   readheadersize = 0;
   readfootersize = 0;
   startaddressupper = 0;
   startaddresslower = 0;
   frameoffsetorig = 0;
   unused = 0;
   line1 = 0;
   line2 = 0;
   line3 = 0;
   frameaddr[0] = -1;
   pFrameaddr = &frameaddr[0];
   if (BB_FLASH==TRUE){
     frame_addr = 0x2000;
     frame_info_addr = 0x5000;
   }else{
     frame_addr = 0x3fb000;
     frame_info_addr = 0x3fb002;
   }
  


   //open config for reading
   if((config=fopen(conffilename,"r")) < 0){
        printf("Could not open %s for reading!\n",conffilename);
        return EXIT_FAILURE;
   }
   if(config!=NULL){
	while(fgets(line,255,config)!=NULL){
	   
	   //read path
	   if(strncmp(line, "path_frfiles", 11) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^\n]", path);
		printf("%s\n", path);
	   	for(i = 0; i < 128; i++){
		   //readframefilename[i] = path[i];
		   //writeframefilename[i] = path[i];
		   framesfilename[i] = path[i];
		}

	   }
	   
	   //read readframefilename
	   if(strncmp(line, "name_readframefile", 18) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^\n]",readframefilename );
		//strcat(readframefilename, temp);
		printf("readframefilename: %s\n", readframefilename);
		strcpy(rawreadframefilename, readframefilename);
	   }
	   //read writeframefilename
	   if(strncmp(line, "name_writeframefile", 19) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^\n]", writeframefilename);
		//strcat(writeframefilename, temp);
		printf("writeframefilename: %s\n", writeframefilename);
		strcpy(rawwriteframefilename, writeframefilename);
	   }
	   //read rawframefilename
	   if(strncmp(line, "name_rawframefile", 17) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^\n]", rawframefilename);
		//strcat(writeframefilename, temp);
		printf("rawframefilename: %s\n", rawframefilename);
		strcpy(rawrawframefilename, rawframefilename);
	   }





	   //read framefilename
	   if(strncmp(line, "framesfile", 10) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^\n]", temp);
		strcat(framesfilename, temp);
		printf("framesfilename: %s\n", framesfilename);
	   }
	   
	   //read startaddress
	   if(strncmp(line, "startaddr_frfiles", 16) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^h\n]", startaddrline);
		sscanf(startaddrline, "%x", &startaddresshex);
		printf("startaddress: %#x\n",startaddresshex);
		startaddress = (__u32)startaddresshex;
	   }
	   //read frame_offset
	   if(strncmp(line, "frame_offset", 12) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^h\n]", frameoffsetline);
		sscanf(frameoffsetline, "%x", &frameoffsethex);
		frameoffsetorig = (__u16)(frameoffsethex);
		strcat(frameoffsetline, "00");
		sscanf(frameoffsetline, "%x", &frameoffsethex);
		frameoffset = (__u16)(frameoffsethex);
		printf("Frame_offset: %#x\n",(frameoffset));
	   }
	   //read readheadersize
	   if(strncmp(line, "header_size_read", 16) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^h\n]", readheadersizeline);
		sscanf(readheadersizeline, "%x", &readheadersizehex);
		printf("Readheadersize: %#x\n",readheadersizehex);
		readheadersize = (__u16)readheadersizehex;
	   }
	   //read readfootersize
	   if(strncmp(line, "footer_size_read", 16) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^h\n]", readfootersizeline);
		sscanf(readfootersizeline, "%x", &readfootersizehex);
		printf("Readfootersize%#x\n",readfootersizehex);
		readfootersize = (__u16)readfootersizehex;
	   }
	   
	   
	}
   }
   fclose(config);

   //count the number of frames in the framesfile(number of lines)
   numberofframes = (__u16)getLinesnumberFromFile(framesfilename);
   printf("Found %d Frames in file %s\n", numberofframes, framesfilename);

   //read out a specific frame for determining the filesize
   if((framesfile=fopen(framesfilename,"r+b")) < 0){
        printf("Could not open %s for reading!\n",framesfilename);
        return EXIT_FAILURE;
   }
   if(fgets(line,255,framesfile)!=NULL){
	while(frameaddr[0] == -1){
	   pFrameaddr = &frameaddr[0];
	   getFrameAddressFromLine(line, pFrameaddr);	
	}
   }
   fclose(framesfile);

   printf("First Frame in %s has Block %d, Major %d and Minor %d\n", framesfilename, frameaddr[0], frameaddr[1], frameaddr[2]);



   //set up pointer   
   *pData = getUpperAddress(frame_info_addr);//0x3f;
   result = rcuFlashWrite(frame_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fb000, %#x, 1, 2: %d\n", *pData, result);
	return EXIT_FAILURE;
   }
   
   *pData = getLowerAddress(frame_info_addr);//0xb002;
   frame_addr += 1;
   result = rcuFlashWrite(frame_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fb001, %#x, 1, 2: %d\n", *pData, result);
	return EXIT_FAILURE;
   }

   //disintegrate the startaddress
   startaddressupper = getUpperAddress(startaddress);
   printf("upper address: %#x\n", startaddressupper);
   startaddresslower = getLowerAddress(startaddress);
   printf("lower address: %#x\n", startaddresslower);

   //write the startaddress 
   *pData = startaddressupper;
   result = rcuFlashWrite(frame_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fb002, %#x, 1, 2: %d\n", *pData, result);
	return EXIT_FAILURE;
   }

   *pData = startaddresslower;
   frame_info_addr += 1;
   result = rcuFlashWrite(frame_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fb003, %#x, 1, 2: %d\n", *pData, result);
	return EXIT_FAILURE;
   }

   //write the not used stopaddress
   *pData = unused;
   frame_info_addr += 1;
   result = rcuFlashWrite(frame_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fb004, %#x, 1, 2: %d\n", *pData, result);
	return EXIT_FAILURE;
   }

   *pData = unused;
   frame_info_addr += 1;
   result = rcuFlashWrite(frame_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fb005, %#x, 1, 2: %d\n", *pData, result);
	return EXIT_FAILURE;
   }
 
   //determine the filesize of the read and writeframes

   //build readframefilename + path
   ptemp = &readframefilename[0];
   pFrameaddr = &frameaddr[0];

   buildFrameFilename(pFrameaddr, ptemp);
   strcpy(temp, path);
   strcat(temp, readframefilename);
   strcpy(readframefilename, temp);
   printf("readframefilename: %s\n", readframefilename);

   //build the rawfilename
   ptemp = &rawframefilename[0];
   pFrameaddr = & frameaddr[0];

   buildFrameFilename(pFrameaddr, ptemp);
   strcpy(temp, path);
   strcat(temp, rawframefilename);
   strcpy(rawframefilename, temp);
   printf("rawframefilename: %s\n", rawframefilename);

   //build writeframefilename + path
   ptemp = &writeframefilename[0];
   buildFrameFilename(pFrameaddr, ptemp);
   strcpy(temp, path);
   strcat(temp, writeframefilename);
   strcpy(writeframefilename, temp);
   printf("writeframefilename: %s\n", writeframefilename);

   
   //lookup the filesizes
   readframefilesize = (__u16)getFileSize(readframefilename);
   printf("Readframefilesize: %d\n", readframefilesize);
   rawframefilesize = (__u16)getFileSize(rawframefilename);
   printf("Rawframefilesize: %d\n", rawframefilesize);
   writeframefilesize = (__u16)getFileSize(writeframefilename);
   printf("Writeframefilesize: %d\n", writeframefilesize);
   
   //Buffers for the data to be flashed
   //preadData = malloc(readframefilesize * sizeof(__u32));



   //put the information together
   //printf("%#x, %#x, %#x\n", frameoffsetorig, numberofframes, writeframefilesize);
   //printf("%#x, %#x, %#x\n", (frameoffsetorig<<12), ((0xfff & numberofframes) <<1), (( 0xff & writeframefilesize) >>7));   
   line1 = (frameoffsetorig << 12) | (( 0xfff & numberofframes) << 1) |((0xffef & ((writeframefilesize/2)-1)) >> 8);

   //printf("%#x, %#x\n", readframefilesize, writeframefilesize);
   //printf("%#x, %#x\n", (readframefilesize<<8), (0x7f & writeframefilesize));
   line2 = (((rawframefilesize/2)-1) << 8) | (0xff & ((writeframefilesize/2)-1));




   line3 = (readheadersize << 8) | (0xff & readfootersize);
   printf("line1: %#x\n", line1);
   printf("line2: %#x\n", line2);
   printf("line3: %#x\n", line3);
  

   //write the information to the flash
   *pData = line1;
   frame_info_addr += 1;
   result = rcuFlashWrite(frame_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fb006, %#x, 1, 2: %d\n", *pData, result);
	return EXIT_FAILURE;
   }

   *pData = line2;
   frame_info_addr += 1;
   result = rcuFlashWrite(frame_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fb007, %#x, 1, 2: %d\n", *pData, result);
	return EXIT_FAILURE;
   }

   *pData = line3;
   frame_info_addr += 1;
   result = rcuFlashWrite(frame_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fb008, %#x, 1, 2: %d\n", *pData, result);
	return EXIT_FAILURE;
   }

   //program the readframes to the flash


   //open frames.txt for reading
   if((framesfile=fopen(framesfilename,"r")) < 0){
        printf("Could not open %s for reading!\n",framesfilename);
        return EXIT_FAILURE;
   }

   address = startaddress;
   preadData = malloc((readframefilesize/2)*sizeof(__u32));

   //if there is something to readout
   while(fgets(line,255,framesfile)!=NULL){
	getFrameAddressFromLine(line, pFrameaddr);
	pFrameaddr = &frameaddr[0];
	if(*pFrameaddr != -1){
   	   //build readframefilename + path
	   strcpy(readframefilename, "\0");

	   for(i = 0; i < 128; i++)
		readframefilename[i] = rawreadframefilename[i];

	   ptemp = &readframefilename[0];
	   pFrameaddr = &frameaddr[0];

	   buildFrameFilename(pFrameaddr, ptemp);

	   strcpy(temp, path);
	   strcat(temp, readframefilename);
	   strcpy(readframefilename, temp);
	   printf("programming %s\n", readframefilename);
	   
	   if((readfile = fopen(readframefilename, "r+b")) < 0){
	   	printf("Error opening %s for reading!\n", readframefilename);
		return EXIT_SUCCESS;
	   }
	   fread(preadData, sizeof(__u16), readframefilesize/2, readfile);
	   fclose(readfile);
	   
   	   result = rcuFlashWrite(address, preadData, readframefilesize/2, 2);
	   if(result < 0){
		printf("rcuFlashWrite failed: %#x, %#x, %d, 2: %d",address ,*preadData,readframefilesize/2, result);
		return EXIT_FAILURE;
	   }
	   printf("writing to address: %#x\n",  address);
	   address+=frameoffset;
	   
	}
   }
   free(preadData);
   rewind(framesfile);

   printf("Rewinded framefile\n");

   address = startaddress + 0x100000;
   printf("Increased startaddress by 0x100000.\n");

   //program the writeframes to the flash
   pwriteData = malloc((writeframefilesize/2)*sizeof(__u32));

   printf("Allocated temporary memory for the writeframes\n");

   while(fgets(line,255,framesfile)!=NULL){
	getFrameAddressFromLine(line, pFrameaddr);
	 pFrameaddr = &frameaddr[0];
	if(*pFrameaddr != -1){
   	   //build writeframefilename + path

	   strcpy(writeframefilename, "\0");

           for(i = 0; i < 128; i++)
                writeframefilename[i] = rawwriteframefilename[i];

	   ptemp = &writeframefilename[0];
	   pFrameaddr = &frameaddr[0];

	   buildFrameFilename(pFrameaddr, ptemp);
	   strcpy(temp, path);
	   strcat(temp, writeframefilename);
	   strcpy(writeframefilename, temp);
	   printf("programming %s\n", writeframefilename);
	   
	   if((readfile = fopen(writeframefilename, "r+b")) < 0){
	   	printf("Error opening %s for reading!\n", writeframefilename);
		return EXIT_SUCCESS;
	   }
	   fread(pwriteData, sizeof(__u16), writeframefilesize/2, readfile);
	   fclose(readfile);
	  	   
   //just to test if everything is working properly   
   // write the stuff to a file
   /**
      if((test=fopen("blubb","w+b")) < 0){
        printf("Could not open %s for reading!\n","blubb");
        return EXIT_FAILURE;
   }
   fwrite(pwriteData, sizeof(__u16), writeframefilesize/2, test);
   fclose(test);
   **/
   	   result = rcuFlashWrite(address, pwriteData, writeframefilesize/2, 2);
	   if(result < 0){
		printf("rcuFlashWrite failed: %#x, %#x, %d, 2: %d",address ,*pwriteData, writeframefilesize/2,result);
		return EXIT_FAILURE;
	   }
	   
	   printf("writing to address: %#x\n",  address);
	   address+=frameoffset;
   
	}
   }


   fclose(framesfile);
   free(pwriteData);
   free(pData);
return result;
}


//pFrameaddr holds Block, Major and Minor, filename is the framefilename, pnewfilename is the one with block, Major and minor in it
void buildFrameFilename(int *pFrameaddr,  char *pfilename){
   //int *pFrameaddr;
   //char *pnewfilename;
   char *startfilename,
	*endfilename,
	*seperator,
	temp[128],
	*ptemp;
   char frameaddrstr[5];
   int i, block, major, minor;

   strcpy(temp, pfilename);
   ptemp = &temp[0];
   startfilename = ptemp;
   block = *pFrameaddr;
   major = *(pFrameaddr+1);
   minor = *(pFrameaddr+2);

   seperator = strchr(ptemp, '$');
   *seperator = '\0';
   endfilename = seperator + 1;

   strcpy(pfilename, startfilename);
   sprintf(frameaddrstr, "%d.%d.%d" , block, major, minor);
   //printf("%s\n", frameaddrstr);
   strcat(pfilename, frameaddrstr);
   strcat(pfilename, endfilename);
   
}


int doScrubbing(char *conffilename,int BB_FLASH){
   FILE *config, *scrubbing, *test;
   int result, startaddrhex, stopaddresshex;
   char path[64],
	filename[64],
	startaddr[64],
	line[255];
   int filesize, i;
   __u32 writeaddress, stopaddress;
   __u32 *pData;
   __u16 *pInitData, *pInitDatabgn, *pInitDataoutput;
   __u8 *pInitDataSwapped, *pInitDatahlp;
   __u16 upperaddr, loweraddr;
  __u32 scrub_addr, scrub_info_addr;

   i = 0;
   writeaddress = 0;
   result = 0;
   startaddrhex = 0;
   stopaddress = 0;
   upperaddr = 0;
   loweraddr = 0;

   if (BB_FLASH==TRUE){ 
     scrub_addr = 0x1000;
     scrub_info_addr = 0x4000;
   }else{
     scrub_addr = 0x3fa000;
     scrub_info_addr = 0x3fa002;
   }

   pData = malloc(sizeof(__u32));
   
   //open config for reading
   if((config=fopen(conffilename,"r")) < 0){
        printf("Could not open %s for reading!\n",conffilename);
        return EXIT_FAILURE;
   }
   if(config!=NULL){
	while(fgets(line,255,config)!=NULL){
	   
	   //read path
	   if(strncmp(line, "path_scfile", 11) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^\n]", path);
		printf("%s\n", path);
	   }
	   
	   //read filename
	   if(strncmp(line, "name_scfile", 11) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^\n]", filename);
		printf("%s\n", filename);
	   }
	   
	   //read startaddress
	   if(strncmp(line, "startaddr_scfile", 16) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^h\n]", startaddr);
		printf("%s\n", startaddr);
		sscanf(startaddr, "%x", &startaddrhex);
		printf("%#x\n",startaddrhex);
	   }
	   
	   
	}
   }
   
   //build path
   strcat(path, filename);
   printf("%s\n", path);
   

   //lookup the filesize
   filesize = getFileSize(path);

    
   //open init config file
   if((scrubbing=fopen(path,"r+b")) < 0){
        printf("Could not open %s for reading!\n",scrubbing);
        return EXIT_FAILURE;
   }

   //buffer for the initial config file
   pInitData = malloc(filesize/2*sizeof(__u16));
   pInitDatahlp = (__u8*)pInitData;
   pInitDataoutput = malloc(filesize/2*sizeof(__u16));
   pInitDataSwapped = (__u8*)pInitDataoutput;

   //read the scrubbing file
   if(scrubbing != NULL){
	fread(pInitData, sizeof(__u16), filesize/2, scrubbing);
   }
   fclose(scrubbing);
   printf("%d\n", filesize);

   //calculate the stop address
   stopaddresshex = calculateStopAddress(filesize, startaddrhex);
   //printf("%#x\n", stopaddresshex);
   writeaddress = (__u32)startaddrhex;
   printf("read writeaddress: %#x\n", writeaddress);
   upperaddr = getUpperAddress(writeaddress);
   printf("upper address: %#x\n", upperaddr);
   loweraddr = getLowerAddress(writeaddress);
   printf("lower address: %#x\n", loweraddr);

   //set up init config pointer   
   *pData = getUpperAddress(scrub_info_addr);//0x3f;
   result = rcuFlashWrite(scrub_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fa000, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }

   //position of start address
   *pData = getLowerAddress(scrub_info_addr);//0xa002;
   scrub_addr += 1;
   result = rcuFlashWrite(scrub_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fa001, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }
   

   //write the startaddresses to flash
   *pData = upperaddr;
   result = rcuFlashWrite(scrub_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fa002, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }
   
   *pData = loweraddr;
   scrub_info_addr += 1;
   result = rcuFlashWrite(scrub_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fa003, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }

   //write the stopaddress to the flash
   stopaddress = (__u32)stopaddresshex;
   upperaddr = getUpperAddress(stopaddress);
   loweraddr = getLowerAddress(stopaddress);
   printf("read stopaddress: %#x\n", stopaddress);
   printf("stop address upper: %#x\n", upperaddr);
   printf("stop address lower: %#x\n", loweraddr);

   *pData = upperaddr;
   scrub_info_addr += 1;
   result = rcuFlashWrite(scrub_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fa004, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }

   *pData = loweraddr;
   scrub_info_addr += 1;
   result = rcuFlashWrite(scrub_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3fa005, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }

   //swap the bytes
   for(i = 0; i < filesize/2; i++){
	*pInitDataSwapped = *((pInitDatahlp)+1);
	*(pInitDataSwapped+1) = *(pInitDatahlp);
   	pInitDataSwapped+=2;
	pInitDatahlp+=2;
   }

   /*just to test if everything is working properly   
   // write the stuff to a file
   if((test=fopen("blubb","w+b")) < 0){
        printf("Could not open %s for reading!\n","blubb");
        return EXIT_FAILURE;
   }
   fwrite(pInitDataoutput, sizeof(__u16), filesize/2, test);
   fclose(test);
   */

   //write the initial config file
   rcuFlashWrite(writeaddress, (__u32*)pInitDataoutput, filesize/2, 2); 
   

   printf("... finished scrubbing\n");
   free(pData);
   //free(pInitDatabgn);
   free(pInitDataoutput);
   fclose(config);

return result;
}

//do all this if the init config file has to be written
int doInit(char *conffilename,int BB_FLASH){
   FILE *config, *initconfig, *test;
   int result, startaddrhex, stopaddresshex;
   char path[64],
	filename[64],
	startaddr[64],
	line[255];
   int filesize, i;
   __u32 writeaddress, stopaddress;
   __u32 *pData;
   __u16 *pInitData, *pInitDatabgn, *pInitDataoutput;
   __u8 *pInitDataSwapped, *pInitDatahlp;
   __u16 upperaddr, loweraddr;
    __u32 init_addr, init_info_addr;

   i = 0;
   writeaddress = 0;
   result = 0;
   startaddrhex = 0;
   stopaddress = 0;
   upperaddr = 0;
   loweraddr = 0;
   if(BB_FLASH==TRUE){
     init_addr = 0x0;
     init_info_addr = 0x3000;
   }else{
     init_addr = 0x3f9000;
     init_info_addr = 0x3f9002;
   }
    
   pData = malloc(sizeof(__u32));
   
   //open config for reading
   if((config=fopen(conffilename,"r")) < 0){
        printf("Could not open %s for reading!\n",conffilename);
        return EXIT_FAILURE;
   }
   if(config!=NULL){
	while(fgets(line,255,config)!=NULL){
	   
	   //read path
	   if(strncmp(line, "path_icfile", 11) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^\n]", path);
		printf("%s\n", path);
	   }
	   
	   //read filename
	   if(strncmp(line, "name_icfile", 11) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^\n]", filename);
		printf("%s\n", filename);
	   }
	   
	   //read startaddress
	   if(strncmp(line, "startaddr_icfile", 16) == 0){
		sscanf(line, "%*[^ \t=]%*[\t ]=%*[\t ]%[^h\n]", startaddr);
		printf("%s\n", startaddr);
		sscanf(startaddr, "%x", &startaddrhex);
		printf("%#x\n",startaddrhex);
	   }
	   
	   
	}
   }
   
   //build path
   strcat(path, filename);
   printf("%s\n", path);
   
 
   //lookup the filesize
   filesize = getFileSize(path);
 printf("Read filesize is: %d\n", filesize);
    
   //open init config file
   if((initconfig=fopen(path,"r+b")) < 0){
        printf("Could not open %s for reading!\n",initconfig);
        return EXIT_FAILURE;
   }

   //buffer for the initial config file
   pInitData = malloc(filesize/2*sizeof(__u16));
   pInitDatahlp = (__u8*)pInitData;
   pInitDataoutput = malloc(filesize/2*sizeof(__u16));
   pInitDataSwapped = (__u8*)pInitDataoutput;

   //read the initconfig file
   if(initconfig != NULL){
	fread(pInitData, sizeof(__u16), filesize/2, initconfig);
   }
   fclose(initconfig);
   printf("%d\n", filesize);

   //calculate the stop address
   stopaddresshex = calculateStopAddress(filesize, startaddrhex);
   //printf("%#x\n", stopaddresshex);
   writeaddress = (__u32)startaddrhex;
   printf("read writeaddress: %#x\n", writeaddress);
   upperaddr = getUpperAddress(writeaddress);
   printf("upper address: %#x\n", upperaddr);
   loweraddr = getLowerAddress(writeaddress);
   printf("lower address: %#x\n", loweraddr);

   //set up init config pointer   
   *pData = getUpperAddress(init_info_addr) ;//0x3f;
   result = rcuFlashWrite(init_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3f9000, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }

   //position of start address
   *pData = getLowerAddress(init_info_addr);//0x9002;
   init_addr += 1;
   result = rcuFlashWrite(init_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3f9001, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }
   

   //write the startaddresses to flash
   *pData = upperaddr;
   result = rcuFlashWrite(init_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3f9002, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }
   
   *pData = loweraddr;
   init_info_addr += 1;
   result = rcuFlashWrite(init_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3f9003, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }

   //write the stopaddress to the flash
   stopaddress = (__u32)stopaddresshex;
   upperaddr = getUpperAddress(stopaddress);
   loweraddr = getLowerAddress(stopaddress);
   printf("read stopaddress: %#x\n", stopaddress);
   printf("stop address upper: %#x\n", upperaddr);
   printf("stop address lower: %#x\n", loweraddr);

   *pData = upperaddr;
   init_info_addr += 1;
   printf(" 1 %#x\n", init_info_addr);
   result = rcuFlashWrite(init_info_addr, pData, 1, 2);
   if(result < 0){
	printf("rcuFlashWrite failed: 0x3f9004, %#x, 1, 2: %d", *pData, result);
	return EXIT_FAILURE;
   }

     *pData = loweraddr;
     init_info_addr += 1;
     printf("2 %#x\n", init_info_addr);
    result = rcuFlashWrite(init_info_addr, pData, 1, 2);
     if(result < 0){
   	printf("rcuFlashWrite failed: 0x3f9005, %#x, 1, 2: %d", *pData, result);
   	return EXIT_FAILURE;
     }
   

   //swap the bytes
   for(i = 0; i < filesize/2; i++){
	*pInitDataSwapped = *((pInitDatahlp)+1);
	*(pInitDataSwapped+1) = *(pInitDatahlp);
   	pInitDataSwapped+=2;
	pInitDatahlp+=2;
   }

   /*just to test if everything is working properly   
   // write the stuff to a file
   if((test=fopen("blubb","w+b")) < 0){
        printf("Could not open %s for reading!\n","blubb");
        return EXIT_FAILURE;
   }
   fwrite(pInitDataoutput, sizeof(__u16), filesize/2, test);
   fclose(test);
   */

   //write the initial config file
   rcuFlashWrite(writeaddress, (__u32*)pInitDataoutput, filesize/2, 2); 
   
   printf("... finished initial flash configuration\n");

   free(pData);

   fflush(stdout);
   //free(pInitDatabgn);

   fflush(stdout);
   free(pInitDataoutput);
   fclose(config);
return result;
}
