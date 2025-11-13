//
//  scrobble_queue.h
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#pragma once

#include "lastfm_api.h"
#include "session_manager.h"

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

namespace foo_lastfm
{

// Structure to hold track data in the scrobble queue
struct QueuedTrack
{
    std::string artist;       // Artist name
    std::string track;        // Track title
    std::string album;        // Album name
    std::string album_artist; // Album artist (if different from artist)
    int duration;             // Track duration in seconds
    int track_number;         // Track number in album
    time_t timestamp;         // Timestamp for scrobble submission
    int retry_count;          // Number of failed scrobble attempts
    time_t last_attempt;      // Timestamp of the last scrobble attempt

    QueuedTrack() : duration(0), track_number(0), timestamp(0), retry_count(0), last_attempt(0) {}
};

class ScrobbleQueue
{
  public:
    // Constructor: Initializes the scrobble queue and loads data from disk
    ScrobbleQueue();
    // Destructor: Saves the queue to disk
    ~ScrobbleQueue();
    // Adds a track to the scrobble queue
    void add_track(const LastfmApi::TrackInfo& track);
    // Processes the queue, attempting to scrobble tracks
    void process_queue();
    // Returns the current size of the scrobble queue
    size_t get_queue_size() const;
    // Clears all tracks from the queue and disk
    void clear_queue();

  private:
    // Queue of tracks pending scrobble
    std::vector<QueuedTrack> m_queue;
    // Mutex for thread-safe queue access
    mutable std::mutex m_mutex;
    // Path to the file storing the scrobble queue
    std::string m_queue_file_path;
    // Loads the queue from disk
    void load_queue();
    // Saves the queue to disk
    void save_queue();
    // Converts TrackInfo to QueuedTrack for queue storage
    QueuedTrack from_track_info(const LastfmApi::TrackInfo& track);
    // Converts QueuedTrack to TrackInfo for scrobbling
    LastfmApi::TrackInfo to_track_info(const QueuedTrack& queued);
    // Checks if network is available for scrobbling
    bool is_network_available();
    // Flag indicating if network was unavailable during last check
    bool m_network_was_unavailable = false;
};

extern ScrobbleQueue* g_scrobble_queue;

} // namespace foo_lastfm
