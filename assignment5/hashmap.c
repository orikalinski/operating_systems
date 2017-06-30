//https://stackoverflow.com/questions/4384359/quick-way-to-implement-dictionary-in-c

#include "hashmap.h"

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