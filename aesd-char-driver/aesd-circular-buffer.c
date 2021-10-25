/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/slab.h> //kfree
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
      */
      
    size_t read_pos;
    size_t total_size;
    size_t prev_size;
   
   if(buffer == NULL || entry_offset_byte_rtn == NULL)  //check for NULL pointer 
   {
        return NULL;
   }
    
   if((buffer->full==false) && (buffer->in_offs==buffer->out_offs))  //check if circular buffer is empty 
    {
      return NULL;
    }
    
   
   
   
   read_pos = buffer->out_offs;
   total_size = 0;
  
    
    
    do {     
          
          prev_size = total_size;
          total_size += buffer->entry[read_pos].size;
          
          
          if( char_offset < total_size)
          {
            *entry_offset_byte_rtn = char_offset - prev_size; //position of character wrt buffer entry
             
             return &buffer->entry[read_pos];	 //return buffer entry corresponding to offset character
          
          }
          
          read_pos++;
          
          if(read_pos == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) //reached last element of circular buffer
          {
             read_pos = 0;  //read first element of circular buffer
          }    
    
    }while(read_pos != buffer->in_offs); //loop until entire buffer is searched 
    
     return NULL; 

}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char *aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */
    
    const char *release_buffptr;
    
    release_buffptr = NULL;
    
   // if(buffer == NULL || add_entry->buffptr == NULL || add_entry->size == 0) //check for null pointer and size of 0 
   // {
    //   return NULL;
   // }
    
    
    if(buffer->full==true)
   {
     release_buffptr=buffer->entry[buffer->in_offs].buffptr;
   }
    
    buffer->entry[buffer->in_offs] = *add_entry;  
    
    buffer->in_offs++;
   
    if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) //reached last element of circular buffer 
    {
      buffer->in_offs = 0;  //write to first element of circular buffer next 
    }
    
    if(buffer->full == true)
    {
       buffer->out_offs ++;
       
       if(buffer->out_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) //reached last element of circular buffer 
       {
         buffer->out_offs = 0; //read first element of circular buffer to make space for new entry 
       }
    }
    
    if(buffer->in_offs == buffer->out_offs)
    {
      buffer->full = true; //buffer is full 
    }
    
    else 
    {
    
        buffer->full = false;
    }
    
    return release_buffptr; 
   
    
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

/**
* releases memory for each element in circular @param buffer 
*/
void aesd_circular_buffer_release(struct aesd_circular_buffer *buffer)
{
	
	struct aesd_buffer_entry *element;
	uint8_t index;


	AESD_CIRCULAR_BUFFER_FOREACH(element,buffer,index) 
	{

		if (element->buffptr != NULL)
		{

#ifndef __KERNEL__
		
		free(element->buffptr);	
#else
		kfree(element->buffptr);
		}
#endif

	}
	
    buffer->in_offs =0 ;
    buffer->out_offs=0;
}
