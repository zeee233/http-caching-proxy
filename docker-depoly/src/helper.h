#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "cache.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <ctime>
#include <string>
#include <arpa/inet.h>
#include <thread>
#include <fstream>
#include <vector>
#include <iomanip>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

using namespace std;

int create_server(const char *port) {
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;
    // const char *port     = "4444";

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    socket_fd = socket(host_info_list->ai_family, 
		host_info_list->ai_socktype, 
		host_info_list->ai_protocol);
    if (socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if 

    status = listen(socket_fd, 100);
    if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl; 
        cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
    } 

    freeaddrinfo(host_info_list);
    return socket_fd;
}

int create_client(const char * hostname, const char * port) {
    struct addrinfo host_info;
    struct addrinfo * host_info_list;
    int socket_fd;
    int status;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(host_info_list->ai_family,
                        host_info_list->ai_socktype,
                        host_info_list->ai_protocol);
    if (socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
    }

    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(host_info_list);
    return socket_fd;
}

int accept_server(int proxy_server_fd, string & ip_address) {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int socket_fd_new;
    socket_fd_new = accept(proxy_server_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (socket_fd_new == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;
        return -1;
    } 

    struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
    ip_address = inet_ntoa(addr->sin_addr);
    return socket_fd_new;
}

void parse_request(const std::string& msg, std::string& method, std::string& hostname, int& port, std::string& first_line, int &max_stale) {
    // Split the message into lines
    std::vector<std::string> lines;
    boost::split(lines, msg, boost::is_any_of("\r\n"));

    // Extract the method and target URL from the first line
    std::vector<std::string> tokens;
    boost::split(tokens, lines[0], boost::is_any_of(" "));
    method = tokens[0];
    first_line = lines[0];

    // Host header 
    std::regex host_regex("Host: ([^\\r\\n]+)");
    std::smatch match;
    if (std::regex_search(msg, match, host_regex)) {
        hostname = match[1].str();
    } 
    size_t pos = hostname.find(":");
    cout << "hostname: " << hostname <<endl;;
    // Check for the "max-stale" directive in the request
    std::regex max_stale_regex("max-stale=([0-9]+)");
    if (std::regex_search(msg, match, max_stale_regex)) {
        cout<<"max stale: "<<endl;
        max_stale = std::stoi(match[1].str());
    } else {
        max_stale = -1;
    }
    // New Version for connect 
    if (method=="CONNECT" ||method == "GET" || method == "POST"){
        // Find the position of the colon separator
        if (pos == std::string::npos) {
            port = 80;
        } else {
            // Extract the host substring
            size_t pos = hostname.find_last_of(":");
            std::string port_str = hostname.substr(pos+1);
            port = std::stoi(port_str);
            hostname = hostname.substr(0, pos);
        }
    }

    // else if(method == "GET" || method == "POST") {
    //     string target="://";
    //     size_t get_pos=url.find("://")+target.length();
    //     size_t get_end_pos=url.find("/", get_pos);
    //     port = 80;
    //     if(method == "GET" ) {
    //         hostname = url.substr(get_pos, get_end_pos - get_pos);
    //     } 
    // }
}
string extract_cache_control_header(const std::string& response) {
    std::string header_name = "Cache-Control:";
    std::size_t header_pos = response.find(header_name);
    if (header_pos == std::string::npos) {
        return "";
    }
    std::size_t end_pos = response.find("\r\n", header_pos);
    if (end_pos == std::string::npos) {
        return "";
    }
    std::string header_value = response.substr(header_pos + header_name.size(), end_pos - (header_pos + header_name.size()));
    return header_value;
}
void handle_connect(ClientRequest * request, int server_fd) {
    // Send a 200 OK response to the client
    const std::string response = "HTTP/1.1 200 Connection established\r\n\r\n";
    int bytes_sent = send(request->socket_fd, response.c_str(), response.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send response to client" << std::endl;
        close(server_fd);
        pthread_exit(NULL);
    }

    // Forward data between the client and the server
    fd_set read_fds;
    int max_fd = std::max(request->socket_fd, server_fd) + 1;
    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(request->socket_fd, &read_fds);
        FD_SET(server_fd, &read_fds);
        int num_ready = select(max_fd, &read_fds, NULL, NULL, NULL);
        if (num_ready == -1) {
            std::cerr << "select() failed" << std::endl;
            break;
        }

        if (FD_ISSET(request->socket_fd, &read_fds)) {
            char buf[BUFSIZ];
            int bytes_received = recv(request->socket_fd, buf, BUFSIZ, 0);
            if (bytes_received <= 0) {
                break;
            } 
            int bytes_sent = send(server_fd, buf, bytes_received, 0);
            if (bytes_sent < 0) {
                break;
            } 
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            char buf[BUFSIZ];
            int bytes_received = recv(server_fd, buf, BUFSIZ, 0);
            if (bytes_received <= 0) {
                break;
            } 
            int bytes_sent = send(request->socket_fd, buf, bytes_received, 0);
            if (bytes_sent < 0) {
                break;
            } 
        }
    }
}

void handle_get(ClientRequest * request, int server_fd) {
    // get uri
    std::string request_uri = get_uri(request);
    //check if request is in cache and needs revalidate 
    if(cache.count(request_uri) != 0) { //requested data exists in cache
        //pthread_mutex_lock(&plock);
        //logFile<<request->ID<<": "
        //pthread_mutex_unlock(&plock);
        cache[request_uri].ID=request->ID;
        if(should_revalidate(cache[request_uri],request->max_stale)) {            
            revalidate(cache[request_uri],request_uri, server_fd, request);
        }
        send_request(request->socket_fd,cache[request_uri].response);
        //send(request->socket_fd, cache[request_uri].response.c_str(), cache[request_uri].response.length(), 0);
    } else { //requested data not in cache
        // Send the GET request to the server
        pthread_mutex_lock(&plock);
        logFile<<request->ID<<": not in cache"<<endl;
        pthread_mutex_unlock(&plock);
        cout<<"if cache.count(request_uri) == 0"<<endl;
        cout<<request->first_line <<endl;
        std::string request_str = request->first_line + "\r\n";
        request_str += "Host: " + request->hostname + "\r\n";
        request_str += "User-Agent: MyProxy\r\n";
        request_str += "Connection: close\r\n\r\n";
        send_request(server_fd, request_str);
        pthread_mutex_lock(&plock);
        logFile<<request->ID<<": Requesting " << '"' <<request->first_line << '"'<<" from " << request->hostname << std::endl;
        pthread_mutex_unlock(&plock);

        // Receive the response from the server
        std::string response_str = receive_response(server_fd, request);
        cout << "(((((((((((((((((())))))))))))))))))"<<endl;
        cout << "response: "<< response_str <<endl;
        

        //create a cache response 
        CachedResponse cached_response;
        //cached_response.response = response_str;
      
        parse_cache_control_directives(cached_response,response_str,request->ID);
        cout << "(((((((((((((((((())))))))))))))))))"<<endl;

        // check cacheability 
        if(is_cacheable(cached_response)) {
            if( cached_response.max_age!=-1){
                pthread_mutex_lock(&plock);
                logFile<<cached_response.ID<<": "<<"cached, expires at "<<std::asctime(std::gmtime(&cached_response.expiration_time));
                pthread_mutex_unlock(&plock);                   
            }
            else{
                pthread_mutex_lock(&plock);
                logFile<<cached_response.ID<<": "<<"cached, but requires re-validation"<<std::endl;
                pthread_mutex_unlock(&plock);                  
            }
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
    printCache();
}

void handle_post(ClientRequest* request, int server_fd) {
    // modify here
    std::string request_str = request->first_line + "\r\n";
    request_str += "Host: " + request->hostname + "\r\n";
    //cout<<"post string: "<<request->content<<endl;
    pthread_mutex_lock(&plock);
    logFile<<request->ID<<": Requesting " << '"'<<request->first_line << '"'<<" from " << request->hostname << std::endl;
    pthread_mutex_unlock(&plock); 
    send_request(server_fd, request->content);
    //cout<<"first step complete"<<endl;
    std::string response_from_server = receive_response(server_fd, request);
    //cout<<"second step complete"<<endl;
    send_request(request->socket_fd, response_from_server);
    //cout<<"third step complete"<<endl;
}


