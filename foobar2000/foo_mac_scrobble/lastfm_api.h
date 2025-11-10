//
//  lastfm_api.h
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#pragma once

#include <string>
#include <map>
#include <ctime>
#include <curl/curl.h>

class LastfmApi {
public:
    // Constructor: Initializes Last.fm API instance
    LastfmApi();
    // Destructor: Cleans up resources
    ~LastfmApi();
    
    // Structure to hold track metadata for scrobbling
    struct TrackInfo {
        std::string artist;
        std::string track;
        std::string album;
        std::string album_artist;
        int duration;
        int track_number;
        time_t timestamp;
        
        TrackInfo() : duration(0), track_number(0), timestamp(0) {}
    };
    
    // Authenticates user with a token (synchronous)
    bool authenticate(const std::string& token);
    // Generates URL for user authentication
    std::string get_auth_url() const;
    // Checks if the current session is authenticated
    bool is_authenticated() const;
    // Checks if a valid session exists
    bool has_saved_session() const { return !m_session_key.empty(); }
    // Validates the current session
    bool validate_session();
    // Updates "now playing" status on Last.fm
    bool update_now_playing(const TrackInfo& track);
    // Submits a track for scrobbling
    bool scrobble_track(const TrackInfo& track);
    // Sets API key and secret for authentication
    void set_credentials(const char* api_key, const char* api_secret);
    // Sets session key for authenticated requests
    void set_session_key(const char* session_key);
    // Returns the current session key
    std::string get_session_key() const { return m_session_key; }
    // Authenticates user with a token (asynchronous)
    void authenticate_async(const std::string& token, std::function<void(bool success)> callback);
    // Updates "now playing" status on Last.fm (asynchronous)
    void update_now_playing_async(const TrackInfo& track);
    // Submits a track for scrobbling (asynchronous)
    void scrobble_track_async(const TrackInfo& track);
    
private:
    // API key for Last.fm
    std::string m_api_key;
    // API secret for Last.fm
    std::string m_api_secret;
    // Session key for authenticated requests
    std::string m_session_key;
    // Executes an API request in a background thread
    void execute_async_request(const std::map<std::string, std::string>& params, std::function<void(bool success, const std::string& response)> callback);
    // Base URL for Last.fm API
    static const char* API_URL;
    // Base URL for authentication
    static const char* AUTH_URL;
    // Helper methods
    std::string calculate_signature(const std::map<std::string, std::string>& params) const;
    // URL-encodes a string for API requests
    std::string url_encode(const std::string& value) const;
    // Sends an API request with parameters and stores response
    bool send_api_request(
        const std::map<std::string, std::string>& params,
        std::string& response,
        CURLcode* res_out = nullptr,
        long* http_code_out = nullptr
    );
    // CURL callback to collect response data
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
};
