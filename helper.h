#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "client_request.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <ctime>
#include <string>
#include <arpa/inet.h>
#include <thread>
#include <fstream>
#include <vector>

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
    if (method=="CONNECT"){
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
    else{
    string target="://";
    size_t get_pos=url.find("://")+target.length();
    size_t get_end_pos=url.find("/", get_pos);
    //cout<<"url: "<<url.substr(get_pos, get_end_pos - get_pos)<<endl;
    hostname = url.substr(get_pos, get_end_pos - get_pos);
    port = 80;
    }
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
void connection(ClientRequest * request, int server_fd) {
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



void parse_cache_control_directives(const std::string& cache_control_header, int& max_age, bool& must_revalidate, bool& no_cache, bool& no_store, bool& is_private) {
    // Define regular expressions for matching the cache control directives
    const boost::regex max_age_regex("max-age=(\\d+)");
    const boost::regex max_stale_regex("max-stale(=(\\d+))?");
    const boost::regex must_revalidate_regex("must-revalidate");
    const boost::regex no_cache_regex("no-cache");
    const boost::regex no_store_regex("no-store");
    const boost::regex private_regex("private");
    const boost::regex public_regex("public");

    // Split the cache control header into individual directives
    std::vector<std::string> directives;
    boost::split(directives, cache_control_header, boost::is_any_of(","));

    // Process each directive
    for (const auto& directive : directives) {
        // Match the directive against each regex and take the appropriate action
        boost::smatch match;
        if (boost::regex_match(directive, match, max_age_regex)) {
            // Set the maximum age for the cache entry
            max_age = std::stoi(match[1]);
        } else if (boost::regex_match(directive, max_stale_regex)) {
            // Mark the entry as stale with a maximum staleness value, if present
            // (Note: we don't need to do anything with this information in this example)
        } else if (boost::regex_match(directive, must_revalidate_regex)) {
            // Require revalidation for the cache entry
            must_revalidate = true;
        } else if (boost::regex_match(directive, no_cache_regex)) {
            // Prohibit caching altogether
            no_cache = true;
        } else if (boost::regex_match(directive, no_store_regex)) {
            // Prohibit storing the cache entry in any form
            no_store = true;
        } else if (boost::regex_match(directive, private_regex)) {
            // Mark the cache entry as private (i.e., only cacheable by the user agent)
            is_private = true;
        } else if (boost::regex_match(directive, public_regex)) {
            // Mark the cache entry as public (i.e., cacheable by any cache)
            is_private = false;
        }
    }
}
