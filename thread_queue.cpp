#include <thread>
#include <mutex>
using namespace std;

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
