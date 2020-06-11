#ifndef CN_UNIT
#define CN_UNIT
#include "tcp.h"
#include "param.h"
#include <string>
using namespace std;

#define error_msg(msg) printf("%d Error: %s\n", __LINE__,  msg);\
    exit(-1);


extern void print_parameter(int rtt, int mss, int threshold, int buffer_size,const char *ip, int port);

extern string read_file_to_buffer(int n);

extern void send_fin_package(int sockfd, struct sockaddr *client_addr);
#endif
