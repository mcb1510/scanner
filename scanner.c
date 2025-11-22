#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BSU CS 452 HW5");
MODULE_AUTHOR("Miguel Carrasco Belmar");

typedef struct {
  dev_t devno;
  struct cdev cdev;
  char default_separators[256];
  size_t default_sep_count;
} Device;			/* per-init() data */


typedef struct {
  char *data;        // data to scan
  size_t data_len;   // length of data
  size_t pos;        // current scanning position
  char *separators;  // separator characters
  size_t sep_count;  // number of separator characters
  int config_mode;   // configuration mode flag, 1= next write sets separators
  size_t token_start; // start index of the current token
  size_t token_end;   // end index of the current token
  size_t token_read_pos; // read position within the current token
} File;				/* per-open() data */

static Device device; 

// This function is called when the file is opened to allocate and initialize per-file data
static int open(struct inode *inode, struct file *filp) {
  File *file=(File *)kmalloc(sizeof(*file),GFP_KERNEL);
  if (!file) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    return -ENOMEM;
  }
  // Initialize data
  file->data=NULL;
  file->data_len=0;
  file->pos=0;

  // Copy default separators from device
  file->separators=kmalloc(device.default_sep_count, GFP_KERNEL);
  if (!file->separators) {
    printk(KERN_ERR "%s: kmalloc() failed\n",DEVNAME);
    kfree(file);
    return -ENOMEM;
  }
  memcpy(file->separators, device.default_separators, device.default_sep_count);
  file->sep_count=device.default_sep_count;
  file->config_mode=0;
  file->token_start=0;
  file->token_end=0;
  file->token_read_pos=0;
  filp->private_data=file;
  return 0;
}

// This function is called when the file is closed to free allocated resources
static int release(struct inode *inode, struct file *filp) {
  File *file=filp->private_data;
  if (file->data)
    kfree(file->data);
  if (file->separators)
    kfree(file->separators);
  kfree(file);
  return 0;
}

// This function handles both writing separators and writing data to be scanned
static ssize_t write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
  File *file = filp->private_data;

  // Write set separators = MODE 1
  if (file-> config_mode == 1) {
    // if there is existing separators, free them
    if (file->separators)
      kfree(file->separators);

    // allocate new separators
    file->separators = kmalloc(count, GFP_KERNEL);
    if (!file->separators)
      return -ENOMEM;
    // copy new separators from user space
    if (copy_from_user(file->separators, buf, count))
      return -EFAULT;

    file->sep_count = count;
    file->config_mode = 0; // reset config mode after setting separators
    return count; 
  }

  // Write data to be scanned = MODE 0
  // free old data if exists
  if (file->data)
    kfree(file->data);
  // allocate new data buffer
  file->data = kmalloc(count, GFP_KERNEL);
  if (!file->data)
    return -ENOMEM;
  // copy data from user space
  if (copy_from_user(file->data, buf, count))
    return -EFAULT;
  file->data_len = count;

  // reset scanning position
  file->pos = 0;
  file->token_start = 0;
  file->token_end = 0;
  file->token_read_pos = 0;
  return count;
}


// This function reads tokens from the scanned data
static ssize_t read(struct file *filp,char *buf,size_t count,loff_t *f_pos) { 
  return -EINVAL; // not implemented
}

// This function handles ioctl calls to set configuration mode
static long ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
   File *file=filp->private_data;
   // only request 0 is supported
   if (cmd==0) { // set configuration mode
     file->config_mode=1; // next write sets separators
     if (file->separators) { // free old separators
       kfree(file->separators);
       file->separators=NULL; // reset separator pointer
     }
     file->sep_count=0; // reset separator count
     return 0;
   }
    return -EINVAL; // invalid command
}

// File operations structure
static struct file_operations ops={
  .open=open,
  .release=release,
  .read=read,
  .write=write,
  .unlocked_ioctl=ioctl,
  .owner=THIS_MODULE
};

// Module initialization function
static int __init my_init(void) {
  int err;

  // set up default separators: space, tab, newline, colon
  device.default_separators[0]=' ';
  device.default_separators[1]='\t';
  device.default_separators[2]='\n';
  device.default_separators[3]=':';
  device.default_sep_count=4;
  
  // register device
  err=alloc_chrdev_region(&device.devno,0,1,DEVNAME);
  if (err<0) {
    printk(KERN_ERR "%s: alloc_chrdev_region() failed\n",DEVNAME);
    return err;
  }
  cdev_init(&device.cdev,&ops);
  device.cdev.owner=THIS_MODULE;
  err=cdev_add(&device.cdev,device.devno,1);
  if (err) {
    printk(KERN_ERR "%s: cdev_add() failed\n",DEVNAME);
    unregister_chrdev_region(device.devno,1);
    return err;
  }
  printk(KERN_INFO "%s: init\n",DEVNAME);
  return 0;
}

// Termination function
static void __exit my_exit(void) {
  cdev_del(&device.cdev);
  unregister_chrdev_region(device.devno,1);
  printk(KERN_INFO "%s: exit\n",DEVNAME);
}

// Module entry and exit points
module_init(my_init);
module_exit(my_exit);
