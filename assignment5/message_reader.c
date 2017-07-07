//
// Created by ori on 30/06/17.
//

#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, const char *argv[]) {
    int file_desc, ret_val;
    int channelIndex = atoi(argv[1]);
    char message[BUFF_LEN];


    file_desc = open("/dev/"DEVICE_FILE_NAME, O_RDONLY);
    if (file_desc < 0) {
        printf("open failed:%s\n", strerror(errno));
        exit(-1);
    }

    ret_val = ioctl(file_desc, IOCTL_SET_CHA, channelIndex);

    if (ret_val < 0) {
        printf("ioctl failed:%s\n", strerror(errno));
        exit(-1);
    }

    ret_val = read(file_desc, message, BUFF_LEN);

    if (ret_val < 0) {
        printf("read failed:%s\n", strerror(errno));
        exit(-1);
    }

    printf("The message is: %s\n", message);

    close(file_desc);
    return 0;
}