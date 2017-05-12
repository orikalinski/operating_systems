//
// Created by ori on 10/05/17.
//

#include <stdio.h>
#include <malloc.h>
#include <zconf.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <ctype.h>

#define MAX_SIZE 1024
#define PAGE_SIZE 4096
#define CEIL(x, y) (1 + ((x - 1) / y))

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Wrong execution format, please pass the character to count, the text file to process, "
                       "the offset and the length");
        return 1;
    }
    char *charToCount = argv[1];
    char *textFileToProcess = argv[2];
    size_t offset, length;
    sscanf(argv[3], "%zu", &offset);
    sscanf(argv[4], "%zu", &length);
    size_t sizeofPagesLength = CEIL(length, PAGE_SIZE) * PAGE_SIZE;
    size_t sizeofPagesOffset = (size_t) (offset / (double)PAGE_SIZE);
    size_t offsetM = (size_t) (offset % PAGE_SIZE);
    int fd = open(textFileToProcess, O_RDONLY);
    const char *mapped = (const char *)mmap(NULL, sizeofPagesLength, PROT_READ, MAP_SHARED, fd, sizeofPagesOffset);
    if (mapped == MAP_FAILED) {
        printf("mmap %s failed: %s\n", textFileToProcess, strerror (errno));
        return 1;
    }
    int i = offsetM;
    long count = 0;
    for (; i < offsetM + length; i++) {
        if (charToCount[0] == mapped[i])
            count += 1;
    }
    close(fd);
    char fileName[MAX_SIZE];
    sprintf(fileName, "/tmp/counter_%lu", (unsigned long) getpid());
    mkfifo(fileName, 0666);
    printf("sending signal to parent %lu\n", (unsigned long) getpid());
    kill(getppid(), SIGUSR1);
    printf("sent signal to parent %lu\n", (unsigned long) getpid());
    fd = open(fileName, O_WRONLY);
    char countStr[MAX_SIZE];
    sprintf(countStr, "%ld", count);
    printf("count: %ld\n", count);
    write(fd, countStr, sizeof(countStr));
    sleep(1);
    close(fd);
    if (unlink(fileName) < 0)
        return 1;
}