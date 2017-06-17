#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <math.h>

#define BUFFER_SIZE 1024

#define WRITE(fd, buffer, size) \
    if ((nsent = write(fd, buffer, size)) == -1){ \
        printf("Something went wrong with write()! %s\n", strerror(errno)); \
        close(fd); \
        return errno; \
    }
#define READ(fd, buffer, size) \
    size_t readSize; \
    if ((readSize = read(fd, buffer, size)) == -1){ \
        printf("Something went wrong with read()! %s\n", strerror(errno)); \
        close(fd); \
        return errno; \
    }
#define OPEN(fd, filePath, oFlags) \
    if ((fd = open(filePath, oFlags)) == -1){ \
        printf("Something went wrong with open()! %s\n", strerror(errno)); \
        return errno; \
    }
#define CONNECT(sockfd, serv_addr) \
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { \
        printf("Something went wrong with connect()! %s\n", strerror(errno)); \
        return 1; \
    }

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Wrong execution format, please pass "
                       "LEN - the length (in bytes) of the stream to process\n");
        return 1;
    }
    int len = atoi(argv[1]);
    int sockfd;
    int totalsent;
    int nsent;
    int batchSize;
    int batchSent;
    char dataBuffer[BUFFER_SIZE];

    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    // print socket details
    getsockname(sockfd,
                (struct sockaddr *) &my_addr,
                &addrsize);
    printf("Client: socket created %s:%d\n",
           inet_ntoa((my_addr.sin_addr)),
           ntohs(my_addr.sin_port));

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(2233); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // hardcoded...

    printf("Client: connecting...\n");
    // Note: what about the client port number?
    // connect socket to the target address
    CONNECT(sockfd, serv_addr)

    // print socket details again
    getsockname(sockfd, (struct sockaddr *) &my_addr, &addrsize);
    getpeername(sockfd, (struct sockaddr *) &peer_addr, &addrsize);
    printf("Client: Connected. \n"
                   "\t\tSource IP: %s Source Port: %d\n"
                   "\t\tTarget IP: %s Target Port: %d\n",
           inet_ntoa((my_addr.sin_addr)), ntohs(my_addr.sin_port),
           inet_ntoa((peer_addr.sin_addr)), ntohs(peer_addr.sin_port));

    // write data to server from urandom
    // block until there's something to read
    // print data to screen every time
    int randfd;
    OPEN(randfd, "/dev/urandom", O_RDONLY)
    totalsent = 0;
    int nDigits = (int) (floor(log10(abs(len))) + 1);
    sprintf(dataBuffer, "%d#", len);
    batchSize = nDigits + 1;
    batchSent = 0;
    while (batchSent < batchSize) {
        WRITE(sockfd, dataBuffer + batchSent, (size_t) (batchSize))
        printf("Client: wrote %d bytes\n", nsent);
        batchSent += nsent;
    }

    while (totalsent < len) {
        batchSize = BUFFER_SIZE < len - totalsent ? BUFFER_SIZE : len - totalsent;
        READ(randfd, dataBuffer, (size_t) batchSize)
        batchSent = 0;
        while (batchSent < batchSize) {
            WRITE(sockfd, dataBuffer + batchSent, (size_t) batchSize);
            // check if error occured (client closed connection?)
            if (nsent < 0) {
                printf("\n Error : nsent < 0. %s \n", strerror(errno));
                return 1;
            }
            printf("Client: wrote %d bytes\n", nsent);
            totalsent += nsent;
            batchSent += nsent;
        }
    }
    char result[BUFFER_SIZE];
    int i;
    printf("Number of filtered bytes read: ");
    while (1) {
        READ(sockfd, result, BUFFER_SIZE)
        for (i = 0; i < readSize; i++) {
            printf("%c", result[i]);
        }
        if (readSize <= 0) break;
    }
    printf("\n");
    close(sockfd);
    return 0;
}
