/*************************************************************************
	> File Name: master.c
	> Author: Dracula
	> Mail: dracula.guanyu.li@gmail.com
	> Created Time: Friday, December 01, 2017 PM04:02:43 HKT
 ************************************************************************/

#include "master.h"

int main(void) {
    int master_sockfd;
    int il_sockfd;
    struct sockaddr_in master_addr;
    struct sockaddr_in il_addr;
    socklen_t len = sizeof(il_addr);

    char data;

    // Start Network Function Threand(Here a simple packet counter)
    pthread_t nf_thread;
    packet_counter_arg pc_arg;
    pthread_mutex_t mutex;
    int counter;

    pc_arg.mutex = &mutex;
    pc_arg.counter = &counter;
    pthread_mutex_init(pc_arg.mutex, NULL);
    if (pthread_create(&nf_thread, NULL, &packet_counter, (void*)&pc_arg) != 0) {
        fprintf(stderr, "Error: FTMB-Master: can't create "
                "network function(packet counter) thread!");
    }

    // Creat a socket
    master_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    master_addr.sin_family = AF_INET;
    master_addr.sin_addr.s_addr = inet_addr(MASTER_IP);
    master_addr.sin_port = htons(MASTER_IL_PORT);

    // Bind
    bind(master_sockfd, (struct sockaddr *)&master_addr, sizeof(master_addr));
    // Listen
    listen(master_sockfd, 1);
    fprintf(stdout, "FTMB-Master: Master is waiting for InputLogger to connect...\n");
    // Accept a connection
    il_sockfd = accept(master_sockfd, (struct sockaddr *)&il_addr, &len);
    fprintf(stdout, "FTMB-Master: Receive connection request from InputLogger\n");

    while (1) {
        read(il_sockfd, &data, 1);
        if (data == 's') {
            fprintf(stdout, "FTMB-Master: Receive the "
                    "request of snapshot from InputLogger\n");
            pthread_mutex_lock(pc_arg.mutex);
            fprintf(stdout, "FTMB-Master: the snapshot state(counter) is %d\n", *(pc_arg.counter));
            pthread_mutex_unlock(pc_arg.mutex);
            data = 't';
            write(il_sockfd, &data, 1);
            fprintf(stdout, "TFTMB-Master: aken snapshot "
                    "and send the reply to InputLogger\n");
        }
    }

    close(il_sockfd);

    return 0;
}
