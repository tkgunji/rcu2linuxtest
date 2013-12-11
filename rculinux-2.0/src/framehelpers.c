// $Id: framehelpers.c,v 1.3 2008/02/27 07:47:54 richter Exp $

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

/** @file       framehelpers.c
    @author     Dominik Fehlker
    @date
    @brief      Helpfunctions for programs that have to deal with Frames 
*/

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>        // error codes
#include "framehelpers.h"
#include "dcscMsgBufferInterface.h"

#define EXIT_FAILURE -1

__u16 getUpperAddress(__u32 u32address){
   __u16 u16address;
   u16address = 0;

   u16address = u32address >> 16;

return u16address;
}



__u16 getLowerAddress(__u32 u32address){
   __u16 u16address;
   u16address = 0;

   u16address = u32address;

return u16address;
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



int calculateStopAddress(int filesize, int startaddress){
   int stopaddress, result;
   __u32 u32rawdata;

   stopaddress = 0;
   result = 0;

   stopaddress = (startaddress + (filesize / 2) - 1);
   //printf("%#x\n", stopaddress);

   /* check if we have a buggy firmware, therefor change
      back to MsgBuf Mode first. */
   result = rcuBusControlCmd(eEnableMsgBuf);
   if(result < 0){
      printf("Error entering Msg Mode: %d\n", result);
   }

   result = rcuSingleRead(0xb102,&u32rawdata);
   if(result < 0){
        printf("Error during rcuSingleRead 0xb102: %d", result);
        return EXIT_FAILURE;
   }

   result = rcuBusControlCmd(eEnableFlash);
   if(result < 0){
      printf("Error entering Flash Mode: %d\n", result);
   }

   //correct the FW Error
   if(u32rawdata == 0xffff0102)
        stopaddress += 100;

return stopaddress;
}



int enterFlashState(int oldstate){
   int result;

   switch(oldstate){
        case 2: //already in Flash State
                break;
        case 1: //state was Selectmap
                result = rcuBusControlCmd(eEnableMsgBuf);
                if(result < 0){
                   printf("Error entering Msg Mode: %d\n", result);
                }

        case 3:
                //State was MsgBuf or SelectMap - switching to Flash
                result = rcuBusControlCmd(eEnableFlash);
                if(result < 0){
                   printf("Error entering Flash Mode: %d\n", result);
                }
                break;
        default:result = -1;
                printf("Error during detection of Bus State - aborting\n");
                break;
   }

return result;
}


int getBusState(){
   int state, result;
   state = 0;
   result = 0;

   result = rcuBusControlCmd(eCheckSelectmap);
   if(result == 1)
        state = 1;
   result = 0;
   result = rcuBusControlCmd(eCheckFlash);
   if(result == 1)
        state = 2;
   result = 0;
   result = rcuBusControlCmd(eCheckMsgBuf);
   if(result == 1)
        state = 3;
   result = 0;

return state;
}


int restoreBusState(int oldstate){
   int result;
   result = 0;

   switch(oldstate){
        case 2: // was already in Flash State
                break;
        case 1:
                //State was MsgBuf or SelectMap - switching to Flash
                result = rcuBusControlCmd(eEnableMsgBuf);
                if(result < 0){
                   printf("Error entering MsgBuf Mode: %d", result);
                }
                result = rcuBusControlCmd(eEnableSelectmap);
                if(result < 0){
                   printf("Error entering Selectmap Mode: %d", result);
                }
                break;
        case 3:
                //State was MsgBuf or SelectMap - switching to Flash
                result = rcuBusControlCmd(eEnableMsgBuf);
                //State was MsgBuf or SelectMap - switching to Flash
                result = rcuBusControlCmd(eEnableMsgBuf);
                if(result < 0){
                   printf("Error entering MsgBuf Mode: %d", result);
                }
                break;
        default:result = -1;
                printf("Error while restoring original Bus state");
                break;
   }
return result;
}


int getFrameAddressFromLine(char line[255], int *pframeaddr){
   int block, major, minor;

   block = -1;
   major = -1;
   minor = -1;

   switch(line[0]){
        case '#'://sprintf(filename,"frame_%d_%d_%d.hex",block,major,minor);
                break;
        case '\0':break;
        case '0':
        case '1':
        case '2':
        case '3':
                block=charToInt(line[0]);
                major=charToInt(line[2]);
                switch(line[3]){
                        case ';':
                                minor=charToInt(line[4]);
                                if(line[5]!=';'){
                                        minor*=10;
                                        minor+=charToInt(line[5]);
                                        if(line[6]!=';'){
                                                minor*=10;
                                                minor+=charToInt(line[6]);
                                        }
                                }
                                break;
                        default:
                                major*=10;
                                major+=charToInt(line[3]);
                                switch(line[4]){
                                        case ';':
                                                minor=charToInt(line[5]);
                                                if(line[6]!=';'){
                                                        minor*=10;
                                                        minor+=charToInt(line[6]);
                                                        if(line[7]!=';'){
                                                                minor*=10;
                                                                minor+=charToInt(line[7]);
                                                        }
                                                }
                                                        break;
                                        default:
                                                major*=10;
                                                major+=charToInt(line[4]);
                                                minor=charToInt(line[6]);
                                                if(line[7]!=';'){
                                                        minor*=10;
                                                        minor+=charToInt(line[7]);
                                                        if(line[8]!=';'){
                                                                minor*=10;
                                                                minor+=charToInt(line[8]);
                                                        }
                                                }
                                                break;
                                }
                                break;

                        }
               // printf("processing ");
               // printf("Block %d,",block);
               // printf("Major %d,",major);
               // printf("Minor %d",minor);
               // printf("\n");
                //sprintf(ReadbackRawFrameFilename,"frame%d.%d.%d.hex",block,major,minor);
                //sprintf(ErrorFrameFilename,"Err_frame%d.%d.%d.hex",block,major,minor);
		*pframeaddr++ = block;
                *pframeaddr++ = major;
                *pframeaddr = minor;

                break;
        default:break;
   }
}


// maybe stupid, but works for now
int charToInt(char zeichen){
int value;
   switch(zeichen){
        case '0':value=0;
                break;
        case '1':value=1;
                break;
        case '2':value=2;
                break;
        case '3':value=3;
                break;
        case '4':value=4;
                break;
        case '5':value=5;
                break;
        case '6':value=6;
                break;
        case '7':value=7;
                break;
        case '8':value=8;
                break;
        case '9':value=9;
                break;
        default:break;
   }
return value;
}


int getLinesnumberFromFile(char *filename){
   int result;
   FILE *frames;
   char line[255];
   int *pData;
   int data[3];
   result = 0;

   //open frames.txt for reading
   if((frames=fopen(filename,"r")) < 0){
        printf("Could not open %s for reading!\n",filename);
        return EXIT_FAILURE;
   }

   //if there is something to readout
   while(fgets(line,255,frames)!=NULL){
        //read line from framesfile
        pData = &data[0];
        getFrameAddressFromLine(line, pData);

        if(data[0] != -1){
                result++;
        }
   }

   fclose(frames);

return result;
}


int getHexValueFromLine(char line[255], unsigned short *pframeaddress){
   int value;
   char shortline[4];
   //value = -1;

   if(strncmp(line,"#",1) == 0){
        //value = -1;
        //printf("found a comment\n");
   }
   else if(strncmp(line,"$frame_addr31_16",15) == 0){
	//printf("pframeaddress zeigt auf: %#x", *pframeaddress);
        //printf("found $frame_addr32_16\n");
        value=*pframeaddress;

        //printf("address31_16: %x\n",value);
   }
   else if(strncmp(line,"$frame_addr15_0",15) == 0){
        //printf("found $frame_addr15_0\n");
        value=*(pframeaddress+1);
 	
        //printf("address15_0: %x\n",value);
   }
   else{
        strncpy(shortline,line,4);
        sscanf (shortline, "%x", &value);
        //printf("normal command: %x\n",value);
   }

return value;
}

