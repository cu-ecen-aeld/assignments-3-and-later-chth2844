//Filename: aesdsocket.c
//Description: server socket implementation with pthreads 
//Date: 10/10/2021
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
#include <time.h>
#include "queue.h"

#define USE_AESD_CHAR_DEVICE 1

//#defines
#define PORT_NO "9000"
#define SOCKET_PROTOCOL 0
#define LISTEN_BACKLOG 5
#define BUFFER_LEN 1000
#ifdef USE_AESD_CHAR_DEVICE
       #define  FILE_PATH "/dev/aesdchar"
#else
       
       #define FILE_PATH "/var/tmp/aesdsocketdata"
       
#endif

#define FILE_PERMISSIONS 0666
#define TIMER_INTERVAL_S 10
#define TIMER_INTERVAL_NS 10

//global variables
int file_fd=0;
int socket_fd=0,client_fd=0;
int terminate=0;
//timer_t timer_id;


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

//initialize mutex
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;


//frees memory ,closes files and frees linked list 

void free_memory()
{
  
  
  //close server socket
  if(close(socket_fd)<0)
  {
      syslog(LOG_ERR,"Cannot close server socket file descriptor\n");
  }
    
  #ifndef USE_AESD_CHAR_DEVICE
  
    //remove file 
    if(remove(FILE_PATH)<0)
    {
      syslog(LOG_ERR,"Cannot remove file\n");
    }
    
  #endif
    
  // Cancel threads, free assicociated pointers
  SLIST_FOREACH(datap,&head,entries)
  {

        if (datap->thread_param.thread_complete != true)
        {

            pthread_cancel(datap->thread_param.thr);
            free(datap->thread_param.write_buff);
            free(datap->thread_param.read_buff);
            close(datap->thread_param.client_fd);
           
            
            
            
        }


   }
    

    
  // free Linked list
  while(!SLIST_EMPTY(&head))
  {
       datap = SLIST_FIRST(&head);
       SLIST_REMOVE_HEAD(&head,entries);
       free(datap);
  }

    
  terminate=1;
    
  //close log
  closelog();

  
}

//signal handler for SIGINT and SIGTERM
void sig_handler(int signo)
{ 

  terminate = 1;
  
  if(signo == SIGINT || signo == SIGTERM)
  { 
    
    syslog(LOG_DEBUG, "Caught signal, exiting\n");
    

    if(close(socket_fd)<0)
    {
      syslog(LOG_ERR,"Cannot close server socket file descriptor\n");
    }
    
  }

}

//packet transfer function 
void packet_transfer(void *threadp)
{

    struct thread_data *threadsock = (struct thread_data*) threadp;
    
    sigset_t mask;
    
    //clear mask to default value
    memset(&mask,0,sizeof(sigset_t));
    
    char *buf;

    int buff_loc=0;

    ssize_t read_bytes;

    int curr_size=BUFFER_LEN ;
  
    int num_bytes,write_bytes;
    
    char single_byte;
    
    int read_pos = 0;
    
    int outbuf_size = BUFFER_LEN;
    
    int read_offset=0;
    
    file_fd = open(FILE_PATH,O_RDWR | O_TRUNC | O_CREAT,FILE_PERMISSIONS); //open file 
   
     if(file_fd==-1)
     {
      syslog(LOG_ERR,"Cannot open file\n");
      exit(-1);
     
     }
    
    //dynamically allocate memory to read and write buffer
    threadsock->read_buff = (char*)malloc(sizeof(char)*BUFFER_LEN);
    threadsock->write_buff = (char*)malloc(sizeof(char)*BUFFER_LEN);

    //receive packets from client 
    while((num_bytes = recv(threadsock->client_fd,threadsock->read_buff+buff_loc, BUFFER_LEN, 0))>0)
    {
        
        
        if(num_bytes == -1)
        {
          
          syslog(LOG_ERR,"recieve error\n");
          free_memory();
          exit(-1);
        
        }

        buff_loc += num_bytes;
        
        // dynamicall increase buffer size for incoming packets
        if (buff_loc >= curr_size){

        curr_size+=BUFFER_LEN;

        threadsock->read_buff = realloc(threadsock->read_buff,sizeof(char)*curr_size);

        }
        // Search for NULL character
        buf = strchr(threadsock->read_buff,'\n');

        if(buf != NULL) break;
        
      }
   


    // mutex lock
    pthread_mutex_lock(&file_mutex);

    // Block signals to avoid partial write to file 
    if (sigprocmask(SIG_BLOCK,&mask,NULL) == -1)
    {
        syslog(LOG_ERR,"signal block failed\n");
        free_memory();
        exit(-1);
    }

    // file write 
    write_bytes = write(file_fd,threadsock->read_buff,buff_loc);
    if (write_bytes == -1)
    {
        syslog(LOG_ERR,"write failed\n");
        free_memory();
        exit(-1);
    }

    // Unblock signals after write
    if (sigprocmask(SIG_UNBLOCK,&mask,NULL) == -1)
    {
        syslog(LOG_ERR,"signal unblock failed\n");
        free_memory();
        exit(-1);
    }

    //mutex unlock
    pthread_mutex_unlock(&file_mutex);
  
    
    
    
    //point to begining of file 
    lseek(file_fd,0,SEEK_SET);
    
   

    // Block signals to avoid partial read from file 
    if (sigprocmask(SIG_BLOCK,&mask,NULL) == -1)
    {
        syslog(LOG_ERR,"signal block failed\n");
        free_memory();
        exit(-1);
    }

    // mutex lock
    pthread_mutex_lock(&file_mutex);

    
    // read from file 
    while((read_bytes = read(file_fd,&single_byte,1)) > 0)
    {

        if(read_bytes <0 ) 
        {

             syslog(LOG_ERR,"read failed\n");
             free_memory();
             exit(-1);

        }

       threadsock->write_buff[read_pos] = single_byte;

        if(threadsock->write_buff[read_pos] == '\n')
        {

            
            int packet_size = read_pos - read_offset + 1;
            
            // Send packets to client 
            if (send(threadsock->client_fd,threadsock->write_buff+read_offset,packet_size, 0) == -1)
            { 
                syslog(LOG_ERR,"read failed\n");
                free_memory();
                exit(-1);
            }

            read_offset = read_pos + 1;

        }

        read_pos++;
        
        //reallocate buffer if more memory is required 
        if(read_pos >= outbuf_size)
        {
            
            outbuf_size += BUFFER_LEN;
            threadsock->write_buff=realloc(threadsock->write_buff,sizeof(char)*outbuf_size);

        }


    }

    // Unblock signals after read and send
    if (sigprocmask(SIG_UNBLOCK,&mask,NULL) == -1)
    {
        syslog(LOG_ERR,"signal unblock failed\n");
        free_memory();
        exit(-1);
    }

    //unlock mutex
    pthread_mutex_unlock(&file_mutex);

    //close client file descriptor 
    close(threadsock->client_fd);

    //thread complete
    threadsock->thread_complete = true;
    
    //free dynamically allocated memory 
    free(threadsock->read_buff);
    free(threadsock->write_buff);
}
	
  
  
 
int main(int argc, char *argv[])
{  

   int rc=-1;
   int status=-1;
   int isdaemon=0;
   int reuse_addr =1;
   struct addrinfo hints;
   struct addrinfo *res; //points to result of getaddrinfo
   struct sockaddr_in client_addr;
   socklen_t client_addr_size;
   client_addr_size = sizeof(struct sockaddr_in);
   char *client_ip;
   //timer_t timer_id;
   //truct itimerspec result;
   //int ret=0;
   int pid=0;
   
   SLIST_INIT(&head); //initialize linked list 
   
   
   openlog("aesdsocket.c",LOG_PID,LOG_USER);  //open log
   
   
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
    
    
   //setting up call to timer_thread function 
     
   
   
   
   //struct sigevent sev;
   
   //int clock_id = CLOCK_MONOTONIC; //use relative time 
   
  // sev.sigev_notify = SIGEV_THREAD;
   //sev.sigev_value.sival_ptr= &timer_id;
   //sev.sigev_notify_function = timer_thread;
  // sev.sigev_notify_attributes = NULL;
   
   //ret = timer_create(clock_id,&sev,&timer_id); //create timer
   
   
    
   while(terminate!=1)
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
     
    //add new element to linked list 
    datap = (slist_data_t*) malloc(sizeof(slist_data_t));
    SLIST_INSERT_HEAD(&head,datap,entries);
    
    //insert thread data 
    datap->thread_param.client_fd = client_fd;
    datap->thread_param.thread_complete = false; 
    
    //create thread
    pthread_create(&(datap->thread_param.thr),(void*)0,(void*)&packet_transfer,(void*)&(datap->thread_param)); 
    
    //iterate through each entry in linked list 
    SLIST_FOREACH(datap,&head,entries)
    {

        if(datap->thread_param.thread_complete == false)
        {

            continue; //thread not yet complete so continue 

        }

        else if (datap->thread_param.thread_complete == true)
        {

            pthread_join(datap->thread_param.thr,NULL); //wait for thread to terminate after thread completes 

        }

    }
     
    

	
    
    }
      

     closelog(); //close log 
     rc =0;
     
     return rc; //exit with success       
         
  }
