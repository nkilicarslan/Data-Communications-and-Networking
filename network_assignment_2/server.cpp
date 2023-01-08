#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include "threads.h"

#define MYPORT "4950"    // the port users will be connecting to

#define MAXBUFLEN 100

// both server and client behaving both receiver and sender so the code is very similar
int main(void)
{


    // here is the part of sending for client part
    // clear the hints struct
    memset(&hints, 0, sizeof hints);
    // set the family to IPv4
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    // set the socket type to datagram
    hints.ai_socktype = SOCK_DGRAM;
    // get the address info
    if ((rv = getaddrinfo("172.24.0.20", MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }
    // if p is null, then we didn't get a socket
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    // here is the part of receiving for client part
    //create a socket for listening
    // clear the hints struct
    memset(&hints2, 0, sizeof hints2);
    // set the family to IPv4
    hints2.ai_family = AF_INET; // set to AF_INET to use IPv4
    // set the socket type to datagram
    hints2.ai_socktype = SOCK_DGRAM;
    // set the flags to AI_PASSIVE so we can use our IP
    hints2.ai_flags = AI_PASSIVE; // use my IP
    // get the address info
    if ((rv2 = getaddrinfo(NULL, MYPORT, &hints2, &servinfo2)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv2));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p2 = servinfo2; p2 != NULL; p2 = p2->ai_next) {
        if ((sockfd2 = socket(p2->ai_family, p2->ai_socktype,
                              p2->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd2, p2->ai_addr, p2->ai_addrlen) == -1) {
            close(sockfd2);
            perror("listener: bind");
            continue;
        }

        break;
    }

    // if p is null, then we didn't get a socket
    if (p2 == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    // create a thread for taking input from user
    thread thread_for_input(take_the_input);
    thread_for_input.detach();
    // create threads for sending and receiving messages
    std::thread thread_for_recieve(recieve_the_msg);
    std::thread thread_for_send(send_the_msg);
    thread_for_recieve.join();
    thread_for_send.join();


    freeaddrinfo(servinfo);
    close(sockfd);

    return 0;
}