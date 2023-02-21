#include "helper.h"
#include "client_request.h"

std::ofstream logFile("proxy.log");



void handle_connection(int socket_fd) {
    char msg[65536] = {0};
    recv(socket_fd, msg, sizeof(msg), 0);

    // Parse the request and store the information in a ClientRequest object
    ClientRequest request;
    parse_request(msg, request.method, request.hostname, request.port, request.first_line);

    std::cout << "First line: " << request.first_line << std::endl;
    std::cout << "Method: " << request.method << std::endl;
    std::cout << "Hostname: " << request.hostname << std::endl;
    std::cout << "Port: " << request.port << std::endl;
    std::cout << "================================" << std::endl;

    // // Pass the ClientRequest object to the request handling and cache management functions
    // handle_request(request);
    // manage_cache(request);

}

int main() { 
    int proxy_server_fd = create_server("8080");
    int request_id = 0;
    char host[200];
    gethostname(host,sizeof host);
    cout<<host<<endl;


    while (true) {
        int new_socket = accept_server(proxy_server_fd);

        // Create a new thread to handle the incoming request
        std::thread th(handle_connection, new_socket);
        th.detach();
    }
    return 0;
}

