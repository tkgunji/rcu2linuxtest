// $Id: framever.c,v 1.4 2008/02/27 07:47:54 richter Exp $

/****************************************************************************
*
* Frameverifier - framever
*
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
**
* a utility to verify frames from the Xilinx memory against the ones and the flash
* and the ones stored on a drive. In case of an error, and Errorfile is created
* which contains the the positions of the bitflips. Also the errors are counted
* for every frame, every cycle and the total number of errors during all cycles.
* For a complete run a logfile log.txt will be created, containing these information.
* 
* usage: framever frames.txt 20
*                     ^      ^- optional number of cycles, if not given a standard is used
*                     '-------- File containing the frames in a way like
                                block;major;minor; - for example "0;5;3;"     
*
* written by Dominik Fehlker, dfehlker@htwm.de, University of Applied Sciences Mittweida
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> //for usleep
#include "dcscMsgBufferInterface.h"
#include "framever.h"
#include <sys/stat.h>
#include <sys/time.h>
//#include "framehelpers.h"

// for the half-baked interpreting of registers
//char meaning[255];

int main(int argc, char **argv){
   __u32 *pSrc;
   __u32 rawData;
   __u16 *pTgt, *pTgtoutput, *pRawData, *pErrFileDataoutput,*pErrFileData;
   __u32 *pRBData;
   //for the files that are opened
   int block, major, minor;
   FILE *frames;
   FILE *logfile;
   FILE *errfile;
   FILE *rbrawfile;
   FILE *rawfile;
   char pathandfilename[96]="";
   char errpathandfilename[96];
   char line[255];
   char putline[255];
   char foldername[96]="";
   int data[212];
   int frameaddress[3];
   int *pframeaddress;
   __u16 rawdata[212];
   int errfiledata[212];
   int cycles, result,n, linecounter, cyclestoreach, i,erroroccured, cycleserrors, linenumber, linenumbercount, overallerrors;
   //for the time
   struct tm *l_time;
   time_t now;
   char mytime[20];
   //for the Hardwareerrorregisters
   unsigned short hwnoferrorsreg;
   unsigned short hwnoflastframewitherr;
   unsigned short hwnoflastframe;
   unsigned short hwnofcycles;
   unsigned short hwstatusreg;
   unsigned short hwerrreg;
   char ReadbackRawFrameFilename[64];
   char RawFrameFilename[64];
   char ErrorFrameFilename[64];
   pframeaddress = &frameaddress[0];
   block = -1;
   major = -1;
   minor = -1;
   cycles = 0;
   cyclestoreach = 0;
   n = 0;
   i = 0;
   linecounter = 0;
   hwnoferrorsreg = 0;
   hwnoflastframewitherr = 0;
   hwnoflastframe = 0;
   hwnofcycles = 0;
   hwerrreg = 0;
   hwstatusreg = 0;
   erroroccured = 0;
   cycleserrors = 0;
   linenumber = 0;
   linenumbercount = 0;
   overallerrors = 0;
   
   //enable writing to the RCU
   initRcuAccess(NULL);

   //initialization of the registers
   result = init();
   if(result < 0){
	printf("Initialization failed: %d\n", result);
	return EXIT_FAILURE;
   }
   
   //create a logfile and write a header to it
   writeHeaderToLogfile();

   // test for the number of parameters
   if((argc != 2) && (argc != 3)) {
        perror("usage: framever <frames> [Cycles]\n");
        return EXIT_FAILURE;
   }
   
   // if no number of cycles is given use a default
   if(argv[2] == NULL){
	cyclestoreach = 2;
	printf("No Parameter for Cycles given, using Cycles = 2\n");
   }
   else{
	cyclestoreach = atoi(argv[2]);
	printf("Using given parameter %d for Cycles.\n", cyclestoreach);
   }
   printf("CyclestoReach: %d\n", cyclestoreach);

   // count the number of frames to be handled
   linenumbercount = linenumber = getLinesnumberFromFile(argv[1]);


   //open frames.txt for reading
   if((frames=fopen(argv[1],"r")) < 0){
        printf("Could not open %s for reading!\n",argv[1]);
        return EXIT_FAILURE;
   }
  

   //open logfile for writing	
   if((logfile=fopen("log.txt","a+b")) < 0){
       	printf("Could not open %s for reading!\n",argv[1]);
       	return EXIT_FAILURE;
   }
   else{
	printf("writing to logfile log.txt.\n");
   }
  
   sprintf(putline,"framever working on %d cycles.\n", cyclestoreach);
   fwrite(putline, strlen(putline), 1, logfile);

 
   //read and show the status registers
   hwnoferrorsreg = getErrorcounterReg();
   hwnoflastframewitherr = getLastErrorFramenumber();
   hwnoflastframe = getLastFramenumber();
   hwnofcycles = getNumberOfCycles();
   hwerrreg = readErrReg();
   hwstatusreg = readStatusReg();
		
   //printf("noferr: %d, lastframewitherr: %d, lastframe: %d, nofcycles: %d, ErrReg: %d, StatusReg: %d\n",hwnoferrorsreg, hwnoflastframewitherr, hwnoflastframe, hwnofcycles, hwerrreg, hwstatusreg);


   sprintf(putline,"Overview about the hardwareregisters before the actual work started:\n");
   printf("%s",  putline);
   fwrite(putline, strlen(putline), 1, logfile);
   sprintf(putline, "Read Error Counter Register (0xb104)	: %d\n", hwnoferrorsreg);
   printf("%s",  putline);
   fwrite(putline, strlen(putline), 1, logfile);
   sprintf(putline, "Last Frame with error (0xb105)	    	: %d\n", hwnoflastframewitherr);
   printf("%s",  putline);
   fwrite(putline, strlen(putline), 1, logfile);
   sprintf(putline, "Last read out frame(0xb106)	    	: %d\n",hwnoflastframe);
   printf("%s",  putline);
   fwrite(putline, strlen(putline), 1, logfile);
   sprintf(putline, "Error register (0xb101)		    	: %#x\n", hwerrreg);
   printf("%s",  putline);
   fwrite(putline, strlen(putline), 1, logfile);
   sprintf(putline, "Status register (0xb100)		    : %#x\n\n", hwstatusreg);
   printf("%s",  putline);
   fwrite(putline, strlen(putline), 1, logfile);
   
   sprintf(putline, "Number of Frames found in %s	 : %d\n\n", argv[1], linenumber);
   printf("%s",  putline);
   fwrite(putline, strlen(putline), 1, logfile);

   //as long as there are frames to be read
   if(frames!=NULL){
        //and as long as we havent reached the number of cycles to reach
	while(cycles < cyclestoreach){

	   //read out time for for the folder generation
   	   time(&now);
   	   l_time = localtime(&now);
   	   strftime(mytime, sizeof mytime, "%T", l_time);
           
	   //generate the folder for this cycle
   	   sprintf(foldername,"cycle_%d_%s",cycles, mytime);
	   result = mkdir(foldername, S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IROTH | S_IWOTH | S_IXOTH);
   	   if(result < 0){
		printf("failed creating folder %s - Error %d\n",foldername, result);
   	   }


  	   sprintf(putline, "current cycle: %d\n=============================\n", cycles);
   	   //printf("%s",  putline);
	   fwrite(putline, strlen(putline), 1, logfile);
   	   strftime(mytime, sizeof mytime, "%D - %T%n", l_time);
	   
   	   sprintf(putline,"%s", mytime);
   	   //printf("%s",  putline);
	   fwrite(putline, strlen(putline), 1, logfile);

	   linenumbercount = linenumber;
	
	    //if there is something to readout
            while(fgets(line,255,frames)!=NULL){
	
	        //read line from framesfile
		pframeaddress = &frameaddress[0];
	        getFrameAddressFromLine(line, pframeaddress);
		pframeaddress = &frameaddress[0];
		block = frameaddress[0];
		major = frameaddress[1];
		minor = frameaddress[2];
		
	

	        if(block != -1){
		     //printf("processing %d, %d, %d\n",block, major, minor);
                     sprintf(ReadbackRawFrameFilename,"frame%d.%d.%d.hex",block,major,minor);
                     sprintf(ErrorFrameFilename,"Err_frame%d.%d.%d.hex",block,major,minor);
                     sprintf(RawFrameFilename,"raw/frame%d.%d.%d.hex",block,major,minor);
		
		     //prepare for reading out
		     result = step();
		     if(result < 0)
		         printf("rcuSingleWrite failed (0xb000, 0x0100), Error: %d\n",result);

		     //read the data
		     result = rcuMultipleRead(0xb41c, 212, data);
        	     //printf("read %d bytes from memory.\n",result);
	
		     //get rid of the unused 16bit
		     pSrc = (__u32*)&data[0];
        	     pTgtoutput = pTgt = (__u16*)malloc(212*sizeof(__u16));
        	     for(n=0;n<212;n++){
            	        *pTgt=*((__u16*)pSrc);
                        pTgt++;
                        pSrc++;
                     }

		     //printf("%s\n",foldername);
		     // generate the filename the data will be written to
		     strcpy(pathandfilename,foldername);
		     strcat(pathandfilename,"/");
		     strcat(pathandfilename,ReadbackRawFrameFilename);

		     //printf("\n%s\n",pathandfilename);
		
		     //write the readout data
                     if((rbrawfile=fopen(pathandfilename,"w+b")) < 0){
                        printf("Error opening %s for ascii writing!\n",pathandfilename);
                	return EXIT_FAILURE;
                     }
		     //printf("done writing raw data to %s.\n", pathandfilename);
		     //clear the filename
		     strcpy(pathandfilename,"\0");

		     //write the data to the file
	             fwrite(pTgtoutput, sizeof(__u16), 212, rbrawfile);
		     
   		     fclose(rbrawfile);

		     //open the raw File for reading and comparison
                     if((rawfile=fopen(RawFrameFilename,"r")) < 0){
                        printf("Error opening %s for reading!\n",RawFrameFilename);
                	return EXIT_FAILURE;
                     }
		     fread(rawdata, sizeof(__u16), 212, rawfile);
		     fclose(rawfile);

		     // do an XOR with the read out data and the correct framefiles
		     pRBData = (__u32*)&data[0];
		     pRawData = &rawdata[0];
		     pErrFileDataoutput = pErrFileData = (__u16*)&errfiledata[0];

		     for(i = 0; i < 212; i++){
			*pErrFileData = ((*pRawData++))^((*pRBData++));
			//analyze for errors and count them
			erroroccured += analyze16bit(*pErrFileData);
			*pErrFileData++;
			//fprintf(stderr, "data=%#x rawdata=%#x errordata=%#x\n", data[i], rawdata[i], errfiledata[i]);
		     }
		   
		     //if a bitflip was detected create an Errorfile wich contains the XOR data 
		     if(erroroccured > 0){
		     	//write the Errorfile	
		     	strcpy(errpathandfilename,foldername);
		     	strcat(errpathandfilename,"/");
		     	strcat(errpathandfilename,ErrorFrameFilename);
			
			printf("found error in frame %d,%d,%d, writing errors to file %s\n", block, major, minor, errpathandfilename);

                     	if((errfile=fopen(errpathandfilename,"w+b")) < 0){
                            printf("Error opening %s for reading!\n",ErrorFrameFilename);
                	    return EXIT_FAILURE;
                         }

		      	fwrite(pErrFileDataoutput, sizeof(__u16), 212, errfile);

		     	fclose(errfile);
		     }
			
		     //read the status registers
		     hwnoferrorsreg = getErrorcounterReg();
		     hwnoflastframewitherr = getLastErrorFramenumber();
		     hwnoflastframe = getLastFramenumber();
		     hwnofcycles = getNumberOfCycles();
		     hwerrreg = readErrReg();
		     hwstatusreg = readStatusReg();
/*
		if(hwerrreg != 0){
		    printf("Hardware Error Register shows %d! - aborting.",hwerrreg);
		    return EXIT_FAILURE;
		}
*/		
		     
  		     //print the Errorregisters to the logfile
  		     sprintf(putline, "current frame: %d, %d, %d - Framenumber: %d\n----------------------------\n", block, major, minor, linenumbercount-1);
		     fwrite(putline, strlen(putline), 1, logfile);
		     sprintf(putline, "Last read out frame(0xb106)	    : %d\n",hwnoflastframe);
		     fwrite(putline, strlen(putline), 1, logfile);
   		     sprintf(putline, "Read Error Counter Register (0xb104): %d\n", hwnoferrorsreg);
		     fwrite(putline, strlen(putline), 1, logfile);
		     sprintf(putline, "Last Frame with error (0xb105)	    : %d\n", hwnoflastframewitherr);
		     fwrite(putline, strlen(putline), 1, logfile);
		     //sprintf(putline, "Error register (0xb101)	 	    : %#x %s\n", hwerrreg, getMeaningOfErrReg(hwerrreg));
		     sprintf(putline, "Error register (0xb101)	 	    : %#x\n", hwerrreg);
		     fwrite(putline, strlen(putline), 1, logfile);
  		     sprintf(putline, "Status register (0xb100)            : %#x\n", hwstatusreg);
		     fwrite(putline, strlen(putline), 1, logfile);

		     
  		     sprintf(putline, "Bitflips found in this frame	    : %d\n\n", erroroccured);
		     fwrite(putline, strlen(putline), 1, logfile);

		     //add up the errors for the whole cycle
		     cycleserrors+=erroroccured;
		     erroroccured = 0;

	//	     printf("Cycle: %d, noferr: %d, lastframewitherr: %d, lastframe: %d, nofcycles: %d, ErrReg: %d, StatusReg: %d\n",cycles, hwnoferrorsreg, hwnoflastframewitherr, hwnoflastframe, hwnofcycles, hwerrreg, hwstatusreg);
	
		    //count the line Framenumbers like the hardware does
		    linenumbercount--;	
		    clearErrReg();

	       }

	   }

	//print detected bitflips to screen and logfile
  	sprintf(putline, "Bitflips found in cycle %d	    : %d\n", cycles, cycleserrors);
   	printf("%s",  putline);
	fwrite(putline, strlen(putline), 1, logfile);
        overallerrors+=cycleserrors;
	sprintf(putline, "Bitflips found so far               : %d\n",overallerrors);
   	printf("%s",  putline);
	
	//reset and start new cycle
	cycleserrors=0;
	cycles++;	    
	rewind(frames);
	}
   }

   //print all detected erorrs and clean up
   sprintf(putline, "Bitflips found during %d cycles with %d frames  : %d\n\n", cyclestoreach, linenumber, overallerrors);
   printf("%s",  putline);
   fwrite(putline, strlen(putline), 1, logfile);
   fclose(logfile);
   printf("closed logfile\n");
   free(pTgtoutput);
   printf("freed memory\n");
   fclose(frames);
   printf("closed framefile\n");
 //  printf("closed rawfile\n");

   releaseRcuAccess();

return EXIT_SUCCESS;
}

int step(){
   int result;
   result = 0;
   result = rcuSingleWrite(0xb000,0x0100);
   usleep(1000);
   return result;
}

__u32 getErrorcounterReg(){
   __u32 u32rawData;
   if(rcuSingleRead(0xb104,&u32rawData) < 0){
	printf("rcuSingleRead failed: 0xb104.");
	return EXIT_FAILURE;
   }
return u32rawData;
}

__u32 getLastErrorFramenumber(){
   __u32 u32rawData;
   if(rcuSingleRead(0xb105, &u32rawData) < 0){
	printf("rcuSingleRead failed: 0xb105.");
	return EXIT_FAILURE;
   }	
return u32rawData;
}

__u32 getLastFramenumber(){
   __u32 u32rawData;
   if(rcuSingleRead(0xb106, &u32rawData) < 0){
	printf("rcuSingleRead failed: 0xb106.");
	return EXIT_FAILURE;
   }	
return u32rawData;
}

__u32 getNumberOfCycles(){
   __u32 u32rawData;
   if(rcuSingleRead(0xb107, &u32rawData) < 0){
	printf("rcuSingleRead failed: 0xb107.");
	return EXIT_FAILURE;
   }		
return u32rawData;
}

__u32 readStatusReg(){
   __u32 u32rawData;
   if(rcuSingleRead(0xb100, &u32rawData) < 0){
	printf("rcuSingleRead failed: 0xb100.");
	return EXIT_FAILURE;
   }		
return u32rawData;
}

__u32 readErrReg(){
   __u32 u32rawData;
   if(rcuSingleRead(0xb101, &u32rawData) < 0){
	printf("rcuSingleRead failed: 0xb101.");
	return EXIT_FAILURE;
   }		
return u32rawData;
}

int clearErrReg(){
   int result;
   result = 0;
   if(rcuSingleWrite(0xb000, 0x1) < 0){
	printf("rcuSingleWrite failed: 0xb000 0x1.");
	result = -1;
	return EXIT_FAILURE;
   }		
return result;
}

int init(){
   int result;
   int data[1];
   __u32 *pData;

   data[0]=0xffffffff;
   result = 0;
   pData = (__u32*)&data[0];
   
   /* stops whatever the controller is doing */
   if(rcuSingleWrite(0xb000, 0x8000) < 0){
	printf("rcuSingleWrite failed: 0xb000, 0x8000\n");
	result=-1;
	return EXIT_FAILURE;
   }
   /* overwrites the selectmap memory */
   if(rcuMultipleWrite(0xb400, pData, 512, 2) < 0){
	printf("rcuMultipleWrite failed: 0xb400, 0xFFFF, 512, 2\n");
	result = -2;
	return EXIT_FAILURE;
   }

   /* overwrites the flash cache */
   if(rcuMultipleWrite(0xb200, pData, 512, 2) < 0){
	printf("rcuMultipleWrite failed: 0xb200, 0xFFFF, 512, 2\n");
	result=-3;
	return EXIT_FAILURE;
   }

   /* clear the error register */
   if(rcuSingleWrite(0xb000, 0x1) < 0){
	printf("rcuSingleWrite failed: 0xb000, 0x1\n");
	result=-4;
	return EXIT_FAILURE;
   }
   /* clear the status register */
   if(rcuSingleWrite(0xb000, 0x2) < 0){
	printf("rcuSingleWrite failed: 0xb000, 0x2\n");
	result=-5;
	return EXIT_FAILURE;
   }

return result;
}


int writeHeaderToLogfile(){
   int result;
   char putline[128];
   FILE *logfile;
   //for the time
   struct tm *l_time;
   time_t now;
   char templine[64];

   result = 0;

   sprintf(putline,"Logfile was generated by FrameVer at ");

   //read out time for Timestamp in Logfile
   time(&now);
   l_time = localtime(&now);
   strftime(templine, sizeof templine, "%c", l_time);

   //open logfile for writing	
   if((logfile=fopen("log.txt","w+b")) < 0){
       	printf("Could not open log.txt for reading!\n");
       	return EXIT_FAILURE;
   }

   strcat(putline, templine);
   strcat(putline, "\n");
   fwrite(putline, strlen(putline), 1, logfile);
   strcpy(putline,"=============================================================\n\n");
   fwrite(putline, strlen(putline), 1, logfile);
   

   fclose(logfile);
return result;
}


int analyze16bit(__u16 bitfield){
   int result;
   unsigned int bitcounter;
   result = 0;
   for(bitcounter=1<<(15); bitcounter!=0; bitcounter>>=1) //loop through all bits
        if ((bitcounter & bitfield) != 0) result++; // count 1-bits

return result;
}

/*
void getMeaningOfErrReg(int value){
  __u16 bitfield;
  bitfield = (__u16)value;
  char *pmeaning;
  pmeaning = (char*)

  strcpy(meaning,""); 
  
  if((bitfield & 1) != 0)
	strcat(meaning,"- Smap done failed");
  if(((bitfield>>=1) & 1) != 0)
	strcat(meaning,"- Smap busy not asserted error");
  if(((bitfield>>=1) & 1) != 0)
	strcat(meaning,"- Smap unknown command");
  if(((bitfield>>=1) & 1) != 0)
	strcat(meaning,"- Smap state machine error");
  if(((bitfield>>=1) & 1) != 0)
	strcat(meaning,"- Flash interface error");
  if(((bitfield>>=1) & 1) != 0)
	strcat(meaning,"- XCM state machine error");
  if(((bitfield>>=1) & 1) != 0)
	strcat(meaning,"- Flash RAM parity error");
  if(((bitfield>>=1) & 1) != 0)
	strcat(meaning,"- Selectmap RAM parity error");
  strcat(meaning,".");

printf("%s", meaning); 
}
*/
