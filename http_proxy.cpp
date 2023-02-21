#include "helper.h"
#include <sstream>
#include "client_request.h"
#include <pthread.h>
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
std::ofstream logFile("proxy.log");
std::vector<std::thread> threads;
std::mutex mtx;

// // Define a function to process the incoming request
// void handle_request(ClientRequest request) {

//     std::cout << "First line: " << request.first_line << std::endl;
//     std::cout << "Method: " << request.method << std::endl;
//     std::cout << "Hostname: " << request.hostname << std::endl;
//     std::cout << "Port: " << request.port << std::endl;
//     std::cout << "================================" << std::endl;


//     // Process the request here
//     // You can use the request object to access the method, hostname, port, etc.
//     // ...
//     std::cout << "Request processed: " << request.first_line << std::endl;
// }

void* handle_request(void* arg) {
    // Cast the argument to a ClientRequest pointer
    ClientRequest* request = static_cast<ClientRequest*>(arg);

    std::cout << "First line: " << request->first_line << std::endl;
    std::cout << "Method: " << request->method << std::endl;
    std::cout << "Hostname: " << request->hostname << std::endl;
    std::cout << "Port: " << request->port << std::endl;
    std::cout << "================================" << std::endl;

    // TODO: Implement the request handling logic

    // Free the memory allocated for the ClientRequest object
    delete request;

    // Exit the thread
    pthread_exit(NULL);
}
string getIpaddress(int socket_fd){
    //get ip address
    
    struct sockaddr_storage socket_addr;
    memset(&socket_addr, 0, sizeof(socket_addr));
    socklen_t socket_addr_len;
    getpeername(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    string IP_address;
    struct sockaddr_in * s = (struct sockaddr_in *)&socket_addr;
    IP_address = inet_ntoa(s->sin_addr);
    //request->ip_address=IP_address;
    
    //cout<<"later: "<<socket_fd<<endl;
    cout<<"ip: "<<IP_address<<endl;
    return IP_address;
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

        // Allocate memory for a new ClientRequest object
        ClientRequest* request = new ClientRequest();

        // Parse the request and store the information in the ClientRequest object
        request->socket_fd = new_socket;
        parse_request(msg, request->method, request->hostname, request->port, request->first_line);
        if (request->method == "CONNECT"){
            const string response = "HTTP/1.1 200 Connection Established\r\nProxy-agent: MyProxy/1.0\r\n\r\n";
            //send(new_socket, &response,sizeof(response),0);
            
            // Create a new thread to handle the request
            pthread_t thread;
            pthread_create(&thread, NULL, handle_request, request);
            cout<<"original: "<<request->socket_fd<<endl;
            request->ip_address=getIpaddress(request->socket_fd);

            if (request->ip_address=="0.0.0.0") continue;
            char recv_msg[3000];
            memset(recv_msg,0,sizeof(recv_msg));
            string port_str=to_string(request->port);
            int client_fd=create_client(request->ip_address.c_str(), port_str.c_str());
            send(client_fd, msg, strlen(msg), 0);
            //recv(client_fd,recv_msg,sizeof(recv_msg),0);
            //cout<<recv_msg<<endl;

        }

        close(new_socket);
        //break;
    }
    close(proxy_server_fd);
    return 0;
}

