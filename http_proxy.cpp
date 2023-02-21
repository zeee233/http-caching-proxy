#include "helper.h"
#include <sstream>
#include <boost/beast.hpp>

using namespace boost::beast;
void parse_request(const char* msg, std::string& method, std::string& path, std::string& version) {
    std::istringstream iss(msg);
    std::getline(iss, method, ' ');
    std::getline(iss, path, ' ');
    std::getline(iss, version, '\r');
    iss.ignore(1);
}

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
        std::string method, hostname;
        int port;
        parse_request(msg, method, hostname, port);

        std::cout << "Method: " << method << std::endl;
        std::cout << "Hostname: " << hostname << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "================================" << std::endl;

    }


    }
    return 0;
}

