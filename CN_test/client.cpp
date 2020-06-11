#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>

#include "param.h"
#include "tcp.h"
#include "unit.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
using namespace std;

queue<tcp_header_t> tcp_queue;
mutex tcp_queue_mutex;

queue<uint32_t> seq_num_queue;
mutex seq_num_mutex;

// thread print
mutex print_mutex;

// control
int ack_num;
uint16_t rwnd = buffer_size;
mutex rwnd_mutex;

        
volatile 

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
    uint8_t buf[buffer_size] = {0};
    uint32_t buf_left = 0;
    uint32_t data_len;

    bool finish_flag = false;

    fstream out;
    out.open("test2.mp4", ios::out | ios::binary);

    while(1){
        tcp_header_t recv_header = {0};

        rwnd_mutex.lock();
        rwnd = buffer_size - buf_left;

        // Save data to buf
        if(rwnd < buffer_size/2){
            out.write((char *)buf, buffer_size - rwnd);
            // clear buffer
            rwnd = buffer_size;
            buf_left = 0;
        }
        rwnd_mutex.unlock();

        data_len = recv_tcp_package(sockfd, sock_addr, &recv_header, &buf[buf_left], rwnd);
        #ifdef DEBUG
        printf("==%x==", buf[buf_left]);
        #endif
        buf_left += data_len;
        

        
        seq_num_mutex.lock(); /* lock */
        // Request next data
        seq_num_queue.push(recv_header.seq_num + data_len);

        push_tcp_header(recv_header);
            
        seq_num_mutex.unlock(); /* unlock */


        if(recv_header.fin == 1)
            finish_flag = true;
        /* receive all data */
        if(finish_flag){
            #ifdef DEBUG
            cout << "Read fin\n";
            #endif
            
            /* clear buf */
            rwnd_mutex.lock();
            // Save data to buf
            
            out.write((char *)buf, buffer_size - rwnd);
            // clear buffer
            rwnd = buffer_size;
            buf_left = 0;
                
            rwnd_mutex.unlock();

            cout << "=====Recv all file=====\n";
            cout << "=====Recv thread end=====\n";
            break;
        }

        this_thread::sleep_for(0.05s);
    }
    out.close();
}

void send_package_thread(int sockfd, struct sockaddr *sock_addr){
        
    while(1){
        tcp_header_t send_header = {0};

        while(is_tcp_header_empty()){
            this_thread::sleep_for(0.5s);
        }

        int seq_num;
        while(not is_tcp_header_empty()){
            tcp_header_t recv_header = get_tcp_header();
            
            /* finish control */
            if(recv_header.fin == 1){
                send_fin_package(sockfd, sock_addr);
                cout << "=====Send thread end=====\n";
                return;
            }
            if(recv_header.ack_num == 0)
                goto send_package_thread_end;
            /* finish control end */

            seq_num = recv_header.ack_num + 1;
            
            seq_num_mutex.lock();
            // Request next data
            
            
            ack_num = seq_num_queue.front();
            
            seq_num_queue.pop();
            
            seq_num_mutex.unlock();
        }
        
        send_header.seq_num = seq_num;
        send_header.ack_num = ack_num;
        send_header.win = rwnd;
        
        send_tcp_package(sockfd, sock_addr, send_header, NULL, 0);

    send_package_thread_end:
        this_thread::sleep_for(chrono::microseconds(rtt));
    }
}

string get_file_from_server(int sockfd, struct sockaddr *sock_addr, int file){
    uint32_t seq_num = client_three_way_handshake(sockfd, sock_addr);


    tcp_header_t send_header = {0};
    send_header.seq_num = seq_num;
    send_header.ack_num = 1;
    send_header.win = rwnd;
    send_tcp_package(sockfd, sock_addr, send_header, NULL, 0);

    thread recv0(recv_package_thread, sockfd, sock_addr);
    thread send0(send_package_thread, sockfd, sock_addr);

    string str;
    while(1){
        sleep(100);
    }

    return str;
}

int main(){
    //unsigned char *buf = (unsigned char *)malloc(sizeof(unsigned char) * buffer_size);
    int sockfd = 0;


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

    print_parameter(rtt, mss, threshold, buffer_size, "127.0.0.1", port);


    string str = get_file_from_server(sockfd, (struct sockaddr *)&serverInfo, 1);

    
    while(1){
        sleep(100);
    }
 
    
    close(sockfd);
    
}
