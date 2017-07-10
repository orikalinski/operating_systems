//
// Created by ori on 27/06/17.
//

#include "message_slot.h"

#include <linux/ioctl.h>
#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>


/* Declare what kind of code we want from the header files
   Defining __KERNEL__ and MODULE allows us to access kernel-level
   code not usually available to userspace programs. */
#undef __KERNEL__
#define __KERNEL__ /* We're part of the kernel */
#undef MODULE
#define MODULE     /* Not a permanent part, though. */
#define ID_LEN 50

/* ***** Example w/ minimal error handling - for ease of reading ***** */

MODULE_LICENSE("GPL");

struct chardev_info{
    spinlock_t lock;
};

static struct chardev_info device_info;

//https://stackoverflow.com/questions/4384359/quick-way-to-implement-dictionary-in-c
static struct nlist *hashtab[HASHSIZE];

messageInfo *messageSlotDup(messageInfo *messageInfo1) /* make a duplicate of messageInfo1 */
{
    int i = 0;
    int j = 0;
    messageInfo *messageInfo2;
    messageInfo2 = (messageInfo *) kmalloc(sizeof(messageInfo), GFP_KERNEL);
    if (messageInfo2 != NULL){
        for (; i < NUM_OF_BUFFERS; i++) {
            for (j = 0; j < BUFF_LEN; j++) {
                messageInfo2->channelBuffs[i][j] = messageInfo1->channelBuffs[i][j];
            }
        }
        messageInfo2->currentChannelIndex = messageInfo1->currentChannelIndex;
    }
    return messageInfo2;
}

char *strdup (const char *s) {
    char *d = kmalloc (strlen (s) + 1, GFP_KERNEL);   // Space for length plus nul
    if (d == NULL) return NULL;          // No memory
    strcpy (d, s);                        // Copy the characters
    return d;                            // Return the new string
}

/* hash: form hash value for string s */
unsigned hash(char *s)
{
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31 * hashval;
    return hashval % HASHSIZE;
}

/* lookup: look for s in hashtab */
struct nlist *lookup(char *s)
{
    struct nlist *np;
    for (np = hashtab[hash(s)]; np != NULL; np = np->next)
        if (strcmp(s, np->name) == 0)
            return np; /* found */
    return NULL; /* not found */
}

/* install: put (name, defn) in hashtab */
struct nlist *install(char *name, messageInfo *defn)
{
    struct nlist *np;
    unsigned hashval;
    if ((np = lookup(name)) == NULL) { /* not found */
        np = (struct nlist *) kmalloc(sizeof(*np), GFP_KERNEL);
        if (np == NULL || (np->name = strdup(name)) == NULL)
            return NULL;
        hashval = hash(name);
        np->next = hashtab[hashval];
        if (hashtab[hashval])
            hashtab[hashval]->previous = np;
        hashtab[hashval] = np;
    } else /* already there */
        kfree((void *) np->defn); /*kfree previous defn */
    if ((np->defn = messageSlotDup(defn)) == NULL)
        return NULL;
    return np;
}

void clearItem(struct nlist *np){
    unsigned hashval;
    if (np->previous)
        np->previous->next = np->next;
    if (np->next)
        np->next->previous = np->previous;
    if (!np->next && !np->previous) {
        hashval = hash(np->name);
        hashtab[hashval] = NULL;
    }

    kfree((void *) np->defn); /*kfree previous defn */
    kfree((void *) np->name); /*kfree name */
    kfree((void *) np); /*kfree np */
}

struct nlist *pop(char *name)
{
    struct nlist *np;
    if ((np = lookup(name)) == NULL) { /* not found */
        return NULL;
    } else {/* already there */
        clearItem(np);
        return np;
    }
}

void hashTableCleanUp(void) {
    int i = 0;
    struct nlist *np;
    struct nlist *tmp;

    for (; i < HASHSIZE; i++){
        np = hashtab[i];
        while (np != NULL){
            tmp = np->next;
            clearItem(np);
            np = tmp;
        }
    }
}


/***************** char device functions *********************/

/* process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
    char uniqueId[ID_LEN];
    messageInfo *messageInfo2;
    unsigned long flags; // for spinlock
    printk("device_open(%p)\n", file);

    /*
     * We don't want to talk to two processes at the same time
     */
    spin_lock_irqsave(&device_info.lock, flags);
    sprintf(uniqueId, "%lu", file->f_inode->i_ino);
    printk("Got device with unique_id %s\n", uniqueId);
    if (lookup(uniqueId)){
        spin_unlock_irqrestore(&device_info.lock, flags);
        return SUCCESS;
    }
    printk("Creating new struct for device with unique_id %s\n", uniqueId);
    messageInfo2 = (messageInfo *) kmalloc(sizeof(messageInfo), GFP_KERNEL);
    memset(messageInfo2->channelBuffs, '\0', 1 * NUM_OF_BUFFERS * BUFF_LEN);
    printk("malloced messageInfo device with unique_id %s\n", uniqueId);
    install(uniqueId, messageInfo2);
    printk("installed struct for device with unique_id %s\n", uniqueId);
    kfree((void *)messageInfo2);
    printk("freed struct for device with unique_id %s\n", uniqueId);
    spin_unlock_irqrestore(&device_info.lock, flags);

    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    printk("device_release(%p,%p)\n", inode, file);

    return SUCCESS;
}

/* a process which has already opened
   the device file attempts to read from it */
static ssize_t device_read(struct file *file, /* see include/linux/fs.h   */
                           char __user *buffer, /* buffer to be filled with data */
                           size_t length,  /* length of the buffer     */
                           loff_t *offset) {
    /* read doesnt really do anything (for now) */
    int i;
    char uniqueId[ID_LEN];

    struct nlist *np;
    printk("device_read(%p,%lu)\n", file, length);

    sprintf(uniqueId, "%lu", file->f_inode->i_ino);
    printk("Looking for device with unique_id %s\n", uniqueId);
    if ((np = lookup(uniqueId)) == NULL) {
        printk("Couldn't find device(%p), wasn't opened yet\n", file);
        return -1;
    }
    printk("Reading content device with unique_id %s, channel_id: %hu\n", uniqueId, np->defn->currentChannelIndex);
    for (i = 0; i < length; i++)
        put_user(np->defn->channelBuffs[np->defn->currentChannelIndex][i], buffer + i);
    return i;
}

/* somebody tries to write into our device file */
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t * offset) {
    int i;
    int returnValue;
    struct nlist *np;
    char uniqueId[ID_LEN];

    printk("device_write(%p,%lu)\n", file, length);
    sprintf(uniqueId, "%lu", file->f_inode->i_ino);
    printk("Looking for device with unique_id %s\n", uniqueId);
    if ((np = lookup(uniqueId)) == NULL) {
        printk("Couldn't find device(%p), wasn't opened yet\n", file);
        return -1;
    }

    printk("Writing content for device with unique_id %s, channel_id: %hu\n", uniqueId, np->defn->currentChannelIndex);
    for (i = 0; i < length; i++)
        get_user(np->defn->channelBuffs[np->defn->currentChannelIndex][i], buffer + i);
    returnValue = i;
    for (;i < BUFF_LEN; i++)
        np->defn->channelBuffs[np->defn->currentChannelIndex][i] = '\0';
    printk("Wrote content:%s for device with unique_id %s, channel_id: %hu\n",
           np->defn->channelBuffs[np->defn->currentChannelIndex], uniqueId, np->defn->currentChannelIndex);

    /* return the number of input characters used */
    return returnValue;
}

static long device_ioctl(//struct inode*  inode,
                         struct file*   file,
                         unsigned int   ioctl_num,/* The number of the ioctl */
                         unsigned long  ioctl_param) { /* The parameter to it */
    struct nlist *np;
    char uniqueId[ID_LEN];

    printk("device_ioctl(%p)\n", file);

    if (IOCTL_SET_CHA == ioctl_num) {
        if (ioctl_param < 0 || ioctl_param > 3) return -1;

        sprintf(uniqueId, "%lu", file->f_inode->i_ino);
        if (!(np = lookup(uniqueId))) {
            printk("Couldn't find device (%p), wasn't opened yet\n", file);
            return -1;
        }
        np->defn->currentChannelIndex = ioctl_param;
    }
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
        .unlocked_ioctl = device_ioctl,
};

/* Called when module is loaded.
 * Initialize the module - Register the character device */
static int __init simple_init(void)
{
    unsigned int rc = 0;
    /* init dev struct*/
    memset(&device_info, 0, sizeof(struct chardev_info));
    spin_lock_init(&device_info.lock);

    /* Register a character device. Get newly assigned major num */
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops /* our own file operations struct */);

    /*
     * Negative values signify an error
     */
    if (rc < 0) {
        printk(KERN_ALERT "%s failed with %d\n",
                "Sorry, registering the character device ", MAJOR_NUM);
        return -1;
    }

    printk("Registeration is a success. The major device number is %d.\n", MAJOR_NUM);
    printk("If you want to talk to the device driver,\n");
    printk("you have to create a device file:\n");
    printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
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
    hashTableCleanUp();
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
    printk("Unregisteration is a success. The major device number is %d.\n", MAJOR_NUM);
}

module_init(simple_init);
module_exit(simple_cleanup);