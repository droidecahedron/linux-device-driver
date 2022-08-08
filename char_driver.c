#include <linux/module.h>	/* for modules */
#include <linux/fs.h>		/* file_operations */
#include <linux/uaccess.h>	/* copy_(to,from)_user */
#include <linux/init.h>		/* module_init, module_exit */
#include <linux/slab.h>		/* kmalloc */
#include <linux/cdev.h>		/* cdev utilities */
#include <linux/semaphore.h>

#define MYDEV_NAME "mycdrv"

static char *ramdisk;
#define ramdisk_size (size_t) (16 * PAGE_SIZE) // ramdisk size 

//NUM_DEVICES defaults to 3 unless specified during insmod
static int NUM_DEVICES = 3;

#define CDRV_IOC_MAGIC 'Z'
#define ASP_CLEAR_BUF _IOW(CDRV_IOC_MAGIC, 1, int)

#define SCULL_IOCRESET    _IO(CDRV_IOC_MAGIC, 0)



/*
 * The other entities only have "Tell" and "Query", because they're
 * not printed in the book, and there's no need to have all six.
 * (The previous stuff was only there to show different ways to do it.
 */
#define SCULL_P_IOCTSIZE _IO(CDRV_IOC_MAGIC,   13)
#define SCULL_P_IOCQSIZE _IO(CDRV_IOC_MAGIC,   14)
/* ... more to come */

#define SCULL_IOC_MAXNR 14







static dev_t first;
static unsigned int count = 1;
//static int my_major = 700, my_minor = 0;
int scull_major = 0, scull_minor = 0;

struct ASP_mycdrv {
	struct cdev dev;
	char *ramdisk;
	struct semaphore sem;
	int devNo;
	// any other field you may want to add
	size_t dev_ramdisk_size;
};

static struct ASP_mycdrv *my_cdev_array;
static struct class *device_class;
dev_t devID = 0;


//prototypes
ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
loff_t  scull_llseek(struct file *filp, loff_t off, int whence);
long    scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int 	scull_open(struct inode *inode, struct file *filp);
int 	scull_release(struct inode *inode, struct file *filp);



//from the book
module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(NUM_DEVICES, int, S_IRUGO);




//this is from the book
static const struct file_operations scull_fops = {
	.owner =    THIS_MODULE,
	.llseek =   scull_llseek,
	.read =     scull_read,
	.write =    scull_write,
	.unlocked_ioctl = scull_ioctl,
	.open =     scull_open,
	.release =  scull_release,
};


loff_t scull_llseek(struct file *filp, loff_t off, int whence)
{
	struct ASP_mycdrv *dev = filp->private_data;
	loff_t newpos;

	switch(whence) {
	  case 0: /* SEEK_SET */
		newpos = off;
		break;

	  case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	  case 2: /* SEEK_END */
		newpos = dev->dev_ramdisk_size + off;
		break;

	  default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0) //expand buffer TODO
	{
		//kmalloc with (size_t)(size_old+_offset)
		//krealloc
		krealloc(dev->ramdisk,dev->dev_ramdisk_size+off, GFP_KERNEL);
	}
	filp->f_pos = newpos;
	return newpos;
}




/*
 * The ioctl() implementation
 */

long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int err = 0, tmp;
	int retval = 0;
	struct ASP_mycdrv *dev = filp->private_data;
    
    
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != CDRV_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;



	/*
	//this is a security check to ensure that userspace application
	// cant read from or write to kernel addresses. i think its ok to omit in the assignment
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;
	*/

	switch(cmd) {

	  case ASP_CLEAR_BUF: 
	  	memset(dev->ramdisk, 0, dev->dev_ramdisk_size);
	  	filp->f_pos = 0;
	  	break;
	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;

}


ssize_t scull_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct ASP_mycdrv *dev = filp->private_data; 
	ssize_t retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (*f_pos >= dev->dev_ramdisk_size)
		goto out;
	if (*f_pos + count > dev->dev_ramdisk_size)
		count = dev->dev_ramdisk_size - *f_pos;

	

	if (copy_to_user(buf,  dev->ramdisk + *f_pos, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

  out:
	up(&dev->sem);
	return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct ASP_mycdrv *dev = filp->private_data;
	ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	if (copy_from_user(dev->ramdisk, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

        /* update the size */
	if (dev->dev_ramdisk_size < *f_pos)
		dev->dev_ramdisk_size = *f_pos;

  out:
	up(&dev->sem);
	return retval;
}




/*
 * Open and close
 */

int scull_open(struct inode *inode, struct file *filp)
{

	struct ASP_mycdrv *dev; /* device information */

	dev = container_of(inode->i_cdev, struct ASP_mycdrv, dev);
	filp->private_data = dev; /* for other methods */


	return 0;          /* success */
}

int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}











//mostly from the book source code
int scull_init_module(void)
{
	int result, i;
	dev_t dev = 0;
	dev_t nth_devID;
	struct ASP_mycdrv* nth_device;

/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */
	if (scull_major) 
	{
		dev = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(devID, NUM_DEVICES, MYDEV_NAME);
	} 
	else 
	{
		result = alloc_chrdev_region(&devID, scull_minor, NUM_DEVICES, MYDEV_NAME);
		if(result != 0)
			goto fail;
		scull_major = MAJOR(devID);
	}
	
	if (result < 0) 
	{
		printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
		return result;
	}
	
	
	//create device class to avoid quantum (dynamic creation of device nodes)
	//from the lecture video
	device_class = class_create(THIS_MODULE, MYDEV_NAME);
	if(device_class == NULL)
	{
		pr_info("could not create class\n");
		goto fail;
	}
	
	my_cdev_array = kmalloc(NUM_DEVICES * sizeof(struct ASP_mycdrv), GFP_KERNEL);
	
	

        /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	for(i=0; i<NUM_DEVICES;i++)
	{
		nth_devID = MKDEV(scull_major, i);
		nth_device = &my_cdev_array[i];
		nth_device->dev_ramdisk_size = ramdisk_size;
		nth_device->devNo = nth_devID;
		cdev_add(&nth_device->dev, nth_devID, 1);
		nth_device->dev.owner = THIS_MODULE;
		cdev_init(&nth_device->dev, &scull_fops);
		//create separate ramdisk for each device
		nth_device->ramdisk = kmalloc(nth_device->dev_ramdisk_size,GFP_KERNEL);
		memset(nth_device->ramdisk,0,sizeof(char)*nth_device->dev_ramdisk_size);
		sema_init(&nth_device->sem,1);
		device_create(device_class,NULL,nth_devID,NULL,MYDEV_NAME "%d",i);
		pr_info("Device ASP_mycdrv %d is registered\n",i);
	}

	return 0; /* succeed */

  fail:
	return 666;
}


int scull_trim(struct ASP_mycdrv *dev)
{

	kfree(dev->ramdisk);
	dev->dev_ramdisk_size = 0;
	dev->devNo = 0;
	//professor says that its ok to omit semaphore releases since the scull api omits it
	//sema_destroy(dev->sem);  : release semaphore? sema_unlink(dev->sem);
	
	return 0;
}


void scull_cleanup_module(void)
{
	int i;
	dev_t devno = MKDEV(scull_major, scull_minor);

	/* Get rid of our char dev entries */
	if (my_cdev_array) {
		for (i = 0; i < NUM_DEVICES; i++) {
			scull_trim(&my_cdev_array[i]);
			cdev_del(&my_cdev_array[i].dev);
		}
		kfree(my_cdev_array);
	}
	
	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, NUM_DEVICES);


}




module_init(scull_init_module);
module_exit(scull_cleanup_module);

MODULE_AUTHOR("jnguyen");
MODULE_LICENSE("GPL v2");
