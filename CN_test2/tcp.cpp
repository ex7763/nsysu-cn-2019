#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>

#include <time.h>

#include "tcp.h"
#include <iostream>
using namespace std;

#define DEBUG_SEND

int client_three_way_handshake(int sockfd, struct sockaddr *socket){
    srand(time(NULL));
        
    tcp_header_t recv_header = {0};
    tcp_header_t send_header = {0};

    printf("=====Start three-way handshake=====\n");
    /* send syn */
    int seq_num = rand() % 10000 + 1;
    send_header.syn = 1;
    send_header.seq_num = seq_num;
    send_tcp_package(sockfd, socket, send_header, NULL, 0);

    /* recv ack/syn */
    recv_tcp_package(sockfd, socket, &recv_header, NULL, 0);
    printf("(ack = %u, syn = %u)\n", recv_header.ack, recv_header.syn);

    /* send ack */
    memset(&send_header, 0, sizeof(send_header));
    send_header.ack = 1;
    send_header.ack_num = recv_header.seq_num + 1;
    seq_num++;
    send_header.seq_num = seq_num;

    send_tcp_package(sockfd, socket, send_header, NULL, 0);
    printf("=====Complete three-way handshake=====\n");
    
    return seq_num+1;
}

int server_three_way_handshake(int sockfd, struct sockaddr *socket){
    tcp_header_t recv_header = {0};
    tcp_header_t send_header = {0};
    uint8_t buf[2048];

    printf("=====Start three-way handshake=====\n");
    /* recv syn */
    recv_tcp_package(sockfd, socket, &recv_header, buf, 2048);
    printf("(ack = %u, syn = %u)\n", recv_header.ack, recv_header.syn);

    /* if it is syn */
    /* send syn/ack */
    if(recv_header.syn == 1){
        send_header.syn = 1;
        send_header.ack = 1;
        send_header.ack_num = recv_header.seq_num + 1;
        send_header.seq_num = rand() % 10000 + 1;

        send_tcp_package(sockfd, socket, send_header, NULL, 0);
    }

    /* recv ack */
    /* check ack_num */
    do{
        recv_tcp_package(sockfd, socket, &recv_header, NULL, 0);
        printf("(ack = %u, syn = %u)\n", recv_header.ack, recv_header.syn);
    }while(!(recv_header.ack_num == send_header.seq_num + 1));

    printf("=====Complete three-way handshake=====\n");
        
    
    return recv_header.seq_num+1;
}

int uint8cpy(uint8_t *dst, uint8_t *src, uint32_t len){
    while(len--){
        *(dst++) = *(src++);
    }
    return 0;
}

int send_tcp_package(int sockfd, struct sockaddr *socket, tcp_header_t header, uint8_t *data, uint32_t data_size){
    socklen_t len = sizeof(*socket);
    
    int package_size = sizeof(tcp_header_t) + sizeof(uint8_t) * data_size;

    /* combine tcp header and data to package */
    uint8_t *tcp_package = (uint8_t *)malloc(package_size);
    uint8cpy(tcp_package, header.b, sizeof(tcp_header_t));
    uint8cpy(tcp_package + sizeof(tcp_header_t), data, data_size);
        
    sendto(sockfd, tcp_package, package_size, 0, socket, len);

    #ifdef DEBUG_SEND
    printf("Send (seq_num = %u, ack_num = %u)\n", header.seq_num, header.ack_num);
    cout << "Send package data size: " <<  data_size << endl;
    #endif
    return 0;
}

int recv_tcp_package(int sockfd, struct sockaddr *socket, tcp_header_t *header, uint8_t *buf, uint32_t buf_size){
    /* wait client data */
    socklen_t len = sizeof(*socket);

    uint8_t data[buffer_size + sizeof(tcp_header_t)];
    uint32_t data_len;

    data_len = recvfrom(sockfd, data, buffer_size, 0, socket, &len);
    data_len = data_len - sizeof(tcp_header_t);

    if(data_len > buf_size)
        printf("Warnning: recv package(%u) bigger than buffer size(%u)!!\n", data_len, buf_size);
    
    /* split tcp heaer and data */
    uint8cpy((*header).b, data, sizeof(tcp_header_t));
    uint8cpy(buf, data+sizeof(tcp_header_t), data_len);

    print_tcp_package(data, data_len);
    
    return data_len;
}

void print_tcp_package(uint8_t *data, uint32_t data_len){
    tcp_header_t header;
    uint8cpy(header.b, data, sizeof(tcp_header_t));

    if(header.fin == 1){
        cout << "\tfin = 1\n";
    }

    printf("\033[1;36mReceive\033[0m a package (rwnd =%u, seq_num = %u, ack_num = %u) data_size = %u\n", header.win, header.seq_num, header.ack_num, data_len);
}

void client_session(){
    /* request data */

    /* get data */

    /* repeat */
}

void server_session(){
    /* handle request */

    /* send data */
}
