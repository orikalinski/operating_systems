#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>

// MINIMAL ERROR HANDLING FOR EASE OF READING
#define BUFFER_SIZE 1024


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
    char data_buff[1025];

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
    if (connect(sockfd,
                (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0) {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        return 1;
    }

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
    int randfd = open("/dev/urandom", O_RDONLY);
    totalsent = 0;
    int notwritten;
    int nDigits = (int) (floor(log10(abs(len))) + 1);
    notwritten = nDigits + 1;
    sprintf(data_buff, "%d#", len);
    data_buff[notwritten] = '\0';
    while (notwritten > 0) {
        // notwritten = how much we have left to write
        // totalsent  = how much we've written so far
        // nsent = how much we've written in last write() call */
        nsent = (int) write(sockfd,
                            data_buff + nDigits + 1 - notwritten,
                            (size_t) (notwritten));
        // check if error occured (client closed connection?)
        if (nsent < 0) {
            printf("\n Error : nsent < 0. %s \n", strerror(errno));
            return 1;
        }
        printf("Client: wrote %d bytes\n", nsent);
        notwritten -= nsent;
    }

    while (totalsent < len) {
        notwritten = BUFFER_SIZE < len - totalsent ? BUFFER_SIZE : len - totalsent;
        read(randfd, data_buff, (size_t) notwritten);
        // keep looping until nothing left to write
        while (notwritten > 0) {
            // notwritten = how much we have left to write
            // totalsent  = how much we've written so far
            // nsent = how much we've written in last write() call */
            nsent = (int) write(sockfd,
                                data_buff + BUFFER_SIZE - notwritten,
                                (size_t) notwritten);
            // check if error occured (client closed connection?)
            if (nsent < 0) {
                printf("\n Error : nsent < 0. %s \n", strerror(errno));
                return 1;
            }
            printf("Client: wrote %d bytes\n", nsent);

            totalsent += nsent;
            notwritten -= nsent;
        }
    }
    char result[BUFFER_SIZE];
    int bytes_read;
    int i;
    printf("Number of filtered bytes read: ");
    while (1) {
        bytes_read = (int) read(sockfd, result, BUFFER_SIZE);
        for (i = 0; i < bytes_read; i++) {
            printf("%c", result[i]);
        }
        if (bytes_read <= 0) break;
    }
    printf("\n");
    close(sockfd); // is socket really done here?
    //printf("Write after close returns %d\n", write(sockfd, recvBuff, 1));
    return 0;
}
