/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesd-circular-buffer.h" // circular buffer operations
#include <linux/mutex.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Chirayu Thakur"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev;

	PDEBUG("open");
	/**
	 * TODO: handle open
	 */
	 
	
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
	
	filp->private_data = dev;
	
	return 0;
	
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	size_t read_offset;
	ssize_t bytes_read;
	ssize_t retval = 0;
	struct aesd_dev *dev = NULL;
	struct aesd_buffer_entry *read_entry = NULL;
	
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle read
	 */
	
	
	dev=filp->private_data;
	
	retval = mutex_lock_interruptible(&dev->lock);
	
	if(retval < 0)
	{
	  retval = -ERESTARTSYS;
	  return retval;
	}
	
	read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buff,*f_pos,&read_offset);
	
	if(read_entry == NULL)
	{
	  
	  mutex_unlock(&dev->lock);
	  return 0;
	}
	
	bytes_read = read_entry->size - read_offset;
	
	if(count < bytes_read)
	{
	   bytes_read = count;
	}
	
	retval = copy_to_user(buf, (read_entry->buffptr + read_offset),bytes_read);
	
	if (retval != 0)
	{
		retval = -EFAULT;
		return retval;
	}

	*f_pos += bytes_read;

	
	mutex_unlock(&dev->lock); //release mutex
	
	
	 return bytes_read;
	
	
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
        
	
	
	const char* buffptr_release = NULL;
	struct aesd_dev *dev = NULL;
	char* line_break = NULL;
	ssize_t retval;
	ssize_t bytes_not_copied;
	dev=filp->private_data;

	/**
	 * TODO: handle write
	 */
	retval =mutex_lock_interruptible(&dev->lock);
	
	if(retval < 0)
	{
	  return retval;
	}
	

	if (dev->entry.size == 0) 
	{
		dev->entry.buffptr = kzalloc(count*sizeof(char), GFP_KERNEL); //allocate memory and set to zero 
		
		if (dev->entry.buffptr == 0)  //null pointer 
		{
		
			mutex_unlock(&dev->lock);
			retval = -ENOMEM; 
	               return retval;
		}
	}
	else 
	{
	       dev->entry.buffptr = krealloc(dev->entry.buffptr, dev->entry.size + count, GFP_KERNEL); //reallocate memory 
		
		if (dev->entry.buffptr == 0) 
		{
			mutex_unlock(&dev->lock);
	               return retval;
		}
	 }

	
       // copy buffer from kernel space to user buffer 
       bytes_not_copied=copy_from_user((void *)(&dev->entry.buffptr[dev->entry.size]), buf, count);
	
	retval = count - bytes_not_copied;
	
	dev->entry.size += retval;

	line_break = (char *)memchr(dev->entry.buffptr, '\n', dev->entry.size);
	
	if (line_break!=NULL) 
	{
		
	   buffptr_release = aesd_circular_buffer_add_entry(&dev->buff, &dev->entry);
	   
	   if (buffptr_release!=NULL) 
           {
		kfree(buffptr_release);
	   }
	   
	  dev->entry.size = 0;
          dev->entry.buffptr = 0;
	}

  
     *f_pos = 0;
     
     //release mutex 
     mutex_unlock(&dev->lock);
     return retval;
       
	 
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */
	 
	 
	 mutex_init(&aesd_device.lock); //initialize mutex;
	 
	 aesd_circular_buffer_init(&aesd_device.buff); //initialize circular buffer

	result = aesd_setup_cdev(&aesd_device);
       
	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	 
	 aesd_circular_buffer_release(&aesd_device.buff);
	 
	 

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
