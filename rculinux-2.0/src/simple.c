/*                                                                                                                                                      
 * This is a user-space application that reads /dev/sample                                                                                              
 * and prints the read characters to stdout                                                                                                             
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(int argc, char **argv)
{
  char * app_name = argv[0];
  char * dev_name = "/dev/dcsc";
  int ret = -1;
  int fd = -1;
  int c, x;
  char *cmd = argv[1];
  size_t len = 0;


  if(argc!=3){
    fprintf(stderr, "./app rd length\n");
    fprintf(stderr, "./app wr data\n");
    goto Done;
  }
  /*                                              
   * Open the sample device RD | WR                                                                                                                     
   */
  if ((fd = open(dev_name, O_RDWR)) < 0) {
    fprintf(stderr, "%s: unable to open %s: %s\n" ,
            app_name, dev_name, strerror(errno));
    goto Done;
  }

  if(strcmp(cmd,"wr")==0){
    c = atoi(argv[2]);
    fprintf(stdout,"execute write command %d\n", c);
    if ((x = write(fd, &c, sizeof(c))) < 0) {
      fprintf(stderr, "%s: unable to write %s: %s\n",
              app_name, dev_name, strerror(errno));
      goto Done;
    }
  }

  /*                                                                                                                                                    
   * Read the sample device byte-by-byte                                                                                                                
   */
  if(strcmp(cmd,"rd")==0){
    len = atoi(argv[2]);
    fprintf(stdout, "execute read command\n");
    if ((x = read(fd, &c, len)) < 0) {
      fprintf(stderr, "%s: unable to read %s: %s\n",
              app_name, dev_name, strerror(errno));
      goto Done;
    }
    fprintf(stdout, "%d\n", c);
  }


  /*                                                                                                                                                    
   * If we are here, we have been successful                                                                                                            
   */
  ret = 0;

 Done:
  if (fd >= 0) {
    close(fd);
  }
  return ret;
}


