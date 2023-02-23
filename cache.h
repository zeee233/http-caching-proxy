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
    string ETag;
    int max_age, 
    bool must_revalidate, 
    bool no_cache, 
    bool no_store, 
    bool is_private
};

// Define a cache as an unordered map from URI to CachedResponse
std::unordered_map<std::string, CachedResponse> cache;

// Function to check if a cached response is expired
bool is_expired(CachedResponse cached_response) {
    std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return cached_response.expiration_time <= current_time;
}

// Function to fetch a response from the cache
std::string fetch_from_cache(std::string uri) {
    if (cache.count(uri) == 0) {
        // Cache miss
        return "";
    } else if (is_expired(cache[uri])) {
        // Cache hit, but expired
        cache.erase(uri);
        return "";
    } else {
        // Cache hit
        return cache[uri].response;
    }
}

// Function to add a response to the cache
void add_to_cache(std::string uri, std::string response, std::time_t expiration_time) {
    // Create a new CachedResponse and add it to the cache
    CachedResponse cached_response = {response, expiration_time};
    cache[uri] = cached_response;
}

bool is_cacheable(const std::string& response_str) {
    std::istringstream iss(response_str);
    std::string line;
    while (std::getline(iss, line)) {
        // Check if the line contains the Cache-Control header
        if (line.find("Cache-Control") != std::string::npos) {
            // Check if the Cache-Control header contains any of the non-cacheable directives
            if (line.find("no-cache") != std::string::npos ||
                line.find("no-store") != std::string::npos ||
                line.find("private") != std::string::npos) {
                return false;
            }
            // Check if the Cache-Control header contains the max-age directive
            size_t pos = line.find("max-age");
            if (pos != std::string::npos) {
                // Extract the max-age value and check if it's greater than 0
                std::string max_age_str = line.substr(pos + 8);
                int max_age = std::stoi(max_age_str);
                if (max_age <= 0) {
                    return false;
                }
            }
            // Check if the Cache-Control header contains the must-revalidate or proxy-revalidate directive
            if (line.find("must-revalidate") != std::string::npos ) {
                return true;  // Need to revalidate the cached response
            }
            return true;  // Response is cacheable
        }
    }
    return true;  // No Cache-Control header found, so response is cacheable by default
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