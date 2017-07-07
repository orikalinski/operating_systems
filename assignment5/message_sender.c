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

int main(int argc, const char *argv[]) {
    int file_desc, ret_val;
    int channelIndex = atoi(argv[1]);
    char *message = (char *) argv[2];


    file_desc = open("/dev/"DEVICE_FILE_NAME, O_WRONLY);
    printf("file descriptor: %d\n", file_desc);
    if (file_desc < 0) {
        printf("Can't open device file: %s\n",
               DEVICE_FILE_NAME);
        exit(-1);
    }

    ret_val = ioctl(file_desc, IOCTL_SET_CHA , channelIndex);
    
    if (ret_val < 0) {
        printf("ioctl_set_msg failed:%d\n", ret_val);
        exit(-1);
    }

    ret_val = (int) write(file_desc, message, strlen(message));

    if (ret_val < 0) {
        printf("write failed:%s\n", strerror(errno));
        exit(-1);
    }

    close(file_desc);
    return 0;
}