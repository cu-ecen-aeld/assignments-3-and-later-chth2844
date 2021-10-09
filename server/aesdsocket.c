//Filename: aesdsocket.c
//Description: server socket implementation
//Date: 10/03/2021
//Author: Chirayu Thakur
//Reference: https://github.com/cu-ecen-aeld/aesd-lectures/blob/master/lecture9/timer_thread.c

#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <getopt.h>
#include <stdbool.h>
#include <pthread.h>
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
#include <stdbool.h>
#include<time.h>
#include "queue.h"

//#defines
#define PORT_NO "9000"
#define SOCKET_PROTOCOL 0
#define LISTEN_BACKLOG 5
#define BUFFER_LEN 1000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define FILE_PERMISSIONS 0644

//global variables
int socket_fd,client_fd,file_fd;
int pid;




//thread structure
struct thread_data{

  
  pthread_t thr;
  char* read_buff;
  char* write_buff;
  int client_fd;
  bool thread_complete;
  



};

//linked list structure
typedef struct slist_data_s slist_data_t;
struct slist_data_s {
    struct thread_data thread_param;
    SLIST_ENTRY(slist_data_s) entries;
};

slist_data_t *datap=NULL;
SLIST_HEAD(slisthead, slist_data_s) head;
//SLIST_INIT(&head);

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

static void timer_thread ()
{
   char buf[100];
   
   
   time_t rtime;
   
   int write_bytes;
   
   sigset_t mask;
   
   struct tm *broken_down_time;
   
   time(&rtime);
   broken_down_time = localtime(&rtime);
   
   size_t size;
   
   size = strftime(buf,100,"timestamp:%a, %d %b %Y %T %z\n",broken_down_time);
   
   pthread_mutex_lock(&file_mutex);
   
   // Block signals to avoid partial write
    if (sigprocmask(SIG_BLOCK,&mask,NULL) == -1)
    {
        syslog(LOG_ERR,"signal block failed\n");
        exit(-1);
    }
   
   
   write_bytes = write(file_fd,buf,size);
    
   if (write_bytes == -1){
        
        printf("error");
    }
    
    
    // Unblock signals to avoid partial write
    if (sigprocmask(SIG_UNBLOCK,&mask,NULL) == -1)
    {
        syslog(LOG_ERR,"signal unblock failed\n");
        exit(-1);
    }

    

   pthread_mutex_unlock(&file_mutex);
    
   

   
   
}

//frees memory ,closes files and socket
void free_memory()
{
   //close server socket
   if(close(socket_fd)<0)
    {
      syslog(LOG_ERR,"Cannot close server socket file descriptor\n");
    }
    
  //remove file 
  if(remove(FILE_PATH)<0)
    {
      syslog(LOG_ERR,"Cannot remove file\n");
    }
    
 
  
   // Cancel threads, free assicociated pointers
    SLIST_FOREACH(datap,&head,entries){

        if (datap->thread_param.thread_complete != true){

            
            free(datap->thread_param.write_buff);
            free(datap->thread_param.read_buff);
            close(datap->thread_param.client_fd);
            pthread_cancel(datap->thread_param.thr);
            
            
            
        }


    }
    
    
     closelog();

  
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
    
    
    
    if(remove(FILE_PATH)<0)
    {
      syslog(LOG_ERR,"Cannot remove file\n");
    }
    
     // Cancel threads, free assicociated pointers
    SLIST_FOREACH(datap,&head,entries){

        if (datap->thread_param.thread_complete != true){

            free(datap->thread_param.write_buff);
            free(datap->thread_param.read_buff);
            close(datap->thread_param.client_fd);
            pthread_cancel(datap->thread_param.thr);
            
            
            
            
        }
    
    closelog();
    
    
    exit(0);
    
    }
    
  }

}

void packet_transfer(void *threadp)
{

  
 
  
  struct thread_data *threadsock = (struct thread_data*) threadp;
  
  sigset_t mask;
  
  
  
  
  int bytes_read,bytes_write,bytes_sent,read_bytes1;
  
  int buf_loc;
  
 
  static int byte_total=0;
  int realloc_val = 1;
  
  threadsock->read_buff = (char *) malloc(sizeof(char) * BUFFER_LEN);
  threadsock->write_buff = (char *) malloc(sizeof(char) * BUFFER_LEN);
  
  
  
  buf_loc = 0;
     
     //receive packets from client socket
     
	do 
	{
	    bytes_read = recv(threadsock->client_fd, threadsock->read_buff + buf_loc, BUFFER_LEN, 0);
	    if(bytes_read == -1) 
	    {  
	       syslog(LOG_ERR,"recieve error\n");
               free_memory();
        	
    	    }
    	    
	    ++realloc_val;
	    
	    buf_loc += bytes_read;
	    
            byte_total +=bytes_read;
            
            threadsock->read_buff = (char *)realloc(threadsock->read_buff, realloc_val*BUFFER_LEN*(sizeof(char))); //reallocate read buffer to make more space 
            
	    if(threadsock->read_buff == NULL)
	    {

            	syslog(LOG_ERR,"realloc error\n");
               free_memory();
               
            }

	} while(strchr(threadsock->read_buff, '\n') == NULL);
	
	
	threadsock->read_buff[buf_loc] = '\0';

        pthread_mutex_lock(&file_mutex);
        
        // Block signals to avoid partial write
        if (sigprocmask(SIG_BLOCK,&mask,NULL) == -1)
        {
        syslog(LOG_ERR,"signal block error\n");
        exit(-1);
        }
      
        bytes_write = write(file_fd,threadsock->read_buff, buf_loc); //write bytes to file 
        
        if(bytes_write == -1)
        {
            syslog(LOG_ERR,"write error\n");
            free_memory();
           
        }
        
        
         // Unblock signals to avoid partial write
    if (sigprocmask(SIG_UNBLOCK,&mask,NULL) == -1)
    {
        syslog(LOG_ERR,"signal unblock failed\n");
        exit(-1);
    }

	
	threadsock->write_buff = (char *) realloc(threadsock->write_buff, byte_total*(sizeof(char))); //reallocate write buffer to make more space
        
        if(threadsock->write_buff == NULL)
        {
           syslog(LOG_ERR,"recieve error\n");
           free_memory();
           
         }

	
	lseek(file_fd, 0, SEEK_SET);
	
	read_bytes1 = read(file_fd,threadsock->write_buff, byte_total); //read contents from file to write_buffer
	
	if(read_bytes1 == -1)
	{
	  syslog(LOG_ERR,"read error\n");
	  free_memory();
	}
	
	pthread_mutex_unlock(&file_mutex);

        bytes_sent = send(threadsock->client_fd, threadsock->write_buff,read_bytes1, 0);  //send packets to client socket
	
	if(bytes_sent == -1)
	{

            syslog(LOG_ERR,"send error\n");
            free_memory();
	}
	

	close(threadsock->client_fd);  //close client file descriptor
	threadsock->thread_complete= true;
	free(threadsock->read_buff);
	free(threadsock->write_buff);
	
  
  
  
}
int main(int argc, char *argv[])
{  

   int rc=-1;
   int status;
   int isdaemon=0;
   int reuse_addr =1;
   struct addrinfo hints;
   struct addrinfo *res; //points to result of getaddrinfo
   struct sockaddr_in client_addr;
   socklen_t client_addr_size;
   client_addr_size = sizeof(struct sockaddr_in);
   char *client_ip;
   timer_t timerid;
   struct itimerspec result;
   int ret;
   
   openlog("aesdsock",LOG_PID,LOG_USER);  //open log
   
   result.it_interval.tv_sec = 10;
   result.it_interval.tv_nsec = 0;
   result.it_value.tv_sec = 10;
   result.it_value.tv_nsec = 0;
   
   
   struct sigevent sev;
   
   int clock_id = CLOCK_MONOTONIC;
   
   memset(&sev,0,sizeof(struct sigevent));
   
  
   
   //setup call to timer_thread
   
   sev.sigev_notify = SIGEV_THREAD;
   sev.sigev_notify_function = timer_thread;
   
   if(timer_create(clock_id,&sev,&timerid) !=0)
   {
      syslog(LOG_ERR,"Cannot create timer!\n");
      printf("Cannot create timer!");
      return rc;
   }
   
   
   
   
   
   ret = timer_settime(timerid,0, &result, NULL);
   
   if(ret)
   {
     syslog(LOG_ERR,"Error in setting timer!\n");
     printf("error in setting timer!");
     return rc;
   
   }
   
   memset(&hints,0,sizeof(hints)); //make sure struct is empty
   hints.ai_family = AF_INET; // don't care IPV4, IPV6
   hints.ai_socktype = SOCK_STREAM; //TCP stream sockets
   hints.ai_flags = AI_PASSIVE; //fill in IP for me 
   
   
   
   
   
   
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
    
    
    
    // Setup signal mask
    sigset_t set;
    sigemptyset(&set);           // empty the set

    // add the signals to the set
    sigaddset(&set,SIGINT);      
    sigaddset(&set,SIGTERM);     
   
   
   //listen to client connections 
   if((status = listen(socket_fd,LISTEN_BACKLOG))<0)
   {
      syslog(LOG_ERR,"listen error\n");
      return rc;
   }
    
    
    
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
     
     
    datap = malloc(sizeof(slist_data_t));
    SLIST_INSERT_HEAD(&head,datap,entries);
    datap->thread_param.client_fd = client_fd;
    pthread_create(&(datap->thread_param.thr),(void*)0,(void*)&packet_transfer,(void*)&(datap->thread_param));

    SLIST_FOREACH(datap,&head,entries){

        if(datap->thread_param.thread_complete == false){

            continue;

        }

        else if (datap->thread_param.thread_complete == true){

            pthread_join(datap->thread_param.thr,NULL);

        }

    }
     
    

	
    
    }
      

     closelog();
     rc =0;
     
     return rc; //exit with success       
         
  }
