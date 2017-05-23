#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_SIZE 1024
#define MAX_NUMBER_OF_PROCS 16
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define SIZE_PER_PROCESS 2 * PAGE_SIZE
#define CEIL(x, y) 1 + ((x - 1) / y);
#define READ_STAT(stat1) \
    if (stat(textFileToProcess, stat1) < 0) { \
        printf("Something went wrong with stat()! %s\n", strerror(errno)); \
        return errno; \
    }
#define ASSIGN_SIGNAL(action, sig) \
    if (0 != sigaction(sig, &action, NULL)) { \
        printf("Something went wrong with sigaction()! %s\n", strerror(errno)); \
        return errno; \
    }
#define OPEN(fd, filePath, oFlags) \
    if ((fd = open(filePath, oFlags)) == -1){ \
        gotError = true; \
        printf("Something went wrong with open()! %s\n", strerror(errno)); \
        return; \
    }
#define SIGNAL(pid, sig) \
    if (kill(pid, sig) < 0){ \
        gotError = true; \
        printf("Something went wrong with kill()! %s\n", strerror(errno)); \
        return; \
    }
#define READ(fd, buffer, size) \
    size_t readSize; \
    if ((readSize = read(fd, buffer, size)) == -1){ \
        gotError = true; \
        printf("Something went wrong with read()! %s\n", strerror(errno)); \
        close(fd); \
        return; \
    }
#define FORK(pid) \
    if ((pid = fork()) < 0){ \
        gotError = true; \
        printf("Something went wrong with fork()! %s\n", strerror(errno)); \
    }


static long counter = 0;
static long procs[MAX_NUMBER_OF_PROCS];
int currentProc = 0;
bool gotError = false;

bool already_handled_signal(long pid){
    int i = 0;
    for (; i < currentProc; i++)
        if (procs[i] == pid)
            return true;
    return false;
}

void my_signal_handler(int signum,
                       siginfo_t *info,
                       void *ptr) {
    unsigned long pid = (unsigned long) info->si_pid;
    if (already_handled_signal(pid)) return;
    procs[currentProc] = pid;
    currentProc++;
    SIGNAL((pid_t)pid, SIGUSR2);
    char fileName[MAX_SIZE];
    sprintf(fileName, "/tmp/counter_%lu", pid);
    int fd;
    OPEN(fd, fileName, O_RDONLY)
    char buffer[MAX_SIZE];
    READ(fd, buffer, MAX_SIZE);
    long count = 0;
    sscanf(buffer, "%ld", &count);
    counter += count;
    close(fd);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Wrong execution format, please pass the character to count and the text file to process");
        return 1;
    }

    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_sigaction = my_signal_handler;
    new_action.sa_flags = SA_SIGINFO;

    ASSIGN_SIGNAL(new_action, SIGUSR1)

    char *textFileToProcess = argv[2];
    struct stat *stat1;
    if ((stat1 = (struct stat *) malloc(sizeof(struct stat))) == NULL) {
        printf("Couldn't allocate memory for the stats");
        return 1;
    }
    READ_STAT(stat1)
    ssize_t fileSize = stat1->st_size;
    ssize_t numOfProcs = CEIL(fileSize, SIZE_PER_PROCESS)
    numOfProcs = numOfProcs > MAX_NUMBER_OF_PROCS ? MAX_NUMBER_OF_PROCS : numOfProcs;

    int i;
    long length = CEIL(fileSize, numOfProcs);
    char lengthStr[MAX_SIZE];
    char offsetStr[MAX_SIZE];
    pid_t pid;
    for (i = 0; i < numOfProcs; i++){
        FORK(pid)
        if (pid == 0) {
            sprintf(lengthStr, "%ld", length * (i + 1) > fileSize ? fileSize - length * i : length);
            sprintf(offsetStr, "%ld", length * i);
            int res = execv("./counter",
                            (char *[]) {"./counter", argv[1], argv[2], offsetStr, lengthStr, NULL});
            if (res != 0)
                gotError = true;
            perror("execv");
            return 0;
        }
    }
    int status;
    while ((pid = waitpid(-1, &status, 0))) {
        if (pid < 0) {
            if (errno == ECHILD) {
                break;
            }
        }
        else {
            if (WIFEXITED(status))
                if (WEXITSTATUS(status) != 0)
                    gotError = true;
        }
    }
    if (gotError)
        printf("Got Error, printing partial results\n");
    printf("The character: %s, was written: %ld times in the text file: %s\n", argv[1], counter, argv[2]);
    return 0;
}