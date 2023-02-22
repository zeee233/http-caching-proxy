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
        if (line.find("Cache-Control:") == 0) {
            size_t pos = line.find("no-cache");
            if (pos != std::string::npos) {
                return false;
            }
            pos = line.find("no-store");
            if (pos != std::string::npos) {
                return false;
            }
            pos = line.find("private");
            if (pos != std::string::npos) {
                return false;
            }
            pos = line.find("max-age=");
            if (pos != std::string::npos) {
                std::string age_str = line.substr(pos + 8);
                size_t age_end = age_str.find_first_not_of("0123456789");
                int age = std::stoi(age_str.substr(0, age_end));
                if (age <= 0) {
                    return false;
                }
            }
            pos = line.find("must-revalidate");
            if (pos != std::string::npos) {
                return true;
            }
            pos = line.find("max-stale=");
            if (pos != std::string::npos) {
                std::string stale_str = line.substr(pos + 10);
                size_t stale_end = stale_str.find_first_not_of("0123456789");
                int stale = std::stoi(stale_str.substr(0, stale_end));
                if (stale < 0) {
                    return false;
                }
            }
            pos = line.find("public");
            if (pos != std::string::npos) {
                return true;
            }
        }
    }
    return true;
}



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
