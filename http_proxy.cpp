#include "helper.h"


std::ofstream logFile("proxy.log");
int main() { 
    int proxy_server_fd = create_server("8080");

    int request_id = 0;
    char host[200];
    gethostname(host,sizeof host);
    cout<<host<<endl;
    while(true) {
        int new_socket = accept_server(proxy_server_fd);

        char msg[65536] = {0} ;
        recv(new_socket, msg, sizeof(msg), 0);
        cout << "test 2 " << endl;
        cour<<"qeqwe"
    }
    return 0;
}

