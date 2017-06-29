//https://stackoverflow.com/questions/4384359/quick-way-to-implement-dictionary-in-c


struct nlist { /* table entry: */
    struct nlist *next; /* next entry in chain */
    struct nlist *previous; /* previous entry in chain */
    char *name; /* defined name */
    messageInfo *defn; /* replacement text */
};

static struct nlist *hashtab[HASHSIZE];

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

char *strdup(char *);
char *messageSlotDup(char *);
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
    unsigned hashval;
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

char *strdup(char *s) /* make a duplicate of s */
{
    char *p;
    p = (char *) kmalloc(strlen(s)+1, GFP_KERNEL); /* +1 for ’\0’ */
    if (p != NULL)
        strcpy(p, s);
    return p;
}

messageInfo *messageSlotDup(messageInfo *messageInfo1) /* make a duplicate of messageInfo1 */
{
    messageInfo *messageInfo2;
    messageInfo2 = (messageInfo *) kmalloc(sizeof(messageInfo), GFP_KERNEL);
    if (messageInfo2 != NULL){
        messageInfo2->channelBuff1 = messageInfo1->channelBuff1;
        messageInfo2->channelBuff2 = messageInfo1->channelBuff2;
        messageInfo2->channelBuff3 = messageInfo1->channelBuff3;
        messageInfo2->channelBuff4 = messageInfo1->channelBuff4;
        messageInfo2->currentChannelIndex = messageInfo1->currentChannelIndex;
    }
    return messageInfo2;
}



