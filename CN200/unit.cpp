#include <iostream>
#include <string>
#include <fstream>

#include <stdlib.h>
#include <time.h>

#include "tcp.h"
#include "unit.h"
using namespace std;

void print_parameter(int rtt, int mss, int threshold, int buffer_size,const char *ip, int port){
    puts("=====Parameter=====");
    printf("The RTT delay = %d ms\n", rtt);
    printf("The threshold = %d ms\n", threshold);
    printf("The MSS = %d ms\n", mss);
    printf("The buffer size = %d ms\n", buffer_size);
    printf("Server's IP is %s\n", ip);
    printf("Server is listening on port %d\n", port);
    puts("==================");

    //    printf("%s", inet_pton(serverInfo.sin_addr.s_addr));
}


string read_file_to_buffer(int n){
    fstream file;
    char str[100] = {0};
    sprintf(str, "./file/%d.mp4", n);
    file.open(str, ios::in | ios::binary);
    
    file.seekg(0, ios::end);
    streampos size = file.tellg();
    string data(size ,'\0');
    file.seekg(0, ios::beg);
    file.read(&data[0], size);
    file.close();

    return data;
}



void send_fin_package(int sockfd, struct sockaddr *client_addr){
    tcp_header_t send_header = {0};
    send_header.fin = 1;
    send_tcp_package(sockfd, client_addr, send_header,NULL, 0);
    // cout << "=====Send all file=====\n";
}
