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
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define CEIL(x, y) (1 + ((x - 1) / y))
#define UNLINK(fileName) \
    if (unlink(fileName) < 0) { \
        printf("Something went wrong with unlink()! %s\n", strerror(errno)); \
        return errno; \
    }
#define OPEN_MMAP(mapped) \
    mapped = (const char *)mmap(NULL, sizeofPagesLength, PROT_READ, MAP_SHARED, fd, sizeofPagesOffset); \
    if (mapped == MAP_FAILED) { \
        printf("open mmap failed: %s\n", strerror(errno)); \
        return 1; \
    }
#define CLOSE_MMAP(mapped, size) \
    if (munmap(mapped, size) < 0){ \
        printf("close mmap failed: %s\n", strerror(errno)); \
        return 1; \
    }

static long flag = 0;

void my_signal_handler(int signum,
                       siginfo_t *info,
                       void *ptr) {
    flag = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Wrong execution format, please pass the character to count, the text file to process, "
                       "the offset and the length");
        return 1;
    }

    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_sigaction = my_signal_handler;
    new_action.sa_flags = SA_SIGINFO;

    if (0 != sigaction(SIGUSR2, &new_action, NULL)) {
        printf("Signal handle registration failed. %s\n", strerror(errno));
        return 1;
    }

    char *charToCount = argv[1];
    char *textFileToProcess = argv[2];
    size_t offset, length;
    sscanf(argv[3], "%zu", &offset);
    sscanf(argv[4], "%zu", &length);
    size_t sizeofPagesLength = (CEIL(length, PAGE_SIZE) + 1) * PAGE_SIZE;
    size_t sizeofPagesOffset = offset - offset % PAGE_SIZE;
    size_t offsetM = (size_t) (offset % PAGE_SIZE);
    int fd = open(textFileToProcess, O_RDONLY);
    const char *mapped;
    OPEN_MMAP(mapped)
    close(fd);
    int i = offsetM;
    long count = 0;
    for (; i < offsetM + length; i++) {
        if (charToCount[0] == mapped[i]) {
            count += 1;
        }
    }
//    CLOSE_MMAP(mmap, sizeofPagesLength)
    char fileName[MAX_SIZE];
    sprintf(fileName, "/tmp/counter_%lu", (unsigned long) getpid());
    mkfifo(fileName, 0666);
    while (flag == 0) {
        kill(getppid(), SIGUSR1);
        sleep(1);
    }
    fd = open(fileName, O_WRONLY);
    char countStr[MAX_SIZE];
    sprintf(countStr, "%ld", count);
    write(fd, countStr, sizeof(countStr));
    sleep(1);
    close(fd);
    UNLINK(fileName)
}