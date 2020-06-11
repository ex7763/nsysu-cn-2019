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
uint32_t seq_num;
uint16_t rwnd = buffer_size;
mutex rwnd_mutex;
uint32_t cumulative_ack;

mutex cumulative_ack_mutex;
        
volatile 

bool is_tcp_header_empty(){
    lock_guard<std::mutex> lg(tcp_queue_mutex);

    if(tcp_queue.empty()){
        return true;
    }

    else{
        return false;        
    }
}

tcp_header_t get_tcp_header(){
    lock_guard<std::mutex> lg(tcp_queue_mutex);

    if(tcp_queue.empty()){
        error_msg("tcp_queue empty");
    }
    tcp_header_t tmp = tcp_queue.front();
    tcp_queue.pop();
    
    return tmp;
}

void push_tcp_header(tcp_header_t t){
    lock_guard<std::mutex> lg(tcp_queue_mutex);

    tcp_queue.push(t);
    
}

void recv_package_thread(int sockfd, struct sockaddr *sock_addr, string out_file_name){
    uint8_t buf[buffer_size] = {0};
    uint32_t buf_left = 0;
    uint32_t data_len;

    fstream out;
    out.open(out_file_name, ios::out | ios::binary);

    while(1){
        tcp_header_t recv_header = {0};

        rwnd_mutex.lock();
        rwnd = buffer_size - buf_left;
        rwnd_mutex.unlock();
        data_len = recv_tcp_package(sockfd, sock_addr, &recv_header, &buf[buf_left], rwnd);

        
        // if server get finish package
        if(recv_header.fin == 1){
            push_tcp_header(recv_header);

            cout << "=====Recv all file=====\n";
            break;
        }
     
        
        /* simulate loss data */
        if(rand()%100 < gLOSS_RATE){
            cout << "\033[1;35m=====Randomly loss above package=====\033[0m\n";
            continue;
        }

        /* bad package */
        if(data_len == 0){
            printf("\033[1;31m*****bad package*****\033[0m\n");
            continue;
        }

        /* loss detect */
        cumulative_ack_mutex.lock();
        if(recv_header.seq_num > cumulative_ack){
            printf("\033[1;31m*****out of cumulative ack*****\033[0m\n");
            printf("\033[1;31m*****loss package*****\033[0m\n");
            tcp_header_t send_header = {0};

            seq_num_mutex.lock(); /* lock */
            send_header.seq_num = seq_num;
            send_header.ack_num = cumulative_ack;
            send_header.win = rwnd;

            send_tcp_package(sockfd, sock_addr, send_header, NULL, 0);
            seq_num_mutex.unlock(); /* unlock */

            while(not is_tcp_header_empty()){
                get_tcp_header();
            }

            cumulative_ack_mutex.unlock();
            this_thread::sleep_for(chrono::microseconds(5 * rtt * 1000));
            continue;            
        }
        else if(recv_header.seq_num < cumulative_ack){
            cumulative_ack_mutex.unlock();
            // ignore
        }
        else{
            cumulative_ack = recv_header.seq_num + data_len;
            cumulative_ack_mutex.unlock(); /* unlock */


            /* add new data */
            buf_left += data_len;
            
            /* Wriet buffer to file */
            rwnd_mutex.lock(); /* lock */
            rwnd = buffer_size - buf_left;

            // Save data to buf
            if(rwnd < buffer_size/4){
                //out.write((char *)buf, buffer_size - rwnd);
                out.write((char *)buf, buf_left);

                // clear buffer
                rwnd = buffer_size;
                buf_left = 0;
            }
            rwnd_mutex.unlock();
            /* Wriet buffer to file end*/
       

#ifdef DEBUG
            printf("==%x==", buf[buf_left]);
#endif

            /* recv last package */
            /* finish connect */
            if(recv_header.ack_num == 0){
                /* receive all data */
#ifdef DEBUG
                cout << "Read fin\n";
#endif
                rwnd = buffer_size - buf_left;
                /* clear buf */
                rwnd_mutex.lock();
                // Save data to buf
                out.write((char *)buf, buf_left);
                // clear buffer
                rwnd = buffer_size;
                buf_left = 0;
                rwnd_mutex.unlock();

                // let server know you get all data
                send_fin_package(sockfd, sock_addr);
            }
        
            seq_num_mutex.lock(); /* lock */
            // Request next data
            seq_num_queue.push(recv_header.seq_num + data_len);
            
            seq_num_mutex.unlock(); /* unlock */

            push_tcp_header(recv_header);
        }

        this_thread::sleep_for(0.005s);
    }
    out.close();
    cout << "=====Recv thread end=====\n";
}

void send_package_thread(int sockfd, struct sockaddr *sock_addr){
            
    while(1){
        // for delay ack
        this_thread::sleep_for(0.5s);
        
        tcp_header_t send_header = {0};

        if(not is_tcp_header_empty()){
            while(not is_tcp_header_empty()){
                tcp_header_t recv_header = get_tcp_header();
            
                /* finish control */
                if(recv_header.fin == 1){
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


                send_header.seq_num = seq_num;
                send_header.ack_num = ack_num;
                
                send_header.win = rwnd;
            
                seq_num_mutex.unlock();
            }
        

        
            send_tcp_package(sockfd, sock_addr, send_header, NULL, 0);
        }
        
        this_thread::sleep_for(chrono::microseconds(2 * rtt * 1000));
        // timer dectect loss
        if(is_tcp_header_empty()){
            this_thread::sleep_for(chrono::microseconds(10 * rtt * 1000));
            if(is_tcp_header_empty()){
                printf("\033[1;31m*****time-out*****\033[0m\n");
                printf("\033[1;31m*****loss package*****\033[0m\n");
                
                
                send_header.seq_num = seq_num;
                
                send_header.ack_num = cumulative_ack;
                send_header.win = rwnd;
                
                send_tcp_package(sockfd, sock_addr, send_header, NULL, 0);
                this_thread::sleep_for(chrono::microseconds(10 * rtt * 1000));
            }
            else{
                #ifdef DEBUG
                printf("no time out\n");
                #endif
            }
        }
        
    send_package_thread_end:;
    }
}

string get_file_from_server(int sockfd, struct sockaddr *sock_addr, string out_file_name, uint32_t file_num){
    seq_num = client_three_way_handshake(sockfd, sock_addr);
    cumulative_ack = 1;

    tcp_header_t send_header = {0};
    send_header.opt = file_num;
    send_header.seq_num = seq_num;
    send_header.ack_num = cumulative_ack;
    send_header.win = rwnd;
    send_tcp_package(sockfd, sock_addr, send_header, NULL, 0);

    thread recv0(recv_package_thread, sockfd, sock_addr, out_file_name);
    thread send0(send_package_thread, sockfd, sock_addr);

    string str;
    while(1){
        sleep(100);
    }

    return str;
}

int main(int argc, char *argv[]){
    //unsigned char *buf = (unsigned char *)malloc(sizeof(unsigned char) * buffer_size);
    int sockfd = 0;
    string out_file_name;

    if(argc == 3){
        out_file_name = "test.mp4";
    }
    else if(argc == 4){
        out_file_name = string(argv[3]);
    }
    else{
        error_msg("Wrong argument number");
    }
        

    /* create socket */
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        error_msg("socket create");
    }

    int _port;
    sscanf(argv[1], "%d", &_port);

    struct sockaddr_in serverInfo;
    bzero(&serverInfo, sizeof(serverInfo));

    /* write server information */
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = htonl(INADDR_ANY); // all ip address can connect
    serverInfo.sin_port = htons(_port); // use port 10101


    print_parameter(rtt, mss, threshold, buffer_size, "127.0.0.1", _port);


    int file_num;
    sscanf(argv[2], "%d", &file_num);
    get_file_from_server(sockfd, (struct sockaddr *)&serverInfo, out_file_name, file_num);

    
    while(1){
        sleep(100);
    }
 
    
    close(sockfd);
    
}
