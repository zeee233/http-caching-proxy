#include "helper.h"
#include <sstream>
#include <pthread.h>
pthread_mutex_t plock = PTHREAD_MUTEX_INITIALIZER;
std::ofstream logFile("proxy.log");
std::vector<std::thread> threads;
std::mutex output_mutex;

//pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

void* handle_request(void* arg) {
    // Cast the argument to a ClientRequest pointer
    ClientRequest* request = static_cast<ClientRequest*>(arg);

    std::cout << "First line: " << request->first_line << std::endl;
    std::cout << "Method: " << request->method << std::endl;
    std::cout << "Hostname: " << request->hostname << std::endl;
    std::cout << "IP: " << request->ip_address << std::endl;
    std::cout << "Port: " << request->port << std::endl;
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
    } else if (request->method == "GET") {
        // get uri
        std::string request_uri = get_uri(request);
        //check if int cache and need revalidate 
        if(cache.count(request_uri) != 0) { //requested data exists in cache
            if(is_expired(cache[request_uri])) {
                revalidate(cache[request_uri],request_uri, server_fd);
            } 
            send(request->socket_fd, cache[request_uri].response.c_str(), cache[request_uri].response.length(), 0);
        } else { //requested data not in cache
            // Send the GET request to the server
            cout<<request->first_line <<endl;
            std::string request_str = request->first_line + "\r\n";
            request_str += "Host: " + request->hostname + "\r\n";
            request_str += "User-Agent: MyProxy\r\n";
            request_str += "Connection: close\r\n\r\n";
            send_request(server_fd, request_str);

            // Receive the response from the server
            std::string response_str = receive_response(server_fd);

            //create a cache response 
            CachedResponse cached_response;
            cached_response.response = response_str;
            parse_cache_control_directives(cached_response);

            // check cacheability 
            if(is_cacheable(cached_response)) {
                // Perform FIFO if cache size exceeds 100
                if (cache.size() > 100) {
                    auto oldest_entry = cache.begin();
                    cache.erase(oldest_entry);
                }
                // Add new response to cache
                cache[request_uri] = cached_response;
            }
            send(request->socket_fd, response_str.c_str(), response_str.length(), 0);
        }
    } else if (request->method == "POST"){
        
    }
    close(server_fd);
    // Free the memory allocated for the ClientRequest object
    delete request;
    close(request->socket_fd);
    
    // Exit the thread
    pthread_exit(NULL);
}


int main() { 
    int proxy_server_fd = create_server("8800");//need to change to the port "0"
    int request_id = 0;
    char host[200];
    gethostname(host,sizeof host);
    cout<<host<<endl;

    while(true) {
        string ip_address;
        int new_socket = accept_server(proxy_server_fd, ip_address);

        char msg[65536] = {0} ;
        recv(new_socket, msg, sizeof(msg), 0);
        //cout<<"data_size received from client: "<<data_size1<<endl;
        cout<<msg<<endl;
        // Allocate memory for a new ClientRequest object
        pthread_mutex_lock(&plock);
        ClientRequest* request = new ClientRequest();
        
        // Parse the request and store the information in the ClientRequest object
        request->ID = request_id;
        //request->ip_address = ip_address;
        request->socket_fd = new_socket;
        //int data_size2=send(request->socket_fd,&response,sizeof(response),0);
        request->ip_address=ip_address;
        request_id++;
        parse_request(msg, request->method, request->hostname, request->port, request->first_line);
        pthread_mutex_unlock(&plock);
        // Create a new thread to handle the request
        pthread_t thread;
        pthread_create(&thread, NULL, handle_request, request);
        //handle_request(request);
    }
    close(proxy_server_fd);
    return 0;
}

