//
// Created by necat on 5.12.2022.
//

#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <iostream>
#include <mutex>
#include <thread>

#define MAXBUFLEN 100
#ifndef NETWORK_HW2_THREADS_H
#define NETWORK_HW2_THREADS_H

#endif //NETWORK_HW2_THREADS_H
using namespace std;
int sockfd, sockfd2;
struct addrinfo hints, *servinfo, *p, hints2, *servinfo2, *p2;
int rv, rv2;
int numbytes, numbytes2;
struct sockaddr_storage their_addr;
#define WINDOW_SIZE 8
char buf[MAXBUFLEN];
socklen_t addr_len;
char s[INET6_ADDRSTRLEN];
int enter_times = 0;
int enter_times2 = 0;
int sequence_number = 0;
mutex mtx;
int vector_index = 0;
int N_windowsize = 8;
int base = 0;
int sender_next_seqnum = 0;
int receiver_next_seqnum = 0;



// senderda aldığın verileri lapur lupur gönder gönderirken de
// nextseq numu arttır eğer next seq num windowa eşit olursa göndermeyi durdur
// sender receive ederken ackin seqine bak eğer beklediğimizse base'i kaydır ack edilmiş demek

// receiver için eğer beklediğim ack gelirse expected seq numu bir arttır
// eğer farklı bir şey geldiyse son acki bir daha gönder

//this packet struct keeps 16 bytes and socket communicates with this struct
struct packet
{
    int ack = 0;
    int seq_number = 0;
    char payload[8] = {0};

};
// this vector keeps the all packets
vector<packet *> packet_vector;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// input threadi ayır bu inputu alıp global vectore pushlayacak, 16ya bölme işi burada içinde
//pake pointer paket struct yaz
// sender vectorden alıp gönderecek
// send vectore eklenir eklenmez yeni paket receivera göndericek

// this function is for sender
int send_the_msg(){
    // enter count is initialized 0 first in order to keep the user how many times pressed enter
    int enter_count = 0;
    while(true){
        // if the user pressed enter 3 times, the program will be terminated
        if(enter_count == 3)    exit(0);
        // if the sender_next_seqnum is equal to the base + window size, the sender will not send any packets until the receiver sends the ack
        if(vector_index < packet_vector.size()){
            if(sender_next_seqnum < base + N_windowsize){
                mtx.lock();
                packet *packet = packet_vector[vector_index];
                vector_index++;
                // if packet payload is empty increase the enter_count
                if(packet->payload[1] == 0){
                    enter_count++;
                }
                mtx.unlock();
                // send the packet to the receiver
                if ((numbytes = sendto(sockfd, (void *)packet, sizeof(struct packet), 0,
                                       p->ai_addr, p->ai_addrlen)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
                mtx.lock();
                sender_next_seqnum++;
                mtx.unlock();
            }
            else    continue;
        }

    }

}

// this function is for receiver
int recieve_the_msg(){
    // enter count is initialized 0 first in order to keep the user how many times pressed enter
    int enter_count = 0;
    while (1){
        // if the user pressed enter 3 times, the program will be terminated
        if(enter_count == 3)    exit(0);
        // receive the packet from the sender
        addr_len = sizeof their_addr;
        struct packet *tmp = new packet();
        if ((numbytes2 = recvfrom(sockfd2, (void *)tmp, sizeof(struct packet), 0,
                                  (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        // if the packet payload is empty increase the enter_count
        if(tmp->payload[1] == 0){
            enter_count++;
        }

        mtx.lock();
        // this is the condition for to check the packet is the expected ACK packet or not
        // if this condition is true increase the base
        if(tmp->ack == 1){
            //printf("ack packet %d is received\n",tmp->seq_number);
            if(tmp->seq_number == base){
                base++;
                //printf("base count is %d", base);
                //print the received message

            }
        }
        mtx.unlock();
        mtx.lock();
        // this is the condition for to check the packet is the DATA packet or ACK packet
        // if this condition hold it means that the incoming packet is the DATA packet
        if(tmp->ack == 0){
            printf("%s", tmp->payload);
            // send the ack to the sender
            if(tmp->seq_number == receiver_next_seqnum){
                // create the ack packet and send it to the sender
                struct packet *pac = new packet;
                pac->ack = 1;
                strcpy(pac->payload, tmp->payload);
                pac->seq_number = tmp->seq_number;
                if ((numbytes = sendto(sockfd, (void *)pac, sizeof(struct packet), 0,
                                       p->ai_addr, p->ai_addrlen)) == -1){
                    perror("talker: sendto");
                    exit(1);
                }
                receiver_next_seqnum++;
            }

        }
        mtx.unlock();





    }

}

//this function is for input thread
int take_the_input(){
    while(true){
        // get the input from the terminal and append the new line char to the end.
        string msg;
        getline(std::cin, msg);
        msg.append("\n");
        // get the length of the input
        int msg_length = msg.length();
        // since the packet contains int ack and int seq_number, the payload size is 8
        // so the input will be divided into 8 byte chunks
        for (int i = 0; i < msg.size(); i += 8) {
            // get the remaining size of the input
            int remain_length = msg_length - i;
            // create the packet
            struct packet *tmp = new packet();
            string temp;
            // if the remaining size is less than 8, copy the remaining size of the input to the packet payload
            if(remain_length<8)    temp = msg.substr(i, remain_length);
            //else copy the 8 byte chunk to the packet payload
            else    temp = msg.substr(i, 8);
            // copy the temp string to the packet payload
            strcpy(tmp->payload, temp.c_str());
            tmp->seq_number = sequence_number;
            sequence_number++;
            tmp->ack = 0;
            mtx.lock();
            packet_vector.push_back(tmp);
            mtx.unlock();
        }

    }
}

