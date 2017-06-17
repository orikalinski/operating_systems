#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define NUM_OF_PRINTABLE 95
#define CREATE_THREAD(tid, connfd) \
    if ((pthread_create(&tid, NULL, handleClient, (int *)connfd)) < 0){ \
        printf("Something went wrong with pthread_create()!\n"); \
        continue; \
    }
#define ASSIGN_SIGNAL(action, sig) \
    if (0 != sigaction(sig, &action, NULL)) { \
        printf("Something went wrong with sigaction()! %s\n", strerror(errno)); \
        return errno; \
    }
#define WRITE(fd, buffer, size) \
    if ((nsent = write(fd, buffer, size)) == -1){ \
        printf("Something went wrong with write()! %s\n", strerror(errno)); \
        close(fd); \
        return errno; \
    }
#define ACCEPT(listenfd, peer_addr, addrsize) \
    if ((connfd = accept(listenfd, \
                         (struct sockaddr *) &peer_addr, \
                         &addrsize)) < 0) { \
        printf("Something went wrong with accept(). %s\n", strerror(errno)); \
        continue; \
    }
#define READ(fd, buffer, size) \
    size_t readSize; \
    if ((readSize = read(fd, buffer, size)) == -1){ \
        printf("Something went wrong with read()! %s\n", strerror(errno)); \
        close(fd); \
        return errno; \
    }


pthread_mutex_t lock;
int GLOBS_STATS[NUM_OF_PRINTABLE] = {0};
int GLOBAL_COUNTER = 0;
int numOfThreads = 0;
int listenfd;

void printStats(int numberOfBytesRead, int *stats) {
    int i = 0;
    printf("\nNumber of bytes read: %d\n", numberOfBytesRead);
    printf("We saw ");
    for (; i < NUM_OF_PRINTABLE; i++) {
        printf("%d '%c's", stats[i], i + 32);
        if (i < NUM_OF_PRINTABLE - 1) printf(", ");
        else printf("\n");
    }
}

void sigint_handler(int signum,
                    siginfo_t *info,
                    void *ptr) {
    int i = 0;
    for (; i < 5; i++){
        if (numOfThreads > 0)
            sleep(1);
        else {
            break;
        }
    }
    printStats(GLOBAL_COUNTER, GLOBS_STATS);
    close(listenfd);
    exit(0);
}


void *handleClient(void *vargp) {
    int connfd = (int) (uintptr_t) vargp;
    int localStats[NUM_OF_PRINTABLE] = {0};
    char dataBuffer[BUFFER_SIZE];
    char lenBuffer[BUFFER_SIZE];
    int totalRead = 0;
    int numberOfFilteredChars = 0;
    int i;
    int len = -1;
    int readLen = 0;
    while (len == -1 || totalRead < len) {
        READ(connfd, dataBuffer, sizeof(dataBuffer) - 1);
        for (i = 0; i < readSize; i++) {
            if (!readLen) {
                if (dataBuffer[i] == '#') {
                    readLen = 1;
                    len = atoi(lenBuffer);
                    totalRead -= (i + 1);
                    continue;
                }
                lenBuffer[i] = dataBuffer[i];
            } else if ((int) dataBuffer[i] >= 32 && (int) dataBuffer[i] <= 126) {
                localStats[(int) dataBuffer[i] - 32] += 1;
                numberOfFilteredChars += 1;
            }
        }
        if (readSize > 0)
            totalRead += readSize;
    }
    char result[BUFFER_SIZE];
    int nDigits = (int) (floor(log10(abs(numberOfFilteredChars))) + 1);
    sprintf(result, "%d", numberOfFilteredChars);
    int totalSent = 0, nsent;
    while (totalSent < nDigits) {
        WRITE(connfd, result, (size_t) nDigits);
        totalSent += nsent;
    }
    pthread_mutex_lock(&lock);
    numOfThreads -= 1;
    for (i = 0; i < NUM_OF_PRINTABLE; i++)
        GLOBS_STATS[i] += localStats[i];
    GLOBAL_COUNTER += totalRead;
    pthread_mutex_unlock(&lock);
    /* close socket  */
    close(connfd);
    pthread_exit(NULL);
    return 0;
}


int main(int argc, char *argv[]) {
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_sigaction = sigint_handler;
    new_action.sa_flags = SA_SIGINFO;

    ASSIGN_SIGNAL(new_action, SIGINT)
    pthread_mutex_init(&lock, NULL);
    int connfd;

    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(2233);

    if (0 != bind(listenfd,
                  (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr))) {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        return 1;
    }

    if (0 != listen(listenfd, 10000)) {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        return 1;
    }
    pthread_t tid;
    while (1) {
        // Prepare for a new connection
        socklen_t addrsize = sizeof(struct sockaddr_in);

        // Accept a connection.
        ACCEPT(listenfd, peer_addr, addrsize)

        getsockname(connfd, (struct sockaddr *) &my_addr, &addrsize);
        getpeername(connfd, (struct sockaddr *) &peer_addr, &addrsize);
        printf("Server: Client connected.\n"
                       "\t\tClient IP: %s Client Port: %d\n"
                       "\t\tServer IP: %s Server Port: %d\n",
               inet_ntoa(peer_addr.sin_addr),
               ntohs(peer_addr.sin_port),
               inet_ntoa(my_addr.sin_addr),
               ntohs(my_addr.sin_port));

        CREATE_THREAD(tid, connfd)
        numOfThreads += 1;
    }
}
