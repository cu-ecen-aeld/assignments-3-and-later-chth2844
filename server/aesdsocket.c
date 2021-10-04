//Filename: aesdsocket.c
//Description: server socket implementation
//Date: 10/03/2021
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
#include <ctype.h>

//#defines
#define PORT_NO "9000"
#define SOCKET_PROTOCOL 0
#define LISTEN_BACKLOG 5
#define BUFFER_LEN 1000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define FILE_PERMISSIONS 0644

//global variables
int socket_fd,client_fd,file_fd;
int total_bytes=0;
int byte_total=0;
int realloc_val = 1;
int pid;
char *read_buff;
char *write_buff;
int bytes_read = 0;
int bytes_write = 0;

//frees memory ,closes files and socket
void free_memory()
{

  close(socket_fd);
  closelog();
  free(read_buff);
  free(write_buff);

}

//signal handler for SIGINT and SIGTERM
void sig_handler(int signo)
{
  if(signo == SIGINT || signo == SIGTERM)
  { 
    
    syslog(LOG_DEBUG, "Caught signal, exiting\n");
    

    if(close(socket_fd)<0)
    {
      syslog(LOG_ERR,"Cannot close server socket file descriptor\n");
    }
    
    if(close(client_fd)<0)
    {
      syslog(LOG_ERR,"Cannot close client socket file descriptor\n");
    }
    
    if(close(file_fd) < 0)
    {
      syslog(LOG_ERR,"Cannot close file descriptor\n");
    } 
    
    
    if(remove(FILE_PATH)<0)
    {
      syslog(LOG_ERR,"Cannot remove file\n");
    }
    
    
    
    exit(0);
    
    }
    
}

int main(int argc, char *argv[])
{  

   int rc=-1;
   int status;
   int isdaemon=0;
   int reuse_addr =1;
   int bytes_sent;
   int buf_loc = 0;
   struct addrinfo hints;
   struct addrinfo *res; //points to result of getaddrinfo
   struct sockaddr_in client_addr;
   socklen_t client_addr_size;
   client_addr_size = sizeof(struct sockaddr_in);
   char *client_ip;
   int read_bytes1;
   
   
   
   
   memset(&hints,0,sizeof(hints)); //make sure struct is empty
   hints.ai_family = AF_INET; // don't care IPV4, IPV6
   hints.ai_socktype = SOCK_STREAM; //TCP stream sockets
   hints.ai_flags = AI_PASSIVE; //fill in IP for me 
   
   
   
   openlog("aesdsock",LOG_PID,LOG_USER);  //open log
   
   
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
   
   if((argc == 2) && (strcmp("-d", argv[1]) == 0))
   {
		
      isdaemon = 1;          //create daemon
			
    }
    
   //create socket file descriptor
   if((socket_fd = socket(AF_INET,SOCK_STREAM,SOCKET_PROTOCOL))<0)
   {
   
    syslog(LOG_ERR,"Cannot obtain socket file descriptor\n");
    return rc;
   
   }
   
   //resuse socket address 
   if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)) < 0) 
   {
        syslog(LOG_ERR, "reuse addreess failed");
        close(socket_fd);
	closelog();
        return rc;
    }
   
   //getting address 
   if((status = getaddrinfo(NULL,PORT_NO,&hints,&res)) !=0)
   {  
     syslog(LOG_ERR,"getaddrinfo error\n");
     return rc;
   
   }
   
   //binding address to socket
   if((status = bind(socket_fd,res->ai_addr,sizeof(struct sockaddr))<0))
   { 
     syslog(LOG_ERR,"bind error\n");
     return rc;
   }
   
   //free addrinfo data structure
   freeaddrinfo(res);
   
   if(isdaemon==1)
    {  
       int devnull_fd;
       int session_id;
       
    	pid = fork();

    	if (pid < 0 ) //error in fork
    	{
	    syslog(LOG_ERR, "fork error\n");
	}

    	if(pid > 0)       //parent process
    	{
	    syslog(LOG_DEBUG, "daemon created with success\n");
	    exit(0);
    	}
    	
    	if(pid == 0)   //child process
    	{
	    session_id = setsid();  //set session id

            if(session_id == -1)
	    {
	    	syslog(LOG_ERR, "error in setting sid\n");
		close(socket_fd);
		closelog();
	    	exit(EXIT_FAILURE);
	    }

            if (chdir("/") == -1)   //change directory
	    {
            	syslog(LOG_ERR, "chdir error");
            	closelog();
            	close(socket_fd);
            	return rc;
            }

    	     devnull_fd = open("/dev/null", O_RDWR);
            dup2(devnull_fd, STDIN_FILENO);
            dup2(devnull_fd, STDOUT_FILENO);
            dup2(devnull_fd, STDERR_FILENO);
            
            //close file descriptors 
            close(STDIN_FILENO);  
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
    	}
    }
   
   
   //listen to client connections 
   if((status = listen(socket_fd,LISTEN_BACKLOG))<0)
   {
      syslog(LOG_ERR,"listen error\n");
      return rc;
   }
    
    read_buff = (char *) malloc(sizeof(char) * BUFFER_LEN);
    write_buff = (char *) malloc(sizeof(char) *BUFFER_LEN );
    
    file_fd = open(FILE_PATH,O_RDWR | O_APPEND | O_CREAT,FILE_PERMISSIONS); //open file 
   
    if(file_fd==-1)
     {
      syslog(LOG_ERR,"Cannot open file\n");
      return 0;
     
     }
     
     
   
    
    while(1)
    {
         
         
      if((client_fd = accept(socket_fd,(struct sockaddr *)&client_addr,&client_addr_size))<0)  //accept connection from client socket
      {
        syslog(LOG_ERR,"accept connection failed\n");
        return rc;
      
      }
      
     if ((client_ip = inet_ntoa(client_addr.sin_addr)) == NULL)    //obtain ip address of client
     {
        syslog(LOG_ERR,"client ip failed\n");
        return rc;
      
     }
     
     else{
     
     syslog(LOG_DEBUG,"Accepted connection from %s\n", client_ip);
     
     }
     
     buf_loc = 0;
     
     //receive packets from client socket
     
	do 
	{
	    bytes_read = recv(client_fd, read_buff + buf_loc, BUFFER_LEN, 0);
	    if(bytes_read == -1) 
	    {  
	       syslog(LOG_ERR,"recieve error\n");
               free_memory();
        	return rc;
    	    }
    	    
	    ++realloc_val;
	    
	    buf_loc += bytes_read;
	    
            byte_total +=bytes_read;
            
            read_buff = (char *)realloc(read_buff, realloc_val*BUFFER_LEN*(sizeof(char))); //reallocate read buffer to make more space 
            
	    if(read_buff == NULL)
	    {

            	syslog(LOG_ERR,"realloc error\n");
               free_memory();
               return rc;
            }

	} while(strchr(read_buff, '\n') == NULL);
	
	
	read_buff[buf_loc] = '\0';


      
        bytes_write = write(file_fd,read_buff, buf_loc); //write bytes to file 
        
        if(bytes_write == -1)
        {
            syslog(LOG_ERR,"write error\n");
            free_memory();
            return rc;
        }

	
	write_buff = (char *) realloc(write_buff, byte_total*(sizeof(char))); //reallocate write buffer to make more space
        
        if(write_buff == NULL)
        {
           syslog(LOG_ERR,"recieve error\n");
           free_memory();
           return rc;
         }

	
	lseek(file_fd, 0, SEEK_SET);
	
	read_bytes1 = read(file_fd,write_buff, byte_total); //read contents from file to write_buffer
	
	if(read_bytes1 == -1)
	{
	  syslog(LOG_ERR,"read error\n");
	  free_memory();
	  return rc;
	}
	

        bytes_sent = send(client_fd, write_buff,read_bytes1, 0);  //send packets to client socket
	
	if(bytes_sent == -1)
	{

            syslog(LOG_ERR,"send error\n");
            free_memory();
            return rc;
	}
	

	close(client_fd);  //close client file descriptor

	
    
    }
      

     closelog();
     rc =0;
     
     return rc; //exit with success       
         
  }
