//
// Created by ori on 27/06/17.
//

#ifndef ASSIGNMENT5_MESSAGE_SLOT_H
#define ASSIGNMENT5_MESSAGE_SLOT_H

#define HASHSIZE 101
#define BUFF_LEN 128
#define NUM_OF_BUFFERS 4
#define NULL 0
/* The major device number. We can't rely on dynamic
 * registration any more, because ioctls need to know
 * it. */
#define MAJOR_NUM 247


/* Set the message of the device driver */
#define IOCTL_SET_CHA _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot"
#define DEVICE_FILE_NAME "message_slot"
#define SUCCESS 0


typedef struct message_info {
    char channelBuffs[NUM_OF_BUFFERS][BUFF_LEN];
    short currentChannelIndex;
} messageInfo;

struct nlist { /* table entry: */
    struct nlist *next; /* next entry in chain */
    struct nlist *previous; /* previous entry in chain */
    char *name; /* defined name */
    messageInfo *defn; /* replacement text */
};

#endif //ASSIGNMENT5_MESSAGE_SLOT_H
