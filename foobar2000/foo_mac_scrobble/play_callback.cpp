//
//  play_callback.cpp
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#include "config.h"
#include "lastfm_api.h"
#include "scrobble_queue.h"
#include "stdafx.h"

#include <SDK/console.h>
#include <SDK/metadb.h>
#include <SDK/play_callback.h>
#include <SDK/playback_control.h>
#include <cmath> // for std::isfinite

namespace foo_lastfm
{
class scrobble_callback : public play_callback_static
{
  private:
    LastfmApi::TrackInfo m_current_track;
    double m_length = 0;
    double m_threshold = 0;
    bool m_scrobbled = false;

  public:
    unsigned get_flags() override
    {
        return flag_on_playback_new_track | flag_on_playback_time | flag_on_playback_seek | flag_on_playback_stop |
               flag_on_playback_pause;
    }

    void on_playback_new_track(metadb_handle_ptr track) override
    {
        if (!cfg_enabled.get())
            return;

        // Check for a saved session, not an active connection
        if (!g_lastfm_api || !g_lastfm_api->has_saved_session())
            return;

        try
        {
            m_scrobbled = false;
            static_api_ptr_t<playback_control> playback_control;
            if (!playback_control->is_playing())
                return;

            file_info_impl info;
            if (!track->get_info(info))
                return;

            // Validate metadata safety to avoid player crashes
            try
            {
                // Length must be valid
                m_length = info.get_length();
                if (!(m_length > 0.0 && std::isfinite(m_length)))
                {
                    console::print("Last.fm Scrobbler: Track length invalid or missing, skipping.");
                    // Clear current track data to prevent re-scrobbling when switching to streams
                    m_current_track = LastfmApi::TrackInfo();
                    m_scrobbled = false;
                    m_threshold = 0;
                    return;
                }

                // Safely assign metadata with fallback for missing values
                auto safeMeta = [&](const char* key)
                {
                    const char* v = info.meta_get(key, 0);
                    return v ? v : "";
                };

                m_current_track.artist = safeMeta("artist");
                m_current_track.track = safeMeta("title");
                m_current_track.album = safeMeta("album");
                m_current_track.album_artist = safeMeta("album artist");

                // Track number (must validate ptr manually, pfc::string8 won't!)
                const char* tn = info.meta_get("tracknumber", 0);
                if (tn && *tn)
                    m_current_track.track_number = atoi(tn);
                else
                    m_current_track.track_number = 0;

                m_current_track.duration = static_cast<int>(m_length);
                m_current_track.timestamp = time(nullptr); // Initial timestamp for now playing

                if (m_current_track.artist.empty() || m_current_track.track.empty())
                {
                    console::print("Last.fm Scrobbler: Missing metadata — skip to prevent crash.");
                    return;
                }

                if (cfg_debug_enabled.get())
                {
                    console::printf("Last.fm Debug: Artist: %s | Title: %s | Album: %s | Album Artist: %s",
                                    m_current_track.artist.c_str(), m_current_track.track.c_str(),
                                    m_current_track.album.c_str(), m_current_track.album_artist.c_str());
                }
            }
            catch (...)
            {
                console::print("Last.fm Scrobbler: Metadata exception — skipping track.");
                return;
            }

            // Setup scrobbling threshold
            m_threshold = (cfg_scrobble_percent.get() / 100.0) * m_length;
            if (m_threshold > 240)
                m_threshold = 240; // Max 4 minutes
            if (m_length < 30)
                return; // Minimum track length

            m_scrobbled = false;

            // Update now playing asynchronously — non-blocking
            if (g_lastfm_api && g_lastfm_api->has_saved_session())
            {
                g_lastfm_api->update_now_playing_async(m_current_track);
                console::print("Last.fm Scrobbler: Updating now playing...");
            }
        }
        catch (const std::exception& e)
        {
            console::complain("Last.fm Scrobbler", e.what());
        }
    }

    void on_playback_time(double p_time) override
    {
        if (!cfg_enabled.get() || m_scrobbled || p_time < m_threshold)
            return;

        // Don't scrobble if current track data is empty (e.g., after switching to radio streams)
        if (m_current_track.artist.empty() || m_current_track.track.empty())
            return;

        try
        {
            static_api_ptr_t<playback_control> playback_control;
            if (!playback_control->is_playing())
                return;

            // Prepare the track for scrobbling
            auto copy = m_current_track;
            copy.timestamp = time(nullptr) - static_cast<time_t>(p_time);

            if (g_lastfm_api && g_lastfm_api->has_saved_session() && g_scrobble_queue)
            {
                // Add to the queue (to make sure nothing is lost)
                g_scrobble_queue->add_track(copy);

                // Try to process the queue right away
                g_scrobble_queue->process_queue();

                console::print("Last.fm Scrobbler: Track queued for scrobbling");
            }

            m_scrobbled = true;
        }
        catch (const std::exception& e)
        {
            console::complain("Last.fm Scrobbler", e.what());
        }
    }

    void on_playback_seek(double p_time) override
    {
        // If seeked back below threshold, allow re-scrobble
        if (p_time < m_threshold)
        {
            m_scrobbled = false;
        }
    }

    void on_playback_stop(play_control::t_stop_reason p_reason) override
    {
        m_scrobbled = false;
        m_length = 0;
        m_threshold = 0;
        // Clear current track data to prevent stale data from being used
        m_current_track = LastfmApi::TrackInfo();
    }

    void on_playback_pause(bool p_state) override
    {
        // No action needed: playback time doesn't advance while paused
    }

    // Unused callbacks
    void on_playback_starting(play_control::t_track_command p_command, bool p_paused) override {}
    void on_playback_edited(metadb_handle_ptr p_track) override {}
    void on_playback_dynamic_info(const file_info& p_info) override {}
    void on_playback_dynamic_info_track(const file_info& p_info) override {}
    void on_volume_change(float p_new_val) override {}
};

static service_factory_t<scrobble_callback> g_scrobble_callback;
} // namespace foo_lastfm
