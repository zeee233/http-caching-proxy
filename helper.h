#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <ctime>
#include <string>
#include <arpa/inet.h>
#include <thread>
#include <fstream>
#include <vector>
#include <mutex>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


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

int accept_server(int proxy_server_fd) {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int socket_fd_new;
    socket_fd_new = accept(proxy_server_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (socket_fd_new == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;
        return -1;
    } 
    return socket_fd_new;
}

void parse_request(const std::string& msg, std::string& method, std::string& hostname, int& port, std::string& first_line) {
    // Split the message into lines
    std::vector<std::string> lines;
    boost::split(lines, msg, boost::is_any_of("\r\n"));

    // Extract the method and target URL from the first line
    std::vector<std::string> tokens;
    boost::split(tokens, lines[0], boost::is_any_of(" "));
    method = tokens[0];
    std::string url = tokens[1];
    first_line = lines[0];

    // Extract the hostname and port number from the URL
    size_t colon = url.find(':');
    size_t slash = url.find('/');
    if (colon == std::string::npos || colon > slash) {
        // No port number in the URL
        hostname = url.substr(0, slash);
        port = 80;
    } else {
        // Port number in the URL
        hostname = url.substr(0, colon);
        port = boost::lexical_cast<int>(url.substr(colon + 1, slash - colon - 1));
    }
}
