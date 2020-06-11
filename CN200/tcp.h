#ifndef CN_TCP
#define CN_TCP
#include "param.h"
#include <stdint.h>

typedef union{
    uint8_t b[24];
    struct{
        uint16_t src_port;
        uint16_t dst_port;

        uint32_t seq_num;
        uint32_t ack_num;

        struct{
            uint8_t hlen:4;
            uint8_t reserved:6;
            uint8_t urg:1;
            uint8_t ack:1;
            uint8_t psh:1;
            uint8_t rst:1;
            uint8_t syn:1;
            uint8_t fin:1;
        };
        uint16_t win;

        uint16_t checksum;
        uint16_t urg_ptr;

        uint32_t opt;
    };
} tcp_header_t;


extern int send_tcp_package(int sockfd, struct sockaddr *socket, tcp_header_t header, uint8_t *data, uint32_t data_size);
extern uint32_t recv_tcp_package(int sockfd, struct sockaddr *socket, tcp_header_t *header, uint8_t *data, uint32_t data_size);
extern void print_tcp_package(uint8_t *data, uint32_t data_len);

extern int server_three_way_handshake(int sockfd, struct sockaddr *socket, int port);
extern int client_three_way_handshake(int sockfd, struct sockaddr *socket, int *port);
#endif
