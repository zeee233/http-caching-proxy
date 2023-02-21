
#include <string>

struct ClientRequest {
    std::string method;
    std::string hostname;
    int port;
    std::string first_line;
    // Add any other relevant fields
};