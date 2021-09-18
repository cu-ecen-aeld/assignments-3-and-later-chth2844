#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

//filename:thread.c
//Date: 09/16/21
//Author: Chirayu Thakur

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    int ret,ret1;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param; //obtaining thread arguments through parameter
    
    if((thread_func_args->wait_to_obtain_ms)<0)
    {
      usleep(0);
    }
    
    else
    {
    
    usleep((thread_func_args->wait_to_obtain_ms)*1000);
    }
    
    ret = pthread_mutex_lock(thread_func_args->mutex); //lock mutex 
    
    if(ret!=0)
    { 
      thread_func_args->thread_complete_success = false; //lock mutex failed 
      return thread_param;
    }
    
    if((thread_func_args->wait_to_release_ms)<0)
    {
      usleep(0);
    }
    
    else
    {
    
    usleep((thread_func_args->wait_to_release_ms)*1000);
    }
    
     ret1 = pthread_mutex_unlock(thread_func_args->mutex); //unlock mutex 
     
     if(ret1!=0)
    { 
      thread_func_args->thread_complete_success = false;  //mutex unlock failed 
      return thread_param;
    }
    
    DEBUG_LOG("thread completed with success");
    thread_func_args->thread_complete_success = true;
    
    
    
    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
     int ret;
     struct thread_data *thread_param = (struct thread_data *) malloc(sizeof(struct thread_data));
     if(thread_param!=NULL)
     {
        thread_param->mutex = mutex;
        thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
        thread_param->wait_to_release_ms = wait_to_release_ms;
        ret = pthread_create(thread,NULL, threadfunc, (void *) thread_param);
        if(ret==0)
        { 
          DEBUG_LOG("Thread Creation successfull");
          return true; //thread creation successfull 
          
        }
      }
      
    ERROR_LOG("Thread creation failed");
    return false; //failure, thread creation failed 
}

