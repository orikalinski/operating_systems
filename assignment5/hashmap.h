//
// Created by ori on 28/06/17.
//

#ifndef ASSIGNMENT5_HASHMAP_H
#define ASSIGNMENT5_HASHMAP_H

#define HASHSIZE 101
#define BUFF_LEN 128
#define NULL 0

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

struct nlist *install(char *name, messageInfo *defn);

struct nlist *lookup(char *s);

struct nlist *pop(char *name);

#endif //ASSIGNMENT5_HASHMAP_H
