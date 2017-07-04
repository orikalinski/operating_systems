//
// Created by ori on 27/06/17.
//

#ifndef ASSIGNMENT5_MESSAGE_SLOT_H
#define ASSIGNMENT5_MESSAGE_SLOT_H

#define HASHSIZE 101
#define BUFF_LEN 128
#define NULL 0
/* The major device number. We can't rely on dynamic
 * registration any more, because ioctls need to know
 * it. */
#define MAJOR_NUM 245


/* Set the message of the device driver */
#define IOCTL_SET_ENC _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot"
#define DEVICE_FILE_NAME "message_slot"
#define SUCCESS 0



#include <linux/ioctl.h>
#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>


typedef struct message_info {
    char channelBuff1[BUFF_LEN];
    char channelBuff2[BUFF_LEN];
    char channelBuff3[BUFF_LEN];
    char channelBuff4[BUFF_LEN];
    short currentChannelIndex;
} messageInfo;

struct nlist { /* table entry: */
    struct nlist *next; /* next entry in chain */
    struct nlist *previous; /* previous entry in chain */
    char *name; /* defined name */
    messageInfo *defn; /* replacement text */
};

#endif //ASSIGNMENT5_MESSAGE_SLOT_H
