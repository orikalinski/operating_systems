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
#include <wait.h>

#define MAX_SIZE 1024
#define CEIL(x, y) 1 + ((x - 1) / y);

static long counter = 0;

void my_signal_handler(int signum,
                       siginfo_t *info,
                       void *ptr) {
    unsigned long pid = (unsigned long) info->si_pid;
    printf("Signal sent from process %lu\n", pid);
    char fileName[MAX_SIZE];
    sprintf(fileName, "/tmp/counter_%lu", pid);
    int fd = open(fileName, O_RDONLY);
    char buffer[MAX_SIZE];
    read(fd, buffer, MAX_SIZE);
    long count;
    sscanf(buffer, "%ld", &count);
    counter += count;
    close(fd);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Wrong execution format, please pass the character to count and the text file to process");
        return 1;
    }
    char *textFileToProcess = argv[2];
    struct stat *stat1;
    if ((stat1 = (struct stat *) malloc(sizeof(struct stat))) == NULL) {
        printf("Couldn't allocate memory for the stats");
        return 1;
    }
    if (stat(textFileToProcess, stat1) < 0) {
        printf("Something went wrong with stat()! %s\n", strerror(errno));
        return errno;
    }
    ssize_t fileSize = stat1->st_size;
    long sqrtSize = (long)sqrt(stat1->st_size);

    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_sigaction = my_signal_handler;
    new_action.sa_flags = SA_SIGINFO;

    if (0 != sigaction(SIGUSR1, &new_action, NULL)) {
        printf("Signal handle registration failed. %s\n", strerror(errno));
        return 1;
    }
    int i;
    long length = CEIL(fileSize, sqrtSize);
    char lengthStr[MAX_SIZE];
    char offsetStr[MAX_SIZE];
    for (i = 0; i < sqrtSize; i++){
        pid_t j = fork();
        printf("pid: %ld\n", (long) j);
        if (j == 0) {
            sprintf(lengthStr, "%ld", length * (i + 1) > fileSize ? fileSize - length * i : length);
            sprintf(offsetStr, "%ld", length * i);
            printf("length: %s, offset: %s\n", lengthStr, offsetStr);
            execv("/home/ori/computer_science/operating_systems/assignment3/counter",
                  (char *[]) {"/home/ori/computer_science/operating_systems/assignment3/counter", argv[1], argv[2], offsetStr, lengthStr});
            return 0;
        }
        else if (j > 0);
        else
            printf("fork failed\n");
    }
    while (wait(NULL) > 0);

    return 0;
}