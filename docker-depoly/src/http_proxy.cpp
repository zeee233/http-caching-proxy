#include "helper.h"

#include <pthread.h>
//pthread_mutex_t plock = PTHREAD_MUTEX_INITIALIZER;

std::vector<std::thread> threads;

//pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

void* handle_request(void* arg) {
    // Cast the argument to a ClientRequest pointer
    ClientRequest* request = static_cast<ClientRequest*>(arg);

    std::cout << "First line: " << request->first_line << std::endl;
    std::cout << "Method: " << request->method << std::endl;
    std::cout << "Hostname: " << request->hostname << std::endl;
    std::cout << "IP: " << request->ip_address << std::endl;
    std::cout << "Port: " << request->port << std::endl;
    std::cout << "max stale: " <<request->max_stale <<std::endl;
    std::cout << "================================" << std::endl;
    
    std::string port_str = std::to_string(request->port);
    int server_fd = create_client(request->hostname.c_str(), port_str.c_str());
    if (server_fd == -1) {
        std::cerr << "Failed to connect to server" << std::endl;
        pthread_exit(NULL);
    }
    // TODO: Implement the request handling logic
    // handle connect request 
    if (request->method == "CONNECT") {
        handle_connect(request, server_fd);
        pthread_mutex_lock(&plock);
        logFile<<request->ID<<": Tunnel closed" << std::endl;
        pthread_mutex_unlock(&plock);
    } else if (request->method == "GET") {
        handle_get(request, server_fd);
    } else if (request->method == "POST"){
        handle_post(request, server_fd);
    }
    close(server_fd);
    // Free the memory allocated for the ClientRequest object
    delete request;
    close(request->socket_fd);
    
    // Exit the thread
    pthread_exit(NULL);
}


int main() { 
    int proxy_server_fd = create_server("12345");//need to change to the port "0"
    int request_id = 0;
    char host[200];
    gethostname(host,sizeof host);
    cout<<host<<endl;

    while(true) {
        string ip_address;
        int new_socket = accept_server(proxy_server_fd, ip_address);

        char msg[65536] = {0} ;
        int bytes_received = recv(new_socket, msg, sizeof(msg), 0);
        if (bytes_received <= 0) {
            // Send a 400 error code to the client
            pthread_mutex_lock(&plock);
            const char* response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            send_request(new_socket, response);
            logFile << request_id <<": Responding " << '"' <<"HTTP/1.1 400 Bad Request" <<'"' << std::endl;
            pthread_mutex_unlock(&plock);
            continue;
        }
        //cout<<"data_size received from client: "<<data_size1<<endl;
        cout<<"==================msg================"<<endl
        <<msg<< endl <<"========================================"<<endl;
        // Allocate memory for a new ClientRequest object
        pthread_mutex_lock(&plock);
        ClientRequest* request = new ClientRequest();
      
        // Get the current time in UTC
        std::time_t now = std::time(nullptr);
        std::tm* utc_time  = std::gmtime(&now);
        
        // Parse the request and store the information in the ClientRequest object
        request->ID = request_id;
        request->socket_fd = new_socket;
        request->ip_address=ip_address;
        request_id++;
        parse_request(msg, request->method, request->hostname, request->port, request->first_line, request->max_stale);
        request->content=msg;
        logFile << request->ID << ": " << '"'<<request->first_line << '"' <<" from " << request->ip_address << " @ "<<std::asctime(utc_time);
        pthread_mutex_unlock(&plock);

        // Create a new thread to handle the request
        pthread_t thread;
        pthread_create(&thread, NULL, handle_request, request);
    }
    close(proxy_server_fd);
    return 0;
}

