#include "helper.h"
#include <sstream>
#include "client_request.h"
#include <pthread.h>
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
std::ofstream logFile("proxy.log");
std::vector<std::thread> threads;

void* handle_request(void* arg) {
    // Cast the argument to a ClientRequest pointer
    ClientRequest* request = static_cast<ClientRequest*>(arg);

    std::cout << "First line: " << request->first_line << std::endl;
    std::cout << "Method: " << request->method << std::endl;
    std::cout << "Hostname: " << request->hostname << std::endl;
    std::cout << "IP: " << request->ip_address << std::endl;
    std::cout << "Port: " << request->port << std::endl;
    std::cout << "================================" << std::endl;

    // TODO: Implement the request handling logic
    if (request->method == "CONNECT"){
        const string response = "HTTP/1.1 200 Connection Established\r\nProxy-agent: MyProxy/1.0\r\n\r\n";
        send(request->socket_fd, &response,sizeof(response),0);
    
        //cout<<"original: "<<request->socket_fd<<endl;
        
        //if (request->ip_address=="0.0.0.0") continue;
        char recv_msg[3000];
        memset(recv_msg,0,sizeof(recv_msg));
        string port_str=to_string(request->port);
        //int client_fd=create_client(request->ip_address.c_str(), port_str.c_str());
        //send(client_fd, msg, strlen(msg), 0);
        //recv(client_fd,recv_msg,sizeof(recv_msg),0);
        //cout<<recv_msg<<endl;

    }
    // Free the memory allocated for the ClientRequest object
    delete request;

    // Exit the thread
    pthread_exit(NULL);
}


int main() { 
    int proxy_server_fd = create_server("8080");
    int request_id = 0;
    char host[200];
    gethostname(host,sizeof host);
    cout<<host<<endl;

    while(true) {
        string ip_address;
        int new_socket = accept_server(proxy_server_fd, ip_address);

        char msg[65536] = {0} ;
        recv(new_socket, msg, sizeof(msg), 0);


        // Allocate memory for a new ClientRequest object
        pthread_mutex_lock(&lock);
        ClientRequest* request = new ClientRequest();
        // Get ip address 
        request->ip_address = ip_address;

        // Parse the request and store the information in the ClientRequest object
        request->ID = request_id;
        request->socket_fd = new_socket;
        request_id++;
        // cout<<"ip: "<<request->ip_address<<endl;
        parse_request(msg, request->method, request->hostname, request->port, request->first_line);
        pthread_mutex_unlock(&lock);
        // Create a new thread to handle the request
        pthread_t thread;
        pthread_create(&thread, NULL, handle_request, request);
        //handle_request(request);

        close(new_socket);
        //break;
    }
    close(proxy_server_fd);
    return 0;
}

