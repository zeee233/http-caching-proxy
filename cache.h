#include <iostream>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <boost/regex.hpp>
#include <regex>


// Define a struct to represent the cached response
struct CachedResponse {
    std::string response;
    std::time_t expiration_time;
    std::string ETag;
    std::string Last_Modified;
    int max_age; 
    bool must_revalidate;
    bool no_cache;
    bool no_store; 
    bool is_private;
};

// Define a cache as an unordered map from URI to CachedResponse
std::unordered_map<std::string, CachedResponse> cache;

// Function to check if a cached response is expired
bool is_validate(CachedResponse cached_response) {
    std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return (cached_response.expiration_time <= current_time) && cached_response.must_revalidate;
}

// Function to add a response to the cache
void add_to_cache(std::string uri, std::string response, std::time_t expiration_time) {
    // Create a new CachedResponse and add it to the cache
    CachedResponse cached_response = {response, expiration_time};
    cache[uri] = cached_response;
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
    if (cached_response.no_cache != true ||
        cached_response.no_store != true ||
        cached_response.is_private != true) {
        return false;
    }
    if (cached_response.max_age <= 0) {
        return false;
    }
    if (cached_response.must_revalidate==true){
        return false;
    }
    return true;
}

bool revalidate(CachedResponse& cached_response, const std::string& request_url, int server_fd) {
    // Check if the cached response has an ETag or Last-Modified header
    if (cached_response.ETag.empty() && cached_response.Last_Modified.empty()) {
        // Cannot revalidate without an ETag or Last-Modified header
        return false;
    }
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
    // Send the request and get the response status code
    //int status_code = send_request(request);
    int bytes_sent = send(server_fd, request.c_str(), request.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send GET request to server" << std::endl;
        close(server_fd);
        pthread_exit(NULL);
    }

    // Receive the response from the server
    char buffer[BUFSIZ];
    std::string response_str;
    int bytes_received;
    do {
        bytes_received = recv(server_fd, buffer, BUFSIZ - 1, 0);
        if (bytes_received < 0) {
            std::cerr << "Failed to receive response from server" << std::endl;
            close(server_fd);
            pthread_exit(NULL);
        }
        buffer[bytes_received] = '\0';
        response_str += buffer;
    } while (bytes_received > 0);

    int status_code = extract_status_code(response_str);


    // Handle the response based on the status code
    if (status_code == 304) {
        // The cached response is still valid
        return true;
    } else if (status_code == 200) {
        // The cached response is invalid, update it with the new response
        cached_response.response = response_str;
        parse_cache_control_directives(cached_response);
        return true;
    } else {
        // Handle other status codes as necessary
        return false;
    }
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