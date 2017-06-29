//
// Created by ori on 28/06/17.
//

#ifndef ASSIGNMENT5_HASHMAP_H
#define ASSIGNMENT5_HASHMAP_H

#define HASHSIZE 101
#define BUFF_LEN 128

typedef struct message_info {
    char channelBuff1[BUFF_LEN];
    char channelBuff2[BUFF_LEN];
    char channelBuff3[BUFF_LEN];
    char channelBuff4[BUFF_LEN];
    short currentChannelIndex;
} messageInfo;

struct nlist *install(char *name, messageInfo *defn);

struct nlist *lookup(char *s);

struct nlist *pop(char *name);

#endif //ASSIGNMENT5_HASHMAP_H
