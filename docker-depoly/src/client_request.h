
#include <string>
#include <sstream>

using namespace std;

struct ClientRequest {
    int socket_fd;
    std::string method;
    std::string hostname;
    int port;
    std::string first_line;
    std::string ip_address;
    int ID;
    int max_stale;
    std::string content;
    // Add any other relevant fields
};

std::string get_uri(ClientRequest* request) {
    // Parse the first line to extract the URI
    std::istringstream iss(request->first_line);
    std::string method, uri, version;
    iss >> method >> uri >> version;
    //cout << "************************************************this is URI: " << uri << endl;
    return uri;
}



