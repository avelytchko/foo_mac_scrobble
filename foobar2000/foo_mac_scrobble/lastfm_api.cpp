//
//  lastfm_api.cpp
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#include "lastfm_api.h"

#include "config.h"
#include "safe_log_utils.h"
#include "scrobble_queue.h"
#include "session_manager.h"
#include "stdafx.h"

#include <CommonCrypto/CommonDigest.h>
#include <algorithm>
#include <chrono>
#include <curl/curl.h>
#include <iomanip>
#include <main_thread_callback.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>
#include <threaded_process.h>

using json = nlohmann::json;

const char* LastfmApi::API_URL = "https://ws.audioscrobbler.com/2.0/";
const char* LastfmApi::AUTH_URL = "https://www.last.fm/api/auth/?api_key=";

void log_debug(const char* fmt, ...)
{
    if (!foo_lastfm::cfg_debug_enabled.get())
        return;
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    FB2K_console_formatter() << buffer;
}

LastfmApi::LastfmApi()
{
    log_debug("Last.fm: LastfmApi instance created");
}
LastfmApi::~LastfmApi()
{
    log_debug("Last.fm: LastfmApi instance destroyed");
}

void LastfmApi::set_credentials(const char* api_key, const char* api_secret)
{
    // Sanitize and store API credentials
    std::string new_api_key = api_key ? api_key : "";
    std::string new_api_secret = api_secret ? api_secret : "";

    if (foo_lastfm::cfg_debug_enabled.get())
    {
        FB2K_console_formatter() << "Last.fm: Credentials set (api_key: " << redact_secret(new_api_key).c_str()
                                 << " [len=" << new_api_key.length()
                                 << "], api_secret: " << redact_secret(new_api_secret).c_str()
                                 << " [len=" << new_api_secret.length() << "])";
    }

    // Clear session if credentials change
    if (new_api_key != m_api_key || new_api_secret != m_api_secret)
    {
        if (!m_session_key.empty() && foo_lastfm::cfg_debug_enabled.get())
        {
            FB2K_console_formatter() << "Last.fm: Credentials changed - clearing session";
        }
        m_session_key.clear();
    }

    m_api_key = new_api_key;
    m_api_secret = new_api_secret;
}

void LastfmApi::set_session_key(const char* session_key)
{
    // Sanitize and store session key
    std::string new_session_key = session_key ? session_key : "";
    new_session_key.erase(std::remove_if(new_session_key.begin(), new_session_key.end(),
                                         [](unsigned char c) { return std::isspace(c) || c == '\0'; }),
                          new_session_key.end());
    if (new_session_key != m_session_key)
    {
        m_session_key = new_session_key;
        log_debug("Last.fm: Session key set (len=%d, value=%s)", (int)m_session_key.length(),
                  redact_secret(m_session_key).c_str());
    }
}

std::string LastfmApi::get_auth_url() const
{
    // Generate URL for Last.fm authentication
    return std::string(AUTH_URL) + m_api_key;
}

bool LastfmApi::is_authenticated() const
{
    return !m_session_key.empty();
}

bool LastfmApi::authenticate(const std::string& token)
{
    log_debug("Last.fm: Starting authentication (token length: %d)", (int)token.length());
    std::map<std::string, std::string> params{
        {"method", "auth.getSession"},
        {"api_key", m_api_key},
        {"token", token},
    };

    std::string response;
    if (!send_api_request(params, response))
        return false;

    // Parse authentication response
    try
    {
        auto json_data = json::parse(response);
        if (json_data.contains("error"))
        {
            int code = json_data["error"].get<int>();
            std::string msg = json_data["message"].get<std::string>();
            FB2K_console_formatter() << "Last.fm ERROR: " << code << " - " << msg.c_str();
            return false;
        }

        if (json_data.contains("session"))
        {
            auto session = json_data["session"];
            m_session_key = session.value("key", "");

            // Save session to file
            std::string username = session.value("name", "");
            if (foo_lastfm::g_session_manager)
            {
                foo_lastfm::g_session_manager->save_session(m_session_key, username);
            }

            foo_lastfm::cfg_username.set(username.c_str());
            return true;
        }

        FB2K_console_formatter() << "Last.fm ERROR: Unexpected JSON format (missing session)";
        return false;
    }
    catch (const std::exception& e)
    {
        FB2K_console_formatter() << "Last.fm JSON parse error: " << e.what();
        return false;
    }
}

bool LastfmApi::update_now_playing(const TrackInfo& track)
{
    if (!is_authenticated())
        return false;
    std::map<std::string, std::string> params{
        {"method", "track.updateNowPlaying"},
        {"api_key", m_api_key},
        {"sk", m_session_key},
        {"artist", track.artist},
        {"track", track.track},
    };
    if (!track.album.empty())
        params["album"] = track.album;
    if (!track.album_artist.empty())
        params["albumArtist"] = track.album_artist;
    if (track.duration > 0)
        params["duration"] = std::to_string(track.duration);
    if (track.track_number > 0)
        params["trackNumber"] = std::to_string(track.track_number);

    std::string response;
    bool ok = send_api_request(params, response);
    if (!ok)
        FB2K_console_formatter() << "Last.fm: Failed to update now playing";
    return ok;
}

bool LastfmApi::scrobble_track(const TrackInfo& track)
{
    if (!is_authenticated())
        return false;

    std::map<std::string, std::string> params{
        {"method", "track.scrobble"}, {"api_key", m_api_key}, {"sk", m_session_key},
        {"artist", track.artist},     {"track", track.track}, {"timestamp", std::to_string(track.timestamp)},
    };
    if (!track.album.empty())
        params["album"] = track.album;
    if (!track.album_artist.empty())
        params["albumArtist"] = track.album_artist;
    if (track.duration > 0)
        params["duration"] = std::to_string(track.duration);
    if (track.track_number > 0)
        params["trackNumber"] = std::to_string(track.track_number);

    std::string response;
    bool ok = send_api_request(params, response);
    if (!ok)
        FB2K_console_formatter() << "Last.fm: Scrobble failed";
    return ok;
}

bool LastfmApi::validate_session()
{
    if (m_session_key.empty())
        return false;

    std::map<std::string, std::string> params{
        {"method", "auth.getSessionInfo"}, {"api_key", m_api_key}, {"sk", m_session_key}};

    std::string response;
    CURLcode curl_res = CURLE_OK;
    long http_code = 0;

    // Send request to validate session
    bool ok = send_api_request(params, response, &curl_res, &http_code);

    if (curl_res == CURLE_COULDNT_CONNECT || curl_res == CURLE_OPERATION_TIMEDOUT ||
        curl_res == CURLE_COULDNT_RESOLVE_HOST || http_code == 0)
    {
        FB2K_console_formatter() << "Last.fm: Offline mode detected, skipping session validation";
        return true; // Keep session in offline mode
    }

    if (!ok || http_code == 403 || http_code == 401)
    {
        FB2K_console_formatter() << "Last.fm: Saved session is invalid (HTTP " << http_code << "), clearing...";
        m_session_key.clear();
        if (foo_lastfm::g_session_manager)
            foo_lastfm::g_session_manager->clear_session();
        return false;
    }

    FB2K_console_formatter() << "Last.fm: Session key validated (length: " << (int)m_session_key.length() << ")";
    return true;
}

std::string LastfmApi::calculate_signature(const std::map<std::string, std::string>& params) const
{
    // Calculate MD5 signature for API request authentication
    std::string sig;
    std::map<std::string, std::string> temp_params;
    for (const auto& [k, v] : params)
    {
        if (k != "format" && k != "api_sig")
        {
            temp_params[k] = v;
        }
    }

    for (const auto& [k, v] : temp_params)
    {
        sig += k + v;
    }
    sig += m_api_secret;

    unsigned char hash[CC_MD5_DIGEST_LENGTH];
    CC_MD5(sig.c_str(), (CC_LONG)sig.length(), hash);

    std::ostringstream ss;
    for (int i = 0; i < CC_MD5_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

std::string LastfmApi::url_encode(const std::string& v) const
{
    // URL-encode string for API requests
    std::ostringstream out;
    out.fill('0');
    out << std::hex;
    for (char c : v)
    {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~')
            out << c;
        else
            out << '%' << std::setw(2) << (int)(unsigned char)c;
    }
    return out.str();
}

size_t LastfmApi::write_callback(void* c, size_t s, size_t n, void* u)
{
    // CURL callback to collect response data
    size_t t = s * n;
    ((std::string*)u)->append((char*)c, t);
    return t;
}

bool LastfmApi::send_api_request(const std::map<std::string, std::string>& params, std::string& response,
                                 CURLcode* res_out, long* http_code_out)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    // Sanitize session key in parameters
    std::map<std::string, std::string> params_clean = params;
    if (auto it = params_clean.find("sk"); it != params_clean.end())
    {
        auto& v = it->second;
        v.erase(std::remove_if(v.begin(), v.end(), [](unsigned char c) { return std::isspace(c) || c == '\0'; }),
                v.end());
    }

    std::map<std::string, std::string> signed_params = params;
    signed_params["format"] = "json";
    if (foo_lastfm::cfg_debug_enabled.get())
    {
        for (const auto& [k, v] : signed_params)
        {
            const bool sensitive = (k == "api_key" || k == "sk" || k == "token");
            FB2K_console_formatter() << "  " << k.c_str() << " = "
                                     << (sensitive ? redact_secret(v).c_str() : v.c_str());
        }
    }

    // Generate API signature
    std::string signature = calculate_signature(params_clean);
    signed_params["api_sig"] = signature;

    // Build POST data
    std::string post_data;
    for (const auto& [k, v] : signed_params)
    {
        if (!post_data.empty())
            post_data += "&";
        post_data += k + "=" + url_encode(v);
    }

    response.clear();

    // Configure CURL for secure API request
    curl_easy_setopt(curl, CURLOPT_URL, API_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "foo_mac_scrobble/0.1 (macOS)");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

    // Log request details for debugging
    auto it_method = signed_params.find("method");
    std::string method = (it_method != signed_params.end()) ? it_method->second : "";
    if (foo_lastfm::cfg_debug_enabled.get())
    {
        if (method == "auth.getSession" || method == "auth.getToken")
        {
            FB2K_console_formatter() << "---- cURL configuration ----";
            FB2K_console_formatter() << "CURLOPT_URL: " << API_URL;
            FB2K_console_formatter() << "CURLOPT_SSL_VERIFYPEER: 1";
            FB2K_console_formatter() << "CURLOPT_SSL_VERIFYHOST: 2";
            FB2K_console_formatter() << "CURLOPT_TIMEOUT: 15";
            FB2K_console_formatter() << "CURLOPT_CONNECTTIMEOUT: 5";
            FB2K_console_formatter() << "CURLOPT_FOLLOWLOCATION: 0";
            FB2K_console_formatter() << "CURLOPT_USERAGENT: foo_mac_scrobble/0.1 (macOS)";
            FB2K_console_formatter() << "CURLOPT_ACCEPT_ENCODING: <empty>";
            FB2K_console_formatter() << "Post data length: " << (int)post_data.size();
            FB2K_console_formatter() << "----------------------------";
        }
        else
        {
            FB2K_console_formatter() << "Post data length: " << (int)post_data.size();
        }
    }

    // Retry logic with exponential backoff
    int attempt = 0;
    CURLcode res = CURLE_OK;
    long http_code = 0;
    long backoff_ms = 200;

    for (; attempt < 3; ++attempt)
    {
        response.clear();
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (res_out)
            *res_out = res;
        if (http_code_out)
            *http_code_out = http_code;

        if (foo_lastfm::cfg_debug_enabled.get())
        {
            if (http_code == 403)
            {
                FB2K_console_formatter() << "Last.fm: 403 Forbidden - likely signature or authentication issue";
                FB2K_console_formatter() << "Last.fm: Response body: " << response.c_str();
            }
        }

        // Handle session invalidation on 403 errors
        if (res == CURLE_OK && http_code == 403)
        {
            if (method != "auth.getSession" && method != "auth.getToken")
            {
                m_session_key.clear();
                foo_lastfm::cfg_session_key.set("");
                if (foo_lastfm::cfg_debug_enabled.get())
                {
                    FB2K_console_formatter() << "Last.fm: Session invalidated due to 403 error";
                }
            }
            else if (foo_lastfm::cfg_debug_enabled.get())
            {
                FB2K_console_formatter() << "Last.fm: 403 during authentication (ignored)";
            }
        }

        if (res == CURLE_OK && http_code >= 200 && http_code < 300)
        {
            break; // Success
        }

        if (foo_lastfm::cfg_debug_enabled.get())
        {
            FB2K_console_formatter() << "Last.fm: HTTP " << http_code << " (" << curl_easy_strerror(res) << ") attempt "
                                     << (attempt + 1);
            if ((http_code == 400 || http_code == 401 || http_code == 403) && !response.empty())
            {
                FB2K_console_formatter() << "Last.fm: Response body: " << response.c_str();
            }
        }

        // Retry on rate limit or server errors
        if (http_code == 429 || (http_code >= 500 && http_code < 600))
        {
            if (foo_lastfm::cfg_debug_enabled.get())
            {
                FB2K_console_formatter() << "Last.fm: Backing off for " << backoff_ms << " ms";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
            backoff_ms = std::min(backoff_ms * 2, 1600L);
            continue;
        }

        break; // Don't retry for other errors
    }

    curl_easy_cleanup(curl);

    // Check for request failure
    if (res != CURLE_OK || http_code < 200 || http_code >= 300)
    {
        FB2K_console_formatter() << "Last.fm ERROR: HTTP " << http_code << " (" << curl_easy_strerror(res) << ")";
        return false;
    }

    // Parse JSON response
    try
    {
        auto test = json::parse(response);
        if (test.contains("error"))
        {
            FB2K_console_formatter() << "Last.fm API error: " << test["error"].get<int>() << " - "
                                     << test["message"].get<std::string>().c_str();
            return false;
        }
    }
    catch (const std::exception& e)
    {
        FB2K_console_formatter() << "Last.fm: Response not valid JSON (" << e.what() << ")";
        return false;
    }

    return true;
}

void LastfmApi::execute_async_request(const std::map<std::string, std::string>& params,
                                      std::function<void(bool success, const std::string& response)> callback)
{
    // Execute API request in a background thread
    std::thread(
        [params, callback, this]()
        {
            std::string response;
            bool success = false;
            try
            {
                success = send_api_request(params, response);
            }
            catch (...)
            {
                success = false;
            }
            fb2k::inMainThread([callback, success, response]() { callback(success, response); });
        })
        .detach();
}

void LastfmApi::authenticate_async(const std::string& token, std::function<void(bool success)> callback)
{
    // Perform asynchronous authentication
    std::map<std::string, std::string> params;
    params["method"] = "auth.getSession";
    params["api_key"] = m_api_key;
    params["token"] = token;

    execute_async_request(params,
                          [this, callback](bool success, const std::string& response)
                          {
                              if (success)
                              {
                                  try
                                  {
                                      auto json_data = json::parse(response);
                                      if (json_data.contains("error"))
                                      {
                                          int code = json_data["error"].get<int>();
                                          std::string msg = json_data["message"].get<std::string>();
                                          if (foo_lastfm::cfg_debug_enabled.get())
                                          {
                                              FB2K_console_formatter()
                                                  << "Last.fm ERROR: " << code << " - " << msg.c_str();
                                          }
                                          callback(false);
                                          return;
                                      }

                                      if (json_data.contains("session"))
                                      {
                                          auto session = json_data["session"];
                                          m_session_key = session.value("key", "");
                                          std::string username = session.value("name", "");
                                          if (foo_lastfm::g_session_manager)
                                          {
                                              foo_lastfm::g_session_manager->save_session(m_session_key, username);
                                          }
                                          if (!username.empty())
                                          {
                                              foo_lastfm::cfg_username.set(username.c_str());
                                          }
                                          callback(true);
                                          return;
                                      }

                                      if (foo_lastfm::cfg_debug_enabled.get())
                                      {
                                          FB2K_console_formatter()
                                              << "Last.fm ERROR: Unexpected JSON format (missing session)";
                                      }
                                      callback(false);
                                  }
                                  catch (const std::exception& e)
                                  {
                                      if (foo_lastfm::cfg_debug_enabled.get())
                                      {
                                          FB2K_console_formatter() << "Last.fm JSON parse error: " << e.what();
                                      }
                                      callback(false);
                                  }
                              }
                              else
                              {
                                  callback(false);
                              }
                          });
}

void LastfmApi::update_now_playing_async(const TrackInfo& track)
{
    if (!is_authenticated() || track.artist.empty() || track.track.empty())
    {
        return;
    }

    std::map<std::string, std::string> params;
    params["method"] = "track.updateNowPlaying";
    params["api_key"] = m_api_key;
    params["sk"] = m_session_key;
    params["artist"] = track.artist;
    params["track"] = track.track;
    params["album"] = track.album;
    params["albumArtist"] = track.album_artist;
    params["duration"] = std::to_string(track.duration);
    if (track.track_number > 0)
    {
        params["trackNumber"] = std::to_string(track.track_number);
    }

    execute_async_request(params,
                          [](bool success, const std::string& response)
                          {
                              // Silently ignore result for now playing
                              (void)success;
                              (void)response;
                          });
}

void LastfmApi::scrobble_track_async(const TrackInfo& track)
{
    if (!is_authenticated() || track.artist.empty() || track.track.empty())
    {
        return;
    }

    std::map<std::string, std::string> params;
    params["method"] = "track.scrobble";
    params["api_key"] = m_api_key;
    params["sk"] = m_session_key;
    params["artist"] = track.artist;
    params["track"] = track.track;
    params["timestamp"] = std::to_string(track.timestamp);
    params["album"] = track.album;
    params["albumArtist"] = track.album_artist;
    params["duration"] = std::to_string(track.duration);
    if (track.track_number > 0)
    {
        params["trackNumber"] = std::to_string(track.track_number);
    }

    execute_async_request(params,
                          [](bool success, const std::string& response)
                          {
                              if (!success && foo_lastfm::cfg_debug_enabled.get())
                              {
                                  FB2K_console_formatter() << "Last.fm: Scrobble failed silently in background";
                              }
                              else if (success && foo_lastfm::cfg_debug_enabled.get())
                              {
                                  FB2K_console_formatter() << "Last.fm: Scrobble successful";
                              }
                          });
}
