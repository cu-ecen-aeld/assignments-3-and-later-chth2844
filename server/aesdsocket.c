//Filename: aesdsocket.c
//Description: server socket implementation
//Author: Chirayu Thakur

#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

//#defines
#define PORT_NO "9000"
#define SOCKET_PROTOCOL 0
#define LISTEN_BACKLOG 5
#define BUFFER_LEN 100
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define FILE_PERMISSIONS 0644

//global variables
int socket_fd,client_fd,fd;

void sig_handler(int signo)
{
  if(signo == SIGINT || signo == SIGTERM)
  { 
    
    syslog(LOG_DEBUG, "Caught signal, exiting\n");
    
    //if(close(fd) < 0)
    //{
     // syslog(LOG_ERR,"Cannot close file descriptor\n");
   // } 
    
    if(close(socket_fd)<0)
    {
      syslog(LOG_ERR,"Cannot close server socket file descriptor\n");
    }
    
    if(close(client_fd)<0)
    {
      syslog(LOG_ERR,"Cannot close client socket file descriptor\n");
    }
    
    //if(remove(FILE_PATH)<0)
    //{
     // syslog(LOG_ERR,"Cannot remove file\n");
    //}
    
    
    
    exit(0);
    
    }
    
}

int main(int argc, char *argv[])
{  
   int rc=-1;
   int status;
   char *read_buff;
   char *write_buff;
   int bytes_read = 0;
   int bytes_write = 0;
   struct addrinfo hints;
   struct addrinfo *res; //points to result of getaddrinfo
   struct sockaddr_in client_addr;
   socklen_t client_addr_size;
   client_addr_size = sizeof(struct sockaddr_in);
   char *client_ip;
   sigset_t signmask;
   
   
   
   memset(&hints,0,sizeof(hints)); //make sure struct is empty
   hints.ai_family = AF_INET; // don't care IPV4, IPV6
   hints.ai_socktype = SOCK_STREAM; //TCP stream sockets
   hints.ai_flags = AI_PASSIVE; //fill in IP for me 
   
   
   
   openlog(NULL,0,LOG_USER);  //open log
   
   
   if(signal(SIGINT,sig_handler) == SIG_ERR) 
   {
     syslog(LOG_ERR,"Cannot handle SIGINT!\n");
     return rc;
   } 
   
   if(signal(SIGTERM,sig_handler) == SIG_ERR) 
   {
     syslog(LOG_ERR,"Cannot handle SIGTERM!\n");
     return rc;
   } 
   
   if((socket_fd = socket(AF_INET,SOCK_STREAM,SOCKET_PROTOCOL))<0)
   {
   
    syslog(LOG_ERR,"Cannot obtain socket file descriptor\n");
    return rc;
   
   }
   
   if(socket_fd == -1)
   { 
     syslog(LOG_ERR,"Cannot obtain socket file descriptor\n");
     return rc;
   }
   
   if((status = getaddrinfo(NULL,PORT_NO,&hints,&res)) !=0)
   {  
     syslog(LOG_ERR,"getaddrinfo error\n");
     return rc;
   
   }
   
   if((status = bind(socket_fd,res->ai_addr,sizeof(struct sockaddr))<0))
   { 
     syslog(LOG_ERR,"bind error\n");
     return rc;
   }
   
   freeaddrinfo(res);
   
   
   
   if((status = listen(socket_fd,LISTEN_BACKLOG))<0)
   {
      syslog(LOG_ERR,"listen error\n");
      return rc;
   }
    
   
    
    if ((sigemptyset(&signmask) == -1) || (sigaddset(&signmask, SIGINT) == -1) || (sigaddset(&signmask, SIGTERM) == -1))
    {
       syslog(LOG_ERR,"failed to empty signal set");
    }
      
    
    while(1)
    {
       if((fd = open(FILE_PATH, O_RDWR | O_CREAT, FILE_PERMISSIONS) == -1))
     {
     
     
     syslog(LOG_ERR,"Cannot open file\n");
     printf("cannot open file\n");
     return rc;
     
     }
      
      
      if((client_fd = accept(socket_fd,(struct sockaddr *)&client_addr,&client_addr_size))<0)
      {
        syslog(LOG_ERR,"accept connection failed\n");
        return rc;
      
      }
      
     if ((client_ip = inet_ntoa(client_addr.sin_addr)) == NULL)
     {
        syslog(LOG_ERR,"client ip failed\n");
        return rc;
      
     }
     
     else{
     
     syslog(LOG_DEBUG,"Accepted connection from %s\n", client_ip);
     
     }
     
     if (sigprocmask(SIG_BLOCK, &signmask, NULL) == -1)
     {
        syslog(LOG_ERR,"signal block failed\n");
         return rc;
     }
     
     
      read_buff = (char *) malloc(sizeof(char) * BUFFER_LEN);
      
     do{
     
         if((bytes_read = recv(client_fd,read_buff,BUFFER_LEN-1,0))<0)
        {
         syslog(LOG_ERR,"recv error\n");
         return rc;
        }
     
     }while(strchr(read_buff,'\n') == NULL);
     
     printf("bytes_read %d\n",bytes_read);
     
     read_buff[bytes_read]='\0';
         
     if((bytes_write = write(fd,read_buff,bytes_read))<0)
     {
        syslog(LOG_ERR,"write error\n");
        return rc;
         
     }
         
     printf("read_buff: %s\n",read_buff);
         
     lseek(fd,0,SEEK_SET);
         
     write_buff = (char *) malloc(sizeof(char) * BUFFER_LEN);
         
         
     if((bytes_read = read(fd,write_buff,BUFFER_LEN))<0)
       {
           syslog(LOG_ERR,"read error\n");
           return rc;
       }
         
       printf("write buffer: %s\n",write_buff);
       printf("bytes read from file %d\n",bytes_read);
         
         
       if (send(client_fd, write_buff,bytes_read, 0) < 0)
       {
            syslog(LOG_ERR,"send error\n");
            return rc;
            
       }
      
      
       if (sigprocmask(SIG_UNBLOCK, &signmask, NULL) == -1)
     {
       syslog(LOG_ERR,"signal unblock failed\n");
       return rc;
     }
      
         free(read_buff);
         free(write_buff);
         close(client_fd);
         close(fd);
         
     }

         
     closelog();
     rc =0;
     
     return rc;
         
         
         
  }
    
