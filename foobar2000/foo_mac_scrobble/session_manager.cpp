//
//  session_manager.cpp
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#include "session_manager.h"
#include "config.h"
#include "safe_log_utils.h"
#include <pfc/string_base.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace foo_lastfm {

SessionManager* g_session_manager = nullptr;

SessionManager::SessionManager() {
    // Initialize session manager and determine file path
    pfc::string8 profileDir = core_api::get_profile_path();

    // Check and remove the "file://" prefix
    const char *prefix = "file://";
    if (strncmp(profileDir.c_str(), prefix, 7) == 0) {
        // Create a new string without the prefix
        pfc::string8 temp;
        temp.add_string(profileDir.c_str() + 7);
        profileDir = temp;
    }

    // Make sure the ending "/" is present
    if (profileDir.length() > 0 && profileDir[profileDir.length() - 1] != '/')
        profileDir.add_char('/');

    // Build the full path to the JSON file
    pfc::string8 sessionPath;
    sessionPath << profileDir << "lastfm_session.json";
    m_session_file_path = sessionPath.get_ptr();

    // Debug Logging
    if (foo_lastfm::cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: SessionManager initialized (path: " << m_session_file_path.c_str() << ")";
        FB2K_console_formatter()
            << "Last.fm: SessionManager ready (file path: " << m_session_file_path.c_str() << ")";
    }
}

SessionManager::~SessionManager() {}

bool SessionManager::load_session(std::string& session_key, std::string& username) {
    // Load session data from JSON file
    std::ifstream f(m_session_file_path);
    if (!f.good()) {
        if (foo_lastfm::cfg_debug_enabled.get()) {
            FB2K_console_formatter()
                << "Last.fm: No existing session file found at "
                << m_session_file_path.c_str();
        }
        return false;
    }

    try {
        json j;
        f >> j;
        f.close();

        if (j.contains("session_key") && j.contains("username")) {
            session_key = j["session_key"].get<std::string>();
            username = j["username"].get<std::string>();

            if (foo_lastfm::cfg_debug_enabled.get()) {
                FB2K_console_formatter()
                    << "Last.fm: Session loaded from disk (username: "
                    << username.c_str() << ")";
            }

            return true;
        } else {
            if (foo_lastfm::cfg_debug_enabled.get()) {
                FB2K_console_formatter()
                    << "Last.fm: Session file found, but missing required fields";
            }
            return false;
        }
    } catch (const std::exception& e) {
        FB2K_console_formatter()
            << "Last.fm ERROR: Failed to parse session file (" << e.what() << ")";
        return false;
    }
}

void SessionManager::save_session(const std::string& session_key,
                                  const std::string& username) {
    // Save session data to JSON file
    json j;
    j["session_key"] = session_key;
    j["username"] = username;

    std::ofstream f(m_session_file_path);
    if (!f.good()) {
        FB2K_console_formatter()
            << "Last.fm ERROR: Cannot open session file for writing: "
            << m_session_file_path.c_str();
        return;
    }

    f << std::setw(4) << j << std::endl;
    f.close();

    if (foo_lastfm::cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: Session saved to disk at "
            << m_session_file_path.c_str();
        FB2K_console_formatter()
            << "Last.fm: Saved session for user: "
            << username.c_str()
            << " (key length: " << (int)session_key.length() << ")";
    }
}

void SessionManager::clear_session() {
    // Clear session data and remove file
    try {
        if (std::filesystem::exists(m_session_file_path)) {
            std::filesystem::remove(m_session_file_path);
            if (foo_lastfm::cfg_debug_enabled.get()) {
                FB2K_console_formatter()
                    << "Last.fm: Session file deleted: "
                    << m_session_file_path.c_str();
            }
        }
    } catch (const std::exception& e) {
        FB2K_console_formatter()
            << "Last.fm ERROR: Failed to delete session file (" << e.what() << ")";
    }
}

} // namespace foo_lastfm
