//
//  scrobble_queue.сpp
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#include "stdafx.h"
#include "scrobble_queue.h"
#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <SDK/filesystem.h>
#include <curl/curl.h>
#include "session_manager.h"

using json = nlohmann::json;

namespace foo_lastfm {

ScrobbleQueue* g_scrobble_queue = nullptr;

ScrobbleQueue::ScrobbleQueue() {
    // Determine path for queue storage
    pfc::string8 profile_path = core_api::get_profile_path();
    pfc::string8 file_path;
    if (filesystem::g_get_native_path(profile_path, file_path)) {
        m_queue_file_path = file_path.c_str();
    } else {
        m_queue_file_path = profile_path.c_str();
        if (m_queue_file_path.find("file://") == 0) {
            m_queue_file_path = m_queue_file_path.substr(7);
        }
    }

    if (!m_queue_file_path.empty() && m_queue_file_path.back() != '/' && m_queue_file_path.back() != '\\') {
        m_queue_file_path += "/";
    }
    m_queue_file_path += "lastfm_scrobble_queue.json";

    // Load existing queue from disk
    load_queue();

    if (cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: Queue initialized, " << m_queue.size() << " tracks loaded";
    }
}

ScrobbleQueue::~ScrobbleQueue() {
    // Save queue to disk on destruction
    save_queue();
}

void ScrobbleQueue::add_track(const LastfmApi::TrackInfo& track) {
    std::lock_guard<std::mutex> lock(m_mutex);
    QueuedTrack queued = from_track_info(track);
    m_queue.push_back(queued);
    save_queue();

    if (cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: Track added to queue (" << m_queue.size() << " total): "
            << track.artist.c_str() << " - " << track.track.c_str();
    }
}

void ScrobbleQueue::process_queue() {
    // Check network availability
    bool online = is_network_available();
    if (!online) {
        if (cfg_debug_enabled.get() && !m_network_was_unavailable) {
            FB2K_console_formatter()
                << "Last.fm: Network unavailable - scrobbling paused ("
                << m_queue.size() << " tracks stored offline). "
                << "New tracks will continue to be queued and saved to disk until connection is restored.";
        }
        m_network_was_unavailable = true;
        return;
    } else if (m_network_was_unavailable) {
        m_network_was_unavailable = false;
        if (cfg_debug_enabled.get()) {
            FB2K_console_formatter()
                << "Last.fm: Network connection restored - resuming queued scrobbles ("
                << m_queue.size() << " tracks pending).";
        }
    }

    if (!g_lastfm_api || !g_lastfm_api->has_saved_session()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
        return;
    }

    if (cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: Processing queue with " << m_queue.size() << " tracks";
    }

    // Process limited number of tracks to avoid blocking
    const int kMaxPerRun = 10;
    int processed = 0;
    std::vector<QueuedTrack> remaining;

    for (auto& queued : m_queue) {
        time_t now = time(nullptr);
        int backoff_seconds = 30 * (1 << std::min(queued.retry_count, 5));

        if (now - queued.last_attempt < backoff_seconds) {
            remaining.push_back(queued);
            continue;
        }

        LastfmApi::TrackInfo track_info = to_track_info(queued);
        bool success = g_lastfm_api->scrobble_track(track_info);

        if (success) {
            FB2K_console_formatter()
                << "Last.fm Scrobbler: Scrobbled successfully - "
                << queued.artist.c_str() << " - " << queued.track.c_str();
            if (cfg_debug_enabled.get()) {
                FB2K_console_formatter()
                    << "Last.fm: Successfully scrobbled from queue: "
                    << queued.artist.c_str() << " - " << queued.track.c_str();
            }
        } else {
            FB2K_console_formatter()
                << "Last.fm Scrobbler: Failed to scrobble - "
                << queued.artist.c_str() << " - " << queued.track.c_str();
            queued.retry_count++;
            queued.last_attempt = now;
            remaining.push_back(queued);

            if (cfg_debug_enabled.get()) {
                FB2K_console_formatter()
                    << "Last.fm: Failed to scrobble from queue (attempt "
                    << queued.retry_count << "): "
                    << queued.artist.c_str() << " - " << queued.track.c_str();
            }
        }
        if (++processed >= kMaxPerRun) {
            if (cfg_debug_enabled.get()) {
                FB2K_console_formatter()
                    << "Last.fm: Processed " << processed << " tracks this cycle - will continue later.";
            }
            remaining.insert(remaining.end(), std::next(&queued), &m_queue.back() + 1);
            break;
        }
    }

    m_queue = remaining;
    save_queue();
}

size_t ScrobbleQueue::get_queue_size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void ScrobbleQueue::clear_queue() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.clear();
    save_queue();
}

void ScrobbleQueue::load_queue() {
    if (cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: Attempting to load queue from: " << m_queue_file_path.c_str();
    }

    // Load queue data from JSON file
    try {
        std::ifstream file(m_queue_file_path);
        if (!file.is_open()) {
            if (cfg_debug_enabled.get()) {
                FB2K_console_formatter()
                    << "Last.fm: Queue file does not exist (first run or empty queue)";
            }
            return;
        }

        json j;
        file >> j;
        m_queue.clear();
        for (const auto& item : j["queue"]) {
            QueuedTrack track;
            track.artist = item.value("artist", "");
            track.track = item.value("track", "");
            track.album = item.value("album", "");
            track.album_artist = item.value("album_artist", "");
            track.duration = item.value("duration", 0);
            track.track_number = item.value("track_number", 0);
            track.timestamp = item.value("timestamp", 0);
            track.retry_count = item.value("retry_count", 0);
            track.last_attempt = item.value("last_attempt", 0);
            m_queue.push_back(track);
        }
        if (cfg_debug_enabled.get()) {
            FB2K_console_formatter()
                << "Last.fm: Successfully loaded " << m_queue.size() << " tracks from queue file";
        }
    } catch (const std::exception& e) {
        FB2K_console_formatter()
            << "Last.fm: Failed to load queue: " << e.what();
    }
}

void ScrobbleQueue::save_queue() {
    if (cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: Attempting to save queue (" << m_queue.size() << " tracks) to: "
            << m_queue_file_path.c_str();
    }

    // Save queue data to JSON file
    try {
        json j;
        j["version"] = 1;
        j["queue"] = json::array();
        for (const auto& track : m_queue) {
            json item;
            item["artist"] = track.artist;
            item["track"] = track.track;
            item["album"] = track.album;
            item["album_artist"] = track.album_artist;
            item["duration"] = track.duration;
            item["track_number"] = track.track_number;
            item["timestamp"] = track.timestamp;
            item["retry_count"] = track.retry_count;
            item["last_attempt"] = track.last_attempt;
            j["queue"].push_back(item);
        }

        std::ofstream file(m_queue_file_path, std::ios::out | std::ios::trunc);
        if (file.is_open()) {
            file << j.dump(2);
            file.flush();
            file.close();
            if (cfg_debug_enabled.get()) {
                FB2K_console_formatter()
                    << "Last.fm: Successfully saved queue to disk";
            }
        } else {
            FB2K_console_formatter()
                << "Last.fm ERROR: Failed to open queue file for writing: "
                << m_queue_file_path.c_str();
        }
    } catch (const std::exception& e) {
        FB2K_console_formatter()
            << "Last.fm: Failed to save queue: " << e.what();
    }
}

QueuedTrack ScrobbleQueue::from_track_info(const LastfmApi::TrackInfo& track) {
    // Convert TrackInfo to QueuedTrack
    QueuedTrack queued;
    queued.artist = track.artist;
    queued.track = track.track;
    queued.album = track.album;
    queued.album_artist = track.album_artist;
    queued.duration = track.duration;
    queued.track_number = track.track_number;
    queued.timestamp = track.timestamp;
    queued.retry_count = 0;
    queued.last_attempt = 0;
    return queued;
}

LastfmApi::TrackInfo ScrobbleQueue::to_track_info(const QueuedTrack& queued) {
    // Convert QueuedTrack to TrackInfo
    LastfmApi::TrackInfo track;
    track.artist = queued.artist;
    track.track = queued.track;
    track.album = queued.album;
    track.album_artist = queued.album_artist;
    track.duration = queued.duration;
    track.track_number = queued.track_number;
    track.timestamp = queued.timestamp;
    return track;
}

bool ScrobbleQueue::is_network_available() {
    // Perform HEAD request to check Last.fm API availability
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    curl_easy_setopt(curl, CURLOPT_URL, "https://ws.audioscrobbler.com/2.0/");
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK);
}

} // namespace foo_lastfm
