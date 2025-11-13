//
//  initquit.cpp
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#include "config.h"
#include "lastfm_api.h"
#include "scrobble_queue.h"
#include "session_manager.h"
#include "stdafx.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

namespace foo_lastfm
{

LastfmApi* g_lastfm_api = nullptr;

// ============================================================
// Helper: Logging for API credentials
// ============================================================
static void log_api_credentials(const char* api_key, const char* api_secret, bool debug_enabled)
{
    if (!debug_enabled)
        return;

    const int key_len = api_key ? static_cast<int>(strlen(api_key)) : 0;
    const int secret_len = api_secret ? static_cast<int>(strlen(api_secret)) : 0;

    FB2K_console_formatter() << "Last.fm: Loaded API credentials - key length: " << key_len
                             << ", secret length: " << secret_len;

    if (key_len == 0 || secret_len == 0)
    {
        FB2K_console_formatter() << "Last.fm: WARNING - Invalid or missing API credentials";
        if (key_len == 0)
            FB2K_console_formatter() << "Last.fm: API Key is missing or empty";
        if (secret_len == 0)
            FB2K_console_formatter() << "Last.fm: API Secret is missing or empty";
    }
    else
    {
        FB2K_console_formatter() << "Last.fm: Credentials appear valid";
    }
}

// ============================================================
// Background worker thread for queue processing
// ============================================================
class queue_processor_thread
{
  public:
    queue_processor_thread() : m_running(false) {}

    void start()
    {
        if (m_running)
            return;
        m_running = true;
        m_thread = std::thread(
            [this]()
            {
                while (m_running)
                {
                    for (int i = 0; i < 30 && m_running; ++i)
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }

                    if (!m_running)
                        break;

                    // Call process_queue() without inMainThread to avoid blocking UI
                    if (g_scrobble_queue)
                    {
                        g_scrobble_queue->process_queue();
                    }
                }
            });
    }

    void stop()
    {
        m_running = false;
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

  private:
    std::atomic<bool> m_running;
    std::thread m_thread;
};

static queue_processor_thread* g_queue_processor = nullptr;

// ============================================================
// Main plugin init/quit class
// ============================================================
class initquit_lastfm : public initquit
{
  public:
    void on_init() override
    {
        FB2K_console_formatter() << "Last.fm Scrobbler: Initializing plugin...";

        // Initialize core objects
        g_lastfm_api = new LastfmApi();
        g_session_manager = new SessionManager();

        // Load API credentials from configuration
        pfc::string8 api_key_pfc = cfg_api_key.get();
        pfc::string8 api_secret_pfc = cfg_api_secret.get();
        const char* api_key = api_key_pfc.c_str();
        const char* api_secret = api_secret_pfc.c_str();

        log_api_credentials(api_key, api_secret, cfg_debug_enabled.get());
        g_lastfm_api->set_credentials(api_key, api_secret);

        // ============================================================
        // Load session from file or config
        // ============================================================
        std::string session_key, username;

        if (g_session_manager->load_session(session_key, username))
        {
            // Loaded from session file
            if (strlen(api_key) == 0 || strlen(api_secret) == 0)
            {
                FB2K_console_formatter() << "Last.fm: Cannot validate session - missing API credentials";
                g_lastfm_api->set_session_key("");
                g_session_manager->clear_session();
                cfg_session_key.set("");
                cfg_username.set("");
            }
            else
            {
                g_lastfm_api->set_session_key(session_key.c_str());
                if (cfg_debug_enabled.get())
                    FB2K_console_formatter() << "Last.fm: Validating session...";

                if (g_lastfm_api->validate_session())
                {
                    cfg_username.set(username.c_str());
                    FB2K_console_formatter() << "Last.fm Scrobbler: Authenticated as: " << username.c_str();
                }
                else
                {
                    FB2K_console_formatter() << "Last.fm: Saved session invalid, clearing...";
                    g_lastfm_api->set_session_key("");
                    g_session_manager->clear_session();
                    cfg_session_key.set("");
                    cfg_username.set("");
                }
            }
        }
        else
        {
            // Try legacy session from config
            const char* config_session_key = cfg_session_key.get();
            std::string session_key_clean = config_session_key ? config_session_key : "";
            session_key_clean.erase(std::remove_if(session_key_clean.begin(), session_key_clean.end(),
                                                   [](unsigned char c) { return std::isspace(c) || c == '\0'; }),
                                    session_key_clean.end());

            if (!session_key_clean.empty())
            {
                g_lastfm_api->set_session_key(session_key_clean.c_str());
                if (cfg_debug_enabled.get())
                    FB2K_console_formatter() << "Last.fm: Validating session from config...";

                if (g_lastfm_api->validate_session())
                {
                    pfc::string8 old_username = cfg_username.get();
                    g_session_manager->save_session(config_session_key, old_username.c_str());
                    FB2K_console_formatter() << "Last.fm Scrobbler: Authenticated as: " << old_username.c_str();
                }
                else
                {
                    FB2K_console_formatter() << "Last.fm: Config session invalid, clearing...";
                    g_lastfm_api->set_session_key("");
                    cfg_session_key.set("");
                    cfg_username.set("");
                }
            }
            else
            {
                FB2K_console_formatter() << "Last.fm Scrobbler: Not authenticated - please configure in preferences";
            }
        }

        // ============================================================
        // Debug summary
        // ============================================================
        if (cfg_debug_enabled.get())
        {
            FB2K_console_formatter() << "Last.fm: Debug logging ENABLED";
            FB2K_console_formatter() << "Last.fm: Scrobbling is " << (cfg_enabled.get() ? "ENABLED" : "DISABLED");
            FB2K_console_formatter() << "Last.fm: Scrobble threshold: " << cfg_scrobble_percent.get() << "%";
        }

        // ============================================================
        // Initialize queue and start worker
        // ============================================================
        g_scrobble_queue = new ScrobbleQueue();
        g_queue_processor = new queue_processor_thread();
        g_queue_processor->start();

        FB2K_console_formatter() << "Last.fm Scrobbler: Initialized successfully";
    }

    void on_quit() override
    {
        if (cfg_debug_enabled.get())
            FB2K_console_formatter() << "Last.fm: Plugin shutting down...";

        // Stop background worker
        if (g_queue_processor)
        {
            g_queue_processor->stop();
            delete g_queue_processor;
            g_queue_processor = nullptr;
        }

        // Cleanup global instances
        delete g_scrobble_queue;
        g_scrobble_queue = nullptr;

        delete g_session_manager;
        g_session_manager = nullptr;

        delete g_lastfm_api;
        g_lastfm_api = nullptr;

        FB2K_console_formatter() << "Last.fm Scrobbler: Shutdown complete";
    }
};

static service_factory_single_t<initquit_lastfm> g_initquit_lastfm;

} // namespace foo_lastfm
