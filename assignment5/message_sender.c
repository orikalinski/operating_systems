//
// Created by ori on 30/06/17.
//

#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define CLOSE(file_desc) \
    ret_val = close(file_desc); \
    if (ret_val < 0) { \
        printf("close failed:%s\n", strerror(errno)); \
        exit(-1); \
    }

int main(int argc, const char *argv[]) {
    int file_desc, ret_val;
    int channelIndex = atoi(argv[1]);
    char *message = (char *) argv[2];


    file_desc = open("/dev/"DEVICE_FILE_NAME, O_WRONLY);
    if (file_desc < 0) {
        printf("open failed:%s\n", strerror(errno));
        exit(-1);
    }

    ret_val = ioctl(file_desc, IOCTL_SET_CHA, channelIndex);

    if (ret_val < 0) {
        printf("ioctl failed:%s\n", strerror(errno));
        CLOSE(file_desc)
        exit(-1);
    }

    ret_val = (int) write(file_desc, message, strlen(message));

    if (ret_val < 0) {
        printf("write failed:%s\n", strerror(errno));
        CLOSE(file_desc)
        exit(-1);
    }

    printf("The message: %s was successfully written", message);

    CLOSE(file_desc)
    return 0;
}