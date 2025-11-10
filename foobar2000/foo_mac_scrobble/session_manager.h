//
//  session_manager.h
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#pragma once
#include <string>

namespace foo_lastfm {

class SessionManager {
public:
    // Constructor: Initializes session manager
    SessionManager();

    // Destructor: Ensures session data is saved
    ~SessionManager();

    // Loads session data (key and username) from file
    // Returns true if session is successfully loaded
    bool load_session(std::string& session_key, std::string& username);

    // Saves session data (key and username) to file
    // Returns true if session is successfully saved
    void save_session(const std::string& session_key, const std::string& username);

    // Clears session data from file and memory
    void clear_session();

private:
    // Path to the session file
    std::string m_session_file_path;
};

extern SessionManager* g_session_manager;

} // namespace foo_lastfm
