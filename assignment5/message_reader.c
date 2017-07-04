//
// Created by ori on 30/06/17.
//

#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[]) {
    int file_desc, ret_val;
    int channelIndex = atoi(argv[1]);
    char message[BUFF_LEN];


    file_desc = open("/dev/"DEVICE_FILE_NAME, 0);
    if (file_desc < 0) {
        printf("Can't open device file: %s\n",
               DEVICE_FILE_NAME);
        exit(-1);
    }

    ret_val = ioctl(file_desc, 0, channelIndex);
    
    if (ret_val < 0) {
        printf("ioctl_set_msg failed:%d\n", ret_val);
        exit(-1);
    }

    if (read(file_desc, message, BUFF_LEN) < 0) return 1;

    close(file_desc);
    return 0;
}