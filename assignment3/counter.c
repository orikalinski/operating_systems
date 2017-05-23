//
// Created by ori on 10/05/17.
//

#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

#define MAX_SIZE 1024
#define FIFO_PERM 0666
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define CEIL(x, y) (1 + ((x - 1) / y))
#define UNLINK(fileName) \
    if (unlink(fileName) < 0) { \
        printf("Something went wrong with unlink()! %s\n", strerror(errno)); \
        return errno; \
    }
#define OPEN_MMAP(mapped, size, fd, offset) \
    mapped = (const char *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, offset); \
    if (mapped == MAP_FAILED) { \
        printf("Something went wrong with mmap()! %s\n", strerror(errno)); \
        return errno; \
    }
#define CLOSE_MMAP(mapped, size) \
    if (munmap((void *)mapped, size) < 0){ \
        printf("Something went wrong with munmap()! %s\n", strerror(errno)); \
        return errno; \
    }
#define SIGNAL(pid, sig) \
    if (kill(pid, sig) < 0){ \
        printf("Something went wrong with kill()! %s\n", strerror(errno)); \
        return errno; \
    }
#define CREATE_FIFO(fileName) \
    if(access(fileName, F_OK ) != -1) \
        remove(fileName); \
    if (mkfifo(fileName, FIFO_PERM) < 0) { \
        printf("Something went wrong with mkfifo()! %s\n", strerror(errno)); \
        return errno; \
    }
#define ASSIGN_SIGNAL(action, sig) \
    if (0 != sigaction(sig, &action, NULL)) { \
        printf("Something went wrong with sigaction()! %s\n", strerror(errno)); \
        return errno; \
    }
#define WRITE(fd, buffer, size) \
    if ((write(fd, buffer, size)) == -1){ \
        printf("Something went wrong with write()! %s\n", strerror(errno)); \
        close(fd); \
        return errno; \
    }
#define OPEN(fd, filePath, oFlags) \
    if ((fd = open(filePath, oFlags)) == -1){ \
        printf("Something went wrong with open()! %s\n", strerror(errno)); \
        return errno; \
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

    ASSIGN_SIGNAL(new_action, SIGUSR2)

    char *charToCount = argv[1];
    char *textFileToProcess = argv[2];
    size_t offset, length;
    sscanf(argv[3], "%zu", &offset);
    sscanf(argv[4], "%zu", &length);
    size_t sizeofPagesLength = (CEIL(length, PAGE_SIZE) + 1) * PAGE_SIZE;
    size_t sizeofPagesOffset = offset - offset % PAGE_SIZE;
    size_t offsetM = (size_t) (offset % PAGE_SIZE);
    int fd;
    const char *mapped;
    OPEN(fd, textFileToProcess, O_RDONLY)
    OPEN_MMAP(mapped, sizeofPagesLength, fd, sizeofPagesOffset)
    close(fd);
    int i = offsetM;
    long count = 0;
    for (; i < offsetM + length; i++) {
        if (charToCount[0] == mapped[i]) {
            count += 1;
        }
    }
    CLOSE_MMAP(mapped, sizeofPagesLength)
    char fileName[MAX_SIZE];
    sprintf(fileName, "/tmp/counter_%lu", (unsigned long) getpid());
    CREATE_FIFO(fileName)
    while (flag == 0) {
        SIGNAL(getppid(), SIGUSR1)
    }
    OPEN(fd, fileName, O_WRONLY)
    char countStr[MAX_SIZE];
    sprintf(countStr, "%ld", count);
    WRITE(fd, countStr, sizeof(countStr));
    close(fd);
    UNLINK(fileName)
}