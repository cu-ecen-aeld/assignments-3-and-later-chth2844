//Filename: writer.c
//Description: Implements file creation and file write according to path and text string provided by user
//Author: Chirayu Thakur
//Date: 09/04/2021

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdint.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
   
   
   openlog(NULL,0,LOG_USER);  //open log
   
   if(argc!=3)
   {
     syslog(LOG_ERR,"Invalid Number of Arguments: %d",argc);   //invalid number of Arguments
     return 1;
     
   }
   
   else {
          const char *filename = argv[1];                     //filename provided by user
          
          int fd = open(filename,O_RDWR|O_CREAT|O_TRUNC,S_IRWXU|S_IRWXG|S_IRWXO);  //open file and truncate to zero if it exists, if not create .txt file
          
          if (fd == -1)
          {
            syslog(LOG_ERR,"Error opening: %s",filename);  //file creation unsucessfull 
            return 1;
          }
          
          else{
          
               const char *writestr = argv[2];            //text string provided by user
               
               ssize_t nr;
               
               nr=write(fd,writestr,strlen(writestr));    //write text string into file 
               
               if(nr==-1)
               {
          
                  syslog(LOG_ERR,"error writing string to file: %s",filename); //file write unsuccessfull 
                  return 1;
               }
          
              else
              {
          
                  syslog(LOG_DEBUG,"Writing %s to %s",filename,writestr);  //file write sucessfull 
                 
              
              }
                 
   
   
            }
         }
   
   closelog(); //close log
   return 0;
}
