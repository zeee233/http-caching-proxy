#include "helper.h"
<<<<<<< HEAD
#include <boost/beast.hpp>
namespace http = boost::beast::http;
=======
#include <sstream>

>>>>>>> d31ccd9355001ffc27c0ec1e563ea0c0b2f98a73


void parse_request(const char* msg, std::string& method, std::string& path, std::string& version) {
    std::istringstream iss(msg);
    std::getline(iss, method, ' ');
    std::getline(iss, path, ' ');
    std::getline(iss, version, '\r');
    iss.ignore(1);
}

std::ofstream logFile("proxy.log");
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
<<<<<<< HEAD
        string request(msg);
        http::request<http::empty_body> req;
        boost::beast::error_code ec;
        boost::beast::string_view message(request);
        http::parse(req, message, ec);
        // Check if parsing was successful
        if (ec) {
            std::cerr << "Error parsing HTTP request: " << ec.message() << std::endl;
            return -1;
        }
        std::cout << "Method: " << req.method_string() << std::endl;
        std::cout << "Target: " << req.target() << std::endl;
        std::cout << "Version: " << req.version() << std::endl;
        // Print the request headers
        for (const auto& header : req) {
            std::cout << header.name_string() << ": " << header.value() << std::endl;
        }

        cout << msg << endl;
        cout << "hello" << endl;
        cout << "test 2 " << endl;
=======
        std::string method, path, version;
        parse_request(msg, method, path, version);
        cout << method << endl << path<< endl<< version<<endl;
>>>>>>> d31ccd9355001ffc27c0ec1e563ea0c0b2f98a73

    }
    return 0;
}

