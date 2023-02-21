#include "helper.h"
#include <sstream>
#include <boost/beast.hpp>

using namespace boost::beast;


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

        while(true) {
        int new_socket = accept_server(proxy_server_fd);

        char msg[65536] = {0} ;
        recv(new_socket, msg, sizeof(msg), 0);
        std::string method, hostname, first_line;
        int port;
        parse_request(msg, method, hostname, port, first_line);

        std::cout << "First line: " << first_line << std::endl;
        std::cout << "Method: " << method << std::endl;
        std::cout << "Hostname: " << hostname << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "================================" << std::endl;

    }


    }
    return 0;
}

