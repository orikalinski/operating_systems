//
// Created by ori on 27/06/17.
//

#include "message_slot.h"

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

static int major; /* device major number */

//https://stackoverflow.com/questions/4384359/quick-way-to-implement-dictionary-in-c
static struct nlist *hashtab[HASHSIZE];

messageInfo *messageSlotDup(messageInfo *messageInfo1) /* make a duplicate of messageInfo1 */
{
    int i = 0;
    messageInfo *messageInfo2;
    messageInfo2 = (messageInfo *) kmalloc(sizeof(messageInfo), GFP_KERNEL);
    if (messageInfo2 != NULL){
        for (; i < BUFF_LEN; i++) {
            messageInfo2->channelBuff1[i] = messageInfo1->channelBuff1[i];
            messageInfo2->channelBuff2[i] = messageInfo1->channelBuff2[i];
            messageInfo2->channelBuff3[i] = messageInfo1->channelBuff3[i];
            messageInfo2->channelBuff4[i] = messageInfo1->channelBuff4[i];
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
        hashtab[hashval]->previous = np;
        hashtab[hashval] = np;
    } else /* already there */
        kfree((void *) np->defn); /*kfree previous defn */
    if ((np->defn = messageSlotDup(defn)) == NULL)
        return NULL;
    return np;
}

struct nlist *pop(char *name)
{
    struct nlist *np;
    if ((np = lookup(name)) == NULL) { /* not found */
        return NULL;
    } else {/* already there */
        np->previous->next = np->next;
        np->next->previous = np->previous;
        kfree((void *) np->defn); /*kfree previous defn */
        kfree((void *) np->name); /*kfree name */
        kfree((void *) np); /*kfree np */
        return np;
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
    if (lookup(uniqueId)){
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -EBUSY;
    }
    messageInfo2 = (messageInfo *) kmalloc(sizeof(messageInfo), GFP_KERNEL);
    install(uniqueId, messageInfo2);
    kfree((void *)messageInfo2);
    spin_unlock_irqrestore(&device_info.lock, flags);

    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    char uniqueId[ID_LEN];
    unsigned long flags; // for spinlock
    printk("device_release(%p,%p)\n", inode, file);

    /* ready for our next caller */
    spin_lock_irqsave(&device_info.lock, flags);
    sprintf(uniqueId, "%lu", file->f_inode->i_ino);
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
    char uniqueId[ID_LEN];
    struct nlist *np;
    sprintf(uniqueId, "%lu", file->f_inode->i_ino);
    if ((np = lookup(uniqueId)) == NULL) {
        printk("Couldn't find device(%p), wasn't opened yet\n", file);
        return 1;
    }
    if (np->defn->currentChannelIndex == 0)
        printk("device_read(%p,%lu) - operation not supported yet (last written - %s)\n", file, length, np->defn->channelBuff1);
    if (np->defn->currentChannelIndex == 1)
        printk("device_read(%p,%lu) - operation not supported yet (last written - %s)\n", file, length, np->defn->channelBuff2);
    if (np->defn->currentChannelIndex == 2)
        printk("device_read(%p,%lu) - operation not supported yet (last written - %s)\n", file, length, np->defn->channelBuff3);
    if (np->defn->currentChannelIndex == 3)
        printk("device_read(%p,%lu) - operation not supported yet (last written - %s)\n", file, length, np->defn->channelBuff4);

    return -EINVAL; // invalid argument error
}

/* somebody tries to write into our device file */
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t * offset) {
    int i;
    struct nlist *np;
    char uniqueId[ID_LEN];

    printk("device_write(%p,%lu)\n", file, length);
    sprintf(uniqueId, "%lu", file->f_inode->i_ino);
    if (!(np = lookup(uniqueId))) {
        printk("Couldn't find device(%p), wasn't opened yet\n", file);
        return 1;
    }

    // TODO: adding zeros to the end of the buffer
    for (i = 0; i < BUF_LEN; i++) {
        if (np->defn->currentChannelIndex == 0)
            get_user(np->defn->channelBuff1[i], buffer + i);
        if (np->defn->currentChannelIndex == 1)
            get_user(np->defn->channelBuff2[i], buffer + i);
        if (np->defn->currentChannelIndex == 2)
            get_user(np->defn->channelBuff3[i], buffer + i);
        if (np->defn->currentChannelIndex == 3)
            get_user(np->defn->channelBuff4[i], buffer + i);
    }

    /* return the number of input characters used */
    return i;
}

static long device_ioctl(//struct inode*  inode,
                         struct file*   file,
                         unsigned int   ioctl_num,/* The number of the ioctl */
                         unsigned long  ioctl_param) { /* The parameter to it */
    struct nlist *np;
    char uniqueId[ID_LEN];

    if (ioctl_param < 0 || ioctl_param > 3) return -1;

    sprintf(uniqueId, "%lu", file->f_inode->i_ino);
    if (!(np = lookup(uniqueId))) {
        printk("Couldn't find device (%p), wasn't opened yet\n", file);
        return -1;
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
        .unlocked_ioctl = device_ioctl,
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