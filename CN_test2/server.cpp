#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>

// #include <pthread.h>

#include "param.h"
#include "tcp.h"
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>

#include "unit.h"
using namespace std;

// tcp queue
queue<tcp_header_t> tcp_queue;
mutex tcp_queue_mutex;

uint16_t seq_num;

// thread print
mutex print_mutex;

// file
string f_str1 = read_file_to_buffer(1);
string f_str2 = read_file_to_buffer(2);
string f_str3 = read_file_to_buffer(3);
string f_str4 = read_file_to_buffer(4);

bool is_tcp_header_empty(){
    tcp_queue_mutex.lock();
    if(tcp_queue.empty()){
        tcp_queue_mutex.unlock();
        return true;
    }

    else{
        tcp_queue_mutex.unlock();
        return false;        
    }
}

tcp_header_t get_tcp_header(){
    tcp_queue_mutex.lock();

    if(tcp_queue.empty()){
        error_msg("tcp_queue empty");
    }
    tcp_header_t tmp = tcp_queue.front();
    tcp_queue.pop();
    
    tcp_queue_mutex.unlock();

    return tmp;
}

void push_tcp_header(tcp_header_t t){
    tcp_queue_mutex.lock();

    tcp_queue.push(t);
    
    tcp_queue_mutex.unlock();
}

void recv_package_thread(int sockfd, struct sockaddr *sock_addr){
    uint32_t pre_ack_num = 0;
    int ack_num_count = 0;

    while(1){
        tcp_header_t recv_header = {0};

        recv_tcp_package(sockfd, sock_addr, &recv_header, NULL, 0);

        if(recv_header.fin == 1){
            print_mutex.lock();
            cout << "=====Send all file=====\n";
            cout << "=====Recv thread end=====\n";
            print_mutex.unlock();
            // return;
        }
        else{
            if(pre_ack_num == recv_header.ack_num){
                ack_num_count++;
            }
            else{
                pre_ack_num = recv_header.ack_num;
            }

            // fast retransmit
            if(ack_num_count >= 3){
                printf("\033[1;31m*****three duplicated*****\033[0m\n");
                printf("\033[1;31m*****fast retransmit*****\033[0m\n");
                // clear buffer
                while(not is_tcp_header_empty()){
                    get_tcp_header();
                }
                ack_num_count = 0;
                // tcp_header_t send_header = {0};

                recv_header.opt = 100;
                push_tcp_header(recv_header);
            }
            else{
                push_tcp_header(recv_header);                
            }
        }

        this_thread::sleep_for(0.005s);
    }
}

void send_package_thread(int sockfd, struct sockaddr *sock_addr){
    uint16_t cwnd = 1;
    string *_file_str = NULL;
    
    while(1){
        while(is_tcp_header_empty()){
            this_thread::sleep_for(chrono::microseconds(500));
        }

        tcp_header_t recv_header = get_tcp_header();

        /* fast recovery */
        if(recv_header.opt == 100){
            printf("\033[1;31m*****fast recovery*****\033[0m\n");
            printf("\033[1;34m*****Slow start*****\033[0m\n");
            cwnd = 1;
            // if(cwnd < threshold){
            //     printf("\033[1;34m*****Slow start*****\033[0m\n");
            //     cwnd = 1;
            // }
            // else{
            //     printf("\033[1;34m*****Condestion avoidanace*****\033[0m\n\n");
            //     cwnd = threshold;
            // }
        }

        if(_file_str == NULL){
            switch(recv_header.opt){
            case 1:
                _file_str = &f_str1;
                break;
            case 2:
                _file_str = &f_str2;
                break;
            case 3:
                _file_str = &f_str3;
                break;
            case 4:
                _file_str = &f_str4;
                break;
            default:
                error_msg("Get file fail");
            }
        }
        string &file_str = *_file_str;


        /* SEND DATA */
        // send_header config
        tcp_header_t send_header = {0};
        
        // send data
        uint32_t offset = recv_header.ack_num;

        #ifdef DEBUG
        printf("==%x== %u", (uint8_t)(file_str[offset - 1]), offset);
        #endif
        printf("\tSend a package at : %u\n", recv_header.ack_num);
        if(recv_header.ack_num > file_str.size()){
            printf("Request file out of range\n");
        }
        else if(file_str.size() < offset + cwnd){
            cwnd = (uint16_t)((uint32_t)file_str.size()-offset+1);
            send_header.ack_num = 0;
            send_header.seq_num = recv_header.ack_num - 1; // start position and data length

            send_tcp_package(sockfd, sock_addr, send_header, (uint8_t *)(&file_str[offset-1]), cwnd);

            // let client know that no data need to recive
            send_fin_package(sockfd, sock_addr);

            print_mutex.lock();
            cout << "=====Send thread end=====\n";
            print_mutex.unlock();
            // return;
        }
        else{
            if(cwnd <= mss){
                send_header.ack_num = recv_header.seq_num;
                send_header.seq_num = recv_header.ack_num; // start position and data length
            
                send_tcp_package(sockfd, sock_addr, send_header, (uint8_t *)(&file_str[offset-1]), cwnd);
                this_thread::sleep_for(chrono::microseconds(rtt * 1000));
            }
            else{
                int x=0;
                uint16_t i_offset = 0;
                uint16_t send_size = mss;
                while(i_offset < cwnd){
                    send_header.ack_num = recv_header.seq_num + x++;
                    send_header.seq_num = recv_header.ack_num + i_offset; // start position and data length
                    
                
                    send_tcp_package(sockfd, sock_addr, send_header, (uint8_t *)(&file_str[offset-1 + i_offset]), send_size);
                    //this_thread::sleep_for(chrono::microseconds(rtt * 1000));
                    
                    if(i_offset + send_size > cwnd){
                        send_size = cwnd - i_offset;
                    }
                    else{
                        i_offset += send_size;
                        send_size = mss;
                    }
                }
            }
        }

        printf("cwnd = %u\n", cwnd);

        // congestion control
        // congestion avoid
        if(cwnd >= threshold){
            printf("\033[1;34m*****Condestion avoidanace*****\033[0m\n\n");
            cwnd += mss;
            if(cwnd <= mss)
                cwnd = 0xFFFF;
        }
        // slow start
        else{
            printf("\033[1;34m*****Slow start*****\033[0m\n");
            cwnd *= 2;
        }
        // check rwnd
        if(cwnd > recv_header.win){
            cwnd = recv_header.win;
        }

        this_thread::sleep_for(chrono::microseconds(rtt * 1000));
    }
}

int main(int argc, char *argv[]){
    //unsigned char *buf = (unsigned char *)calloc(buffer_size, sizeof(unsigned char));
    int sockfd = 0;

    if(argc != 2){
        error_msg("Wrong argument number");        
    }

    /* create socket */
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        error_msg("socket create");
    }

    struct sockaddr_in serverInfo;
    bzero(&serverInfo, sizeof(serverInfo));

    /* write server information */
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = htonl(INADDR_ANY); // all ip address can connect
    serverInfo.sin_port = htons(port); // use port 10101

    /* bind to computer */
    if((bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo))) == -1){
        error_msg("bind socket");
    }
    
    int _port;
    sscanf(argv[1], "%d", &_port);
    print_parameter(rtt, mss, threshold, buffer_size, "127.0.0.1", _port);

    struct sockaddr_in client_addr;
    server_three_way_handshake(sockfd, (struct sockaddr *)&client_addr);
    
    thread recv0(recv_package_thread, sockfd, (struct sockaddr *)&client_addr);
    thread send0(send_package_thread, sockfd, (struct sockaddr *)&client_addr);

    while(1){
        sleep(1000);
    }

    close(sockfd);
}
