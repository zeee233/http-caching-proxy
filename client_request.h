
#include <string>

struct ClientRequest {
    int socket_fd;
    std::string method;
    std::string hostname;
    int port;
    std::string first_line;
    std::string ip_address;
    int ID;
    // Add any other relevant fields
};