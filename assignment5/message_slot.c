//
// Created by ori on 27/06/17.
//

#include "message_slot.h"
#include "hashmap.h"

/* Declare what kind of code we want from the header files
   Defining __KERNEL__ and MODULE allows us to access kernel-level
   code not usually available to userspace programs. */
#undef __KERNEL__
#define __KERNEL__ /* We're part of the kernel */
#undef MODULE
#define MODULE     /* Not a permanent part, though. */

/* ***** Example w/ minimal error handling - for ease of reading ***** */

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include<linux/slab.h>
//#include <stdio.h>

MODULE_LICENSE("GPL");

struct chardev_info{
    spinlock_t lock;
};

static struct chardev_info device_info;

static int major; /* device major number */

/***************** char device functions *********************/

/* process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
    unsigned long flags; // for spinlock
    printk("device_open(%p)\n", file);

    /*
     * We don't want to talk to two processes at the same time
     */
    spin_lock_irqsave(&device_info.lock, flags);
    char *uniqueId;
    sprintf(uniqueId, "%d", file->f_inode->i_ino);
    if (lookup(uniqueId)){
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -EBUSY;
    }
    messageInfo *messageInfo2;
    messageInfo2 = (messageInfo *) kmalloc(sizeof(messageInfo), GFP_KERNEL);
    install(uniqueId, messageInfo2);
    kfree((void *)messageInfo2);
    spin_unlock_irqrestore(&device_info.lock, flags);

    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    unsigned long flags; // for spinlock
    printk("device_release(%p,%p)\n", inode, file);

    /* ready for our next caller */
    spin_lock_irqsave(&device_info.lock, flags);
    char *uniqueId;
    sprintf(uniqueId, "%d", file->f_inode->i_ino);
    if (lookup(uniqueId)) {
        pop(uniqueId);
    }
    else {
        printk("Couldn't release device(%p,%p), wasn't opened yet\n", inode, file);
    }
    spin_unlock_irqrestore(&device_info.lock, flags);

    return SUCCESS;
}

/* a process which has already opened
   the device file attempts to read from it */
static ssize_t device_read(struct file *file, /* see include/linux/fs.h   */
                           char __user *buffer, /* buffer to be filled with data */
                           size_t length,  /* length of the buffer     */
                           loff_t *offset) {
    /* read doesnt really do anything (for now) */
    struct nlist *np;
    char *uniqueId;
    sprintf(uniqueId, "%d", file->f_inode->i_ino);
    if (!(np = lookup(uniqueId))) {
        printk("Couldn't find device(%p), wasn't opened yet\n", file);
        return 1;
    }
    if (np->currentChannelIndex == 0)
        printk("device_read(%p,%d) - operation not supported yet (last written - %s)\n", file, length, np->channelBuff1);
    if (np->currentChannelIndex == 1)
        printk("device_read(%p,%d) - operation not supported yet (last written - %s)\n", file, length, np->channelBuff2);
    if (np->currentChannelIndex == 2)
        printk("device_read(%p,%d) - operation not supported yet (last written - %s)\n", file, length, np->channelBuff3);
    if (np->currentChannelIndex == 3)
        printk("device_read(%p,%d) - operation not supported yet (last written - %s)\n", file, length, np->channelBuff4);


    return -EINVAL; // invalid argument error
}

/* somebody tries to write into our device file */
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t * offset) {
    int i;

    printk("device_write(%p,%d)\n", file, length);
    struct nlist *np;
    char *uniqueId;
    sprintf(uniqueId, "%d", file->f_inode->i_ino);
    if (!(np = lookup(uniqueId))) {
        printk("Couldn't find device(%p), wasn't opened yet\n", file);
        return 1;
    }

    // TODO: adding zeros to the end of the buffer
    for (i = 0; i < BUF_LEN; i++) {
        if (np->defn->currentChannelIndex == 0)
            get_user(np->channelBuff1[i], buffer + i);
        if (np->defn->currentChannelIndex == 1)
            get_user(np->channelBuff2[i], buffer + i);
        if (np->defn->currentChannelIndex == 2)
            get_user(np->channelBuff3[i], buffer + i);
        if (np->defn->currentChannelIndex == 3)
            get_user(np->channelBuff4[i], buffer + i);
    }

    /* return the number of input characters used */
    return i;
}

static long device_ioctl(struct inode*  inode,
                         struct file*   file,
                         unsigned int   ioctl_num,/* The number of the ioctl */
                         unsigned long  ioctl_param) { /* The parameter to it */
    if (ioctl_param < 0 || ioctl_param > 3) return 1;

    struct nlist *np;
    char *uniqueId;
    sprintf(uniqueId, "%d", file->f_inode->i_ino);
    if (!(np = lookup(uniqueId))) {
        printk("Couldn't find device(%p,%p), wasn't opened yet\n", inode, file);
        return 1;
    }
    np->defn->currentChannelIndex = ioctl_param;
    return SUCCESS;
}

/************** Module Declarations *****************/

/* This structure will hold the functions to be called
 * when a process does something to the device we created */
struct file_operations Fops = {
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .release = device_release,    /* a.k.a. close */
        .ioctl = device_ioctl
};

/* Called when module is loaded.
 * Initialize the module - Register the character device */
static int __init simple_init(void) {
    /* init dev struct*/
    memset(&device_info, 0, sizeof(struct chardev_info));
    spin_lock_init(&device_info.lock);

    /* Register a character device. Get newly assigned major num */
    major = register_chrdev(0, DEVICE_RANGE_NAME, &Fops /* our own file operations struct */);

    /*
     * Negative values signify an error
     */
    if (major < 0) {
        printk(KERN_ALERT "%s failed with %d\n",
                "Sorry, registering the character device ", major);
        return major;
    }

    printk("Registeration is a success. The major device number is %d.\n", major);
    printk("If you want to talk to the device driver,\n");
    printk("you have to create a device file:\n");
    printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, major);
    printk("You can echo/cat to/from the device file.\n");
    printk("Dont forget to rm the device file and rmmod when you're done\n");

    return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
static void __exit simple_cleanup(void) {
    /*
     * Unregister the device
     * should always succeed (didnt used to in older kernel versions)
     */
    unregister_chrdev(major, DEVICE_RANGE_NAME);
}

module_init(simple_init);
module_exit(simple_cleanup);