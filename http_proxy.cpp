#include "helper.h"
#include <sstream>
#include <boost/beast.hpp>

using namespace boost::beast;
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
        http::request<http::empty_body> req;
        string request_str(msg);
        http::parse(request, request_str.data(), request_str.size());
        /*
        http::request_parser< http::empty_body> parser;
        boost::beast::error_code ec;
        parser.put(boost::asio::buffer(msg),ec);
        http::request<http::empty_body> req;
        req=parser.get();
        ///http::request<http::string_body> req;
        //string req_str(msg);
        //req.parse_request(req_str);
        for (auto const& f: req)
        {
            cout << f.name_string() << ": " << f.value() << endl;
        }
*/
        //cout<<msg<<endl;
        //std::string method, path, version;
        //parse_request(msg, method, path, version);
        //cout << method << endl << path<< endl<< version<<endl;

    }
    return 0;
}

