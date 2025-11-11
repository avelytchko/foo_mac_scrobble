//
//  config.cpp
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#include "stdafx.h"
#include "config.h"

namespace foo_lastfm {
    // Initialize API key (default: empty)
    const GUID guid_cfg_api_key = { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
    // Initialize API secret (default: empty)
    const GUID guid_cfg_api_secret = { 0x23456789, 0x2345, 0x2345, { 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01 } };
    // Initialize session key (default: empty)
    const GUID guid_cfg_session_key = { 0x34567890, 0x3456, 0x3456, { 0x34, 0x56, 0x78, 0x90, 0xbc, 0xde, 0xf0, 0x12 } };
    // Initialize username (default: empty)
    const GUID guid_cfg_username = { 0x45678901, 0x4567, 0x4567, { 0x45, 0x67, 0x89, 0x01, 0xcd, 0xef, 0x01, 0x23 } };
    // Initialize scrobble threshold percentage (default: 50%)
    const GUID guid_cfg_scrobble_percent = { 0x56789012, 0x5678, 0x5678, { 0x56, 0x78, 0x90, 0x12, 0xde, 0xf0, 0x12, 0x34 } };
    // Initialize scrobbling enabled flag (default: true)
    const GUID guid_cfg_enabled = { 0x67890123, 0x6789, 0x6789, { 0x67, 0x89, 0x01, 0x23, 0xef, 0x01, 0x23, 0x45 } };
    // Initialize debug logging enabled flag (default: false)
    const GUID guid_cfg_debug_enabled = { 0x78901234, 0x7890, 0x7890, { 0x78, 0x90, 0x12, 0x34, 0xf0, 0x12, 0x34, 0x56 } };
    const GUID guid_preferences_page = { 0xa7b8c9da, 0xe0f1, 0xa1b2, { 0x4c, 0x5d, 0x6e, 0x7f, 0x80, 0x91, 0xa2, 0xb3 } };
    
    // Initialize API key (default: empty)
    cfg_string cfg_api_key(guid_cfg_api_key, "");
    // Initialize API secret (default: empty)
    cfg_string cfg_api_secret(guid_cfg_api_secret, "");
    // Initialize session key (default: empty)
    cfg_string cfg_session_key(guid_cfg_session_key, "");
    // Initialize username (default: empty)
    cfg_string cfg_username(guid_cfg_username, "");
    // Initialize scrobble threshold percentage (default: 50%)
    cfg_int cfg_scrobble_percent(guid_cfg_scrobble_percent, 50);
    // Initialize scrobbling enabled flag (default: true)
    cfg_bool cfg_enabled(guid_cfg_enabled, true);
    // Initialize debug logging enabled flag
    // Default: controlled by compile-time flag FOO_LASTFM_DEBUG_DEFAULT
    #ifndef FOO_LASTFM_DEBUG_DEFAULT
    #define FOO_LASTFM_DEBUG_DEFAULT 0
    #endif

    #if FOO_LASTFM_DEBUG_DEFAULT
    cfg_bool cfg_debug_enabled(guid_cfg_debug_enabled, true);
    #else
    cfg_bool cfg_debug_enabled(guid_cfg_debug_enabled, false);
    #endif
}

    // Export the GUID for external use
namespace lastfm_config {
    // Initialize API key (default: empty)
    const GUID guid_cfg_api_key = foo_lastfm::guid_cfg_api_key;
    // Initialize API secret (default: empty)
    const GUID guid_cfg_api_secret = foo_lastfm::guid_cfg_api_secret;
    // Initialize session key (default: empty)
    const GUID guid_cfg_session_key = foo_lastfm::guid_cfg_session_key;
    // Initialize username (default: empty)
    const GUID guid_cfg_username = foo_lastfm::guid_cfg_username;
    // Initialize scrobble threshold percentage (default: 50%)
    const GUID guid_cfg_scrobble_percent = foo_lastfm::guid_cfg_scrobble_percent;
    // Initialize scrobbling enabled flag (default: true)
    const GUID guid_cfg_enabled = foo_lastfm::guid_cfg_enabled;
    // Initialize debug logging enabled flag (default: false)
    const GUID guid_cfg_debug_enabled = foo_lastfm::guid_cfg_debug_enabled;
    const GUID guid_preferences_page = foo_lastfm::guid_preferences_page;
}
