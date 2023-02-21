#include "helper.h"
#include "client_request.h"

std::ofstream logFile("proxy.log");
std::vector<std::thread> threads;
std::mutex mtx;

// Define a function to process the incoming request
void handle_request(ClientRequest request) {

    std::cout << "First line: " << request.first_line << std::endl;
    std::cout << "Method: " << request.method << std::endl;
    std::cout << "Hostname: " << request.hostname << std::endl;
    std::cout << "Port: " << request.port << std::endl;
    std::cout << "================================" << std::endl;


    // Process the request here
    // You can use the request object to access the method, hostname, port, etc.
    // ...
    std::cout << "Request processed: " << request.first_line << std::endl;
}


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

        // Parse the request and store the information in a ClientRequest object
        ClientRequest request;
        request.socket_fd = new_socket;
        parse_request(msg, request.method, request.hostname, request.port, request.first_line);

        // Create a new thread to handle the request
        //mtx.lock();
        handle_request(request);
        //mtx.unlock();

        close(new_socket);
        //break;
    }
    return 0;
}

