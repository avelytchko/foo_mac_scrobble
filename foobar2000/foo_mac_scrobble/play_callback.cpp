//
//  play_callback.cpp
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#include "stdafx.h"
#include "lastfm_api.h"
#include "config.h"
#include <SDK/play_callback.h>
#include <SDK/metadb.h>
#include <SDK/playback_control.h>
#include <SDK/console.h>
#include "scrobble_queue.h"

namespace foo_lastfm {
    class scrobble_callback : public play_callback_static {
    private:
        LastfmApi::TrackInfo m_current_track;
        double m_length = 0;
        double m_threshold = 0;
        bool m_scrobbled = false;

    public:
        unsigned get_flags() override {
            return flag_on_playback_new_track | flag_on_playback_time |
                   flag_on_playback_seek | flag_on_playback_stop |
                   flag_on_playback_pause;
        }

        void on_playback_new_track(metadb_handle_ptr track) override {
            if (!cfg_enabled.get()) return;

            // Check for a saved session, not an active connection
            if (!g_lastfm_api || !g_lastfm_api->has_saved_session()) return;

            try {
                static_api_ptr_t<playback_control> playback_control;
                if (!playback_control->is_playing()) return;

                file_info_impl info;
                if (!track->get_info(info)) return;

                m_current_track.artist = info.meta_get("artist", 0);
                m_current_track.track = info.meta_get("title", 0);
                m_current_track.album = info.meta_get("album", 0);
                m_current_track.album_artist = info.meta_get("album artist", 0);
                m_length = info.get_length();
                m_current_track.duration = static_cast<int>(m_length);
                pfc::string8 track_number = info.meta_get("tracknumber", 0);
                m_current_track.track_number = track_number.is_empty() ? 0 : atoi(track_number);
                m_current_track.timestamp = time(nullptr);  // Initial timestamp for now playing

                if (m_current_track.artist.empty() || m_current_track.track.empty()) {
                    console::print("Last.fm Scrobbler: Skipping track due to missing metadata");
                    return;
                }

                m_threshold = (cfg_scrobble_percent.get() / 100.0) * m_length;
                if (m_threshold > 240) m_threshold = 240;  // Max 4 minutes
                if (m_length < 30) return;  // Minimum track length

                m_scrobbled = false;

                if (g_lastfm_api && g_lastfm_api->has_saved_session()) {
                    // Asynchronous call - does not block the UI
                    g_lastfm_api->update_now_playing_async(m_current_track);
                    console::print("Last.fm Scrobbler: Updating now playing...");
                }
            } catch (const std::exception& e) {
                console::complain("Last.fm Scrobbler", e.what());
            }
        }

        void on_playback_time(double p_time) override {
            if (!cfg_enabled.get() || m_scrobbled || p_time < m_threshold) return;

            try {
                static_api_ptr_t<playback_control> playback_control;
                if (!playback_control->is_playing()) return;

                // Prepare the track for scrobbling
                auto copy = m_current_track;
                copy.timestamp = time(nullptr) - static_cast<time_t>(p_time);

                if (g_lastfm_api && g_lastfm_api->has_saved_session() && g_scrobble_queue) {
                    // Add to the queue (to make sure nothing is lost)
                    g_scrobble_queue->add_track(copy);

                    // Try to process the queue right away
                    g_scrobble_queue->process_queue();

                    console::print("Last.fm Scrobbler: Track queued for scrobbling");
                }

                m_scrobbled = true;
            } catch (const std::exception& e) {
                console::complain("Last.fm Scrobbler", e.what());
            }
        }

        void on_playback_seek(double p_time) override {
            // If seeked back below threshold, allow re-scrobble
            if (p_time < m_threshold) {
                m_scrobbled = false;
            }
        }

        void on_playback_stop(play_control::t_stop_reason p_reason) override {
            m_scrobbled = false;
            m_length = 0;
            m_threshold = 0;
        }

        void on_playback_pause(bool p_state) override {
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
}
