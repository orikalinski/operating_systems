#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define NUM_OF_PRINTABLE 95
#define CREATE_THREAD(tid, connfd) \
    if ((pthread_create(&tid, NULL, handleClient, (void *)connfd)) < 0){ \
        printf("Something went wrong with fork()! %s\n", strerror(errno)); \
    }
#define ASSIGN_SIGNAL(action, sig) \
    if (0 != sigaction(sig, &action, NULL)) { \
        printf("Something went wrong with sigaction()! %s\n", strerror(errno)); \
        return errno; \
    }


// MINIMAL ERROR HANDLING FOR EASE OF READING


pthread_mutex_t lock;
int GLOBS_STATS[NUM_OF_PRINTABLE] = {0};
int GLOBAL_COUNTER = 0;

void printStats(int numberOfBytesRead, int *stats){
    int i = 0;
    printf("\nNumber of bytes read: %d\n", numberOfBytesRead);
    printf("We saw ");
    for(; i < NUM_OF_PRINTABLE; i++){
        printf("%d '%c's", stats[i], i + 32);
        if (i < NUM_OF_PRINTABLE - 1) printf(", ");
        else printf("\n");
    }
}

void sigint_handler(int signum,
                    siginfo_t *info,
                    void *ptr) {
    printStats(GLOBAL_COUNTER, GLOBS_STATS);
    pthread_exit(NULL);
}


void handleClient(void *vargp){
    int connfd = (int) vargp;
    int localStats[NUM_OF_PRINTABLE] = {0};
    char data_buff[BUFFER_SIZE];
    char lenBuffer[BUFFER_SIZE];
    int bytes_read = 0;
    int total_read = 0;
    int numberOfFilteredChars = 0;
    int i;
    int len = -1;
    int readLen = 0;
    while (len == -1 || total_read < len) {
        bytes_read = read(connfd,
                          data_buff,
                          sizeof(data_buff) - 1);
        for (i = 0; i < bytes_read; i++){
            if (!readLen){
                if (data_buff[i] == '#') {
                    readLen = 1;
                    len = atoi(lenBuffer);
                    bytes_read -= (i + 1);
                    continue;
                }
                lenBuffer[i] = data_buff[i];
            }
            else if ((int)data_buff[i] >= 32 && (int)data_buff[i] <= 126) {
                localStats[(int) data_buff[i] - 32] += 1;
                numberOfFilteredChars += 1;
            }
        }
        if (bytes_read > 0)
            total_read += bytes_read;
    }
    char result[BUFFER_SIZE];
    int nDigits = (int) (floor(log10(abs(numberOfFilteredChars))) + 1);
    sprintf(result, "%d", numberOfFilteredChars);
    write(connfd, result, nDigits);
    pthread_mutex_lock(&lock);
    for (i = 0; i < NUM_OF_PRINTABLE; i++)
        GLOBS_STATS[i] += localStats[i];
    GLOBAL_COUNTER += total_read;
    pthread_mutex_unlock(&lock);
//    printStats(total_read, localStats);
    /* close socket  */
    close(connfd);
}


int main(int argc, char *argv[]) {
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_sigaction = sigint_handler;
    new_action.sa_flags = SA_SIGINFO;

    ASSIGN_SIGNAL(new_action, SIGINT)
    pthread_mutex_init(&lock, NULL);
    int len = -1;
    int n = 0;
    int listenfd = -1;
    int connfd = -1;

    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;

    time_t ticks;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    // INADDR_ANY = any local machine address
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(2233);

    if (0 != bind(listenfd,
                  (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr))) {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        return 1;
    }

    if (0 != listen(listenfd, 10)) {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        return 1;
    }
    pthread_t tid;
    while (1) {
        // Prepare for a new connection
        socklen_t addrsize = sizeof(struct sockaddr_in);

        // Accept a connection.
        // Can use NULL in 2nd and 3rd arguments
        // but we want to print the client socket details
        connfd = accept(listenfd,
                        (struct sockaddr *) &peer_addr,
                        &addrsize);

        if (connfd < 0) {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            return 1;
        }

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
    }
}
