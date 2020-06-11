void recv_package_thread(int sockfd){
    struct sockaddr_in client_addr;

    while(1){
        tcp_header_t recv_header = {0};
        
        recv_tcp_package(sockfd, (struct sockaddr *)&client_addr, &recv_header, NULL, 0);

        push_tcp_header(recv_header);
        this_thread::sleep_for(0.1s);
    }
    /* wait for another client to connect */
    server_three_way_handshake(sockfd, (struct sockaddr *)&client_addr);
}

void send_package_thread(int sockfd){
    struct sockaddr_in client_addr;

    while(1){
        tcp_header_t send_header = {0};
        
        send_tcp_package(sockfd, (struct sockaddr *)&client_addr, send_header, NULL, 0);

        this_thread::sleep_for(0.1s);
    }
    /* wait for another client to connect */
    server_three_way_handshake(sockfd, (struct sockaddr *)&client_addr);
}
