#include <iostream>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <boost/regex.hpp>
#include <regex>
#include <sstream>
#include <pthread.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <pthread.h>
#include <sstream>
#include <fstream>
#include "client_request.h"
#include <iomanip>


std::ofstream logFile("/var/log/erss/proxy.log");
pthread_mutex_t plock = PTHREAD_MUTEX_INITIALIZER;


// Define a struct to represent the cached response
struct CachedResponse {
    std::string response;
    std::time_t expiration_time;
    std::time_t request_expire;
    std::string ETag;
    std::string Last_Modified;
    int max_age; 
    bool must_revalidate;
    bool no_cache;
    bool no_store; 
    bool is_private;
    int ID;
    bool is_chunk;
};

// Define a cache as an unordered map from URI to CachedResponse
std::unordered_map<std::string, CachedResponse> cache;

bool compare_time(CachedResponse & cached_response){
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now(); 
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    //cout<<"before comparing: "<<std::asctime(std::gmtime(&time))<<endl;
    std::tm* utc_tm = std::gmtime(&time);
    std::time_t current_time = std::mktime(utc_tm);
    //cout<<"after comparing: "<<std::asctime(std::gmtime(&current_time))<<endl;
    //cout<<"current expiration time: "<<std::asctime(std::gmtime(&current_time));
    if (cached_response.expiration_time <= current_time){
        pthread_mutex_lock(&plock);
        logFile<<cached_response.ID<<": "<<"in cache, but expired at "<<std::asctime(std::gmtime(&cached_response.expiration_time));
        pthread_mutex_unlock(&plock);
        return true;             
    }
    else{
        pthread_mutex_lock(&plock);
        logFile<<cached_response.ID<<": "<<"in cache, valid"<<std::endl;
        pthread_mutex_unlock(&plock); 
        return false;           
    }     
}
// Function to check if a cached response is expired
bool should_revalidate(CachedResponse & cached_response, int max_stale) {
    //must not use stored response without successful validation.
    if (cached_response.no_cache) {
        pthread_mutex_lock(&plock);
        logFile<<cached_response.ID<<": "<<"in cache, requires validation"<<std::endl;
        pthread_mutex_unlock(&plock);
        return true;
    }
    
    if(cached_response.expiration_time==std::time_t(0)){//implies there are both no Expires and no max-age
        pthread_mutex_lock(&plock);
        logFile<<cached_response.ID<<": "<<"in cache, valid"<<std::endl;
        pthread_mutex_unlock(&plock);            
        return false;//then we don't need to revalidate
    }
    else{
        if(cached_response.must_revalidate){//do not need to care for max-stale
            return compare_time(cached_response);            
        }
        else{
            if(max_stale==-1){//implies there is no max_stale
                return compare_time(cached_response); 
            }
            else{//implies there is max_stale
                cached_response.expiration_time+=max_stale;
                return compare_time(cached_response); 
            }                
        }            
    }
    

    pthread_mutex_lock(&plock);
    logFile<<cached_response.ID<<": "<<"in cache, valid"<<std::endl;
    pthread_mutex_unlock(&plock);
    return false;    
    //std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    //return (cached_response.expiration_time <= current_time) && cached_response.must_revalidate;
}



void send_request(int server_fd, std::string request) {
    int bytes_sent = send(server_fd, request.c_str(), request.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send GET request to server" << std::endl;
        close(server_fd);
        pthread_exit(NULL);
    }
}

std::string receive_response(int server_fd, ClientRequest * client) {
    char buf[BUFSIZ];
    std::string response_str;
    int bytes_received;
    do {
        bytes_received = recv(server_fd, buf, BUFSIZ, 0);
        if (bytes_received < 0) {
            const char* response = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
            send_request(client->socket_fd, response);
            pthread_mutex_lock(&plock);
            logFile << client->ID <<": Responding " << '"'<<"HTTP/1.1 502 Bad Gateway" << '"'<< std::endl;
            pthread_mutex_unlock(&plock);
            break;
        }
        //cout<<"one buf: "<<buf<<endl;
        response_str.append(buf, bytes_received);
    } while (bytes_received > 0);
    // Split the message into lines
    pthread_mutex_lock(&plock);
    std::vector<std::string> lines;
    boost::split(lines, buf, boost::is_any_of("\r\n"));
    logFile << client->ID <<": Receiving " << lines[0] << " from "<< client->hostname <<std::endl;
    pthread_mutex_unlock(&plock);
    return response_str;
}

// bool is_cacheable(const std::string& response_str) {
//     std::istringstream iss(response_str);
//     std::string line;
//     while (std::getline(iss, line)) {
//         // Check if the line contains the Cache-Control header
//         if (line.find("Cache-Control") != std::string::npos) {
//             // Check if the Cache-Control header contains any of the non-cacheable directives
//             if (line.find("no-cache") != std::string::npos ||
//                 line.find("no-store") != std::string::npos ||
//                 line.find("private") != std::string::npos) {
//                 return false;
//             }
//             // Check if the Cache-Control header contains the max-age directive
//             size_t pos = line.find("max-age");
//             if (pos != std::string::npos) {
//                 // Extract the max-age value and check if it's greater than 0
//                 std::string max_age_str = line.substr(pos + 8);
//                 int max_age = std::stoi(max_age_str);
//                 if (max_age <= 0) {
//                     return false;
//                 }
//             }
//             // Check if the Cache-Control header contains the must-revalidate or proxy-revalidate directive
//             if (line.find("must-revalidate") != std::string::npos ) {
//                 return true;  // Need to revalidate the cached response
//             }
//             return true;  // Response is cacheable
//         }
//     }
//     return true;  // No Cache-Control header found, so response is cacheable by default
// }
bool is_cacheable(CachedResponse & cached_response){
    if (cached_response.no_store == true){
        pthread_mutex_lock(&plock);
        logFile<<cached_response.ID<<": "<<"not cacheable because "<<"no_store"<<std::endl;
        pthread_mutex_unlock(&plock);         
        return false;
    }
    if(cached_response.is_private == true)   {
        pthread_mutex_lock(&plock);
        logFile<<cached_response.ID<<": "<<"not cacheable because "<<"is private"<<std::endl;
        pthread_mutex_unlock(&plock);          
        return false;
    }
    if(cached_response.ETag.empty() && cached_response.Last_Modified.empty()){
        pthread_mutex_lock(&plock);
        logFile<<cached_response.ID<<": "<<"not cacheable because "<<"no ETag and no Last_Modified"<<std::endl;
        pthread_mutex_unlock(&plock);  
        return false;       
    }
    if(cached_response.is_chunk){//if it is chunked, then it is not cacheble
        pthread_mutex_lock(&plock);
        logFile<<cached_response.ID<<": "<<"not cacheable because "<<"no ETag and no Last_Modified"<<std::endl;
        pthread_mutex_unlock(&plock);        
        return false;
    }
    return true;
}

int extract_status_code(const std::string& response_str) {
    std::istringstream iss(response_str);
    std::string line;
    while (std::getline(iss, line)) {
        // Check if the line contains the HTTP status line
        if (line.find("HTTP/1.") != std::string::npos) {
            // Extract the status code from the HTTP status line
            int status_code = std::stoi(line.substr(9, 3));
            return status_code;
        }
    }
    // If we haven't found the status code, return -1 to indicate an error
    return -1;
}

std::time_t to_utc(const std::string & date_str) {
    // Parse the date string into a tm struct
    //cout<<"the date string is: "<<date_str<<endl;
    std::tm tm; 
    std::stringstream ss(date_str); 
    ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S %Z"); 
    std::time_t t = std::mktime(&tm);
    //cout<<"in utfc where: "<<std::asctime(std::gmtime(tm));
    return t;
}

void parse_cache_control_directives(CachedResponse &cached_response,std::string response_str,int ID) {

    cout << "response string: " << response_str << endl; 
    // Define the regular expressions for ETag and each cache control directive
    const boost::regex etag_regex("ETag: \"(.+)\"");
    const boost::regex last_modified_regex("Last-Modified: (.+)");
    const boost::regex max_age_regex("max-age=(\\d+)");
    const boost::regex must_revalidate_regex("must-revalidate");
    const boost::regex no_cache_regex("no-cache");
    const boost::regex no_store_regex("no-store");
    const boost::regex is_private_regex("private");

    // Initialize the cache control directive flags
    cached_response.response = response_str;
    cached_response.expiration_time= std::time_t(0);
    cached_response.ETag="";
    cached_response.Last_Modified = "";
    cached_response.max_age=-1;
    cached_response.must_revalidate = false;
    cached_response.no_cache = false;
    cached_response.no_store = false;
    cached_response.is_private = false;
    cached_response.ID = ID;
    cached_response.is_chunk = false;
    // Split the response header into lines
    std::vector<std::string> lines;
    boost::split(lines, cached_response.response, boost::is_any_of("\r\n"));

    size_t chunked_pos = response_str.find("Transfer-Encoding: chunked\r\n");
        if (chunked_pos != std::string::npos) {
            // The response is chunked
            cached_response.is_chunk = true;
        } else {
            // The response is not chunked
            cached_response.is_chunk - false;
        }


    // Iterate over each line in the response header
    for (const auto& line : lines) {
        // Check if the line contains a cache control directive, ETag or Last-Modified
        if (boost::istarts_with(line, "Cache-Control: ")) {
            // Extract the cache control directives from the line
            std::string directives = line.substr(15);
            // Check for the max-age directive
            boost::smatch max_age_match;
            if (boost::regex_search(directives, max_age_match, max_age_regex)) {
                cached_response.max_age = std::stoi(max_age_match[1]);
                std::regex date_regex(R"(Date:\s+(.*)\r\n)");
                std::smatch match;
                if (std::regex_search(response_str, match, date_regex)) {
                    // Get the date string from the match
                    std::string date_str = match[1].str();
                    std::time_t utc_time = to_utc(date_str);
                    std::time_t expiration_time_utc = utc_time + cached_response.max_age;
                    cached_response.expiration_time = expiration_time_utc;
                    //cout<<"max age expiration time: "<<std::asctime(std::gmtime(&expiration_time_utc));
                }
            } else {
                size_t expires_pos = response_str.find("Expires: ");
                if (expires_pos != std::string::npos) {
                    // Expires header is present
                    std::string expires_str = response_str.substr(expires_pos + 9);
                    // do something with the expires_str
                    std::time_t expire_utc_time = to_utc(expires_str);
                    cached_response.expiration_time = expire_utc_time;
                    //cout << "THis is the part with expire header" << endl;
                    //std::cout<< "UTC time: " << std::asctime(std::gmtime(&expire_utc_time));
                    //cout << "--------------------------" <<endl;
                }
            }
            
            // Check for the must-revalidate directive
            if (boost::regex_search(directives, must_revalidate_regex)) {
                cached_response.must_revalidate = true;
            }

            // Check for the no-cache directive
            if (boost::regex_search(directives, no_cache_regex)) {
                cached_response.no_cache = true;
            }

            // Check for the no-store directive
            if (boost::regex_search(directives, no_store_regex)) {
                cached_response.no_store = true;
            }

            // Check for the private directive
            if (boost::regex_search(directives, is_private_regex)) {
                cached_response.is_private = true;
            }
        } else if (boost::istarts_with(line, "ETag: ")) {
            // Extract the ETag value from the line
            boost::smatch etag_match;
            if (boost::regex_search(line, etag_match, etag_regex)) {
                cached_response.ETag = etag_match[1];
            }
        } else if (boost::istarts_with(line, "Last-Modified: ")) {
            // Extract the Last-Modified value from the line
            boost::smatch last_modified_match;
            if (boost::regex_search(line, last_modified_match, last_modified_regex)) {
                cached_response.Last_Modified = last_modified_match[1];
            }
        }
    }
}
void printCache(){
    std::cout<<"enter into the cache "<<std::endl;
    for(auto &v :cache){
        std::cout<<"url is: "<<v.first<<" content is as follows: "<<std::endl;

        std::time_t utc_time = v.second.expiration_time;
        std::cout<<"expiration_time: "<< "UTC time: " << std::asctime(std::gmtime(&utc_time));
        // Convert to GMT
        std::tm* gmt_tm = std::gmtime(&utc_time);
        std::time_t gmt_time = std::mktime(gmt_tm);
        std::cout << "GMT time: " << std::asctime(std::gmtime(&gmt_time));

        std::cout<<"ETag: "<<v.second.ETag<<std::endl;
        std::cout<<"Last_Modified: "<<v.second.Last_Modified<<std::endl;
        std::cout<<"max_age: "<<v.second.max_age<<std::endl;
        std::cout<<"must_revalidate: "<<v.second.must_revalidate<<std::endl;
        std::cout<<"no_cache: "<<v.second.no_cache<<std::endl;
        std::cout<<"no_store: "<<v.second.no_store<<std::endl;
        std::cout<<"is_private: "<<v.second.is_private<<std::endl;
        std::cout<<"is_chunk: "<<v.second.is_chunk<<std::endl;
        std::cout<<"content end!"<<std::endl;
        std::cout<<"********************************************"<<std::endl;
    }
    std::cout<<"out of the cache "<<std::endl;
}
bool revalidate(CachedResponse& cached_response, const std::string& request_url, int server_fd, ClientRequest* client) {
    // Check if the cached response has an ETag or Last-Modified header
/*     if (cached_response.ETag.empty() && cached_response.Last_Modified.empty()) {
        // Cannot revalidate without an ETag or Last-Modified header
        pthread_mutex_lock(&plock);
        logFile<<request->ID<<": "<<"in cache, valid"<<endl;
        pthread_mutex_unlock(&plock);
        return false;
    } */
    // Create a new request with the appropriate headers for revalidation
    std::string request;
    if (!cached_response.ETag.empty()) {
        request = "GET " + request_url + " HTTP/1.1\r\n"
                  "If-None-Match: " + cached_response.ETag + "\r\n"
                  "\r\n";
    } else {
        request = "GET " + request_url + " HTTP/1.1\r\n"
                  "If-Modified-Since: " + cached_response.Last_Modified + "\r\n"
                  "\r\n";
    }
    // // Send the request and get the response status code
    // int bytes_sent = send(server_fd, request.c_str(), request.length(), 0);
    // if (bytes_sent < 0) {
    //     std::cerr << "Failed to send GET request to server" << std::endl;
    //     close(server_fd);
    //     pthread_exit(NULL);
    // }
    send_request(server_fd, request);

    // // Receive the response from the server
    // char buffer[BUFSIZ];
    // std::string response_str;
    // int bytes_received;
    // do {
    //     bytes_received = recv(server_fd, buffer, BUFSIZ - 1, 0);
    //     if (bytes_received < 0) {
    //         std::cerr << "Failed to receive response from server" << std::endl;
    //         close(server_fd);
    //         pthread_exit(NULL);
    //     }
    //     buffer[bytes_received] = '\0';
    //     response_str += buffer;
    // } while (bytes_received > 0);
    std::string response_str = receive_response(server_fd, client);

    int status_code = extract_status_code(response_str);

    // Handle the response based on the status code
    if (status_code == 304) {
        // The cached response is still valid
        return true;
    } else if (status_code == 200) {
        // The cached response is invalid, update it with the new response
        //cached_response.response = response_str;
        parse_cache_control_directives(cached_response,response_str,client->ID);
        return true;
    } else {
        // Handle other status codes as necessary
        return false;
    }
}



/*
int main() {
    // Example usage
    std::string uri = "http://example.com/resource";
    std::string response = fetch_from_cache(uri);
    if (response.empty()) {
        // Cache miss, fetch response from server
        response = "Response from server";
        std::time_t expiration_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + std::chrono::minutes(5));
        add_to_cache(uri, response, expiration_time);
    }
    std::cout << "Response: " << response << std::endl;
    return 0;
}
*/

