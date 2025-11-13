//
//  config.h
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#pragma once

#include "lastfm_api.h"

#include <foobar2000/SDK/foobar2000.h>

// Configuration GUIDs
namespace lastfm_config
{
// Configuration variable for Last.fm API key
extern const GUID guid_cfg_api_key;
// Configuration variable for Last.fm API secret
extern const GUID guid_cfg_api_secret;
// Configuration variable for Last.fm session key
extern const GUID guid_cfg_session_key;
// Configuration variable for Last.fm username
extern const GUID guid_cfg_username;
// Configuration variable for scrobble threshold percentage
extern const GUID guid_cfg_scrobble_percent;
// Configuration variable for enabling/disabling scrobbling
extern const GUID guid_cfg_enabled;
// Configuration variable for enabling debug logging
extern const GUID guid_cfg_debug_enabled;
extern const GUID guid_preferences_page;
} // namespace lastfm_config

// Configuration variables - external declarations
namespace foo_lastfm
{
extern LastfmApi* g_lastfm_api;
// Configuration variable for Last.fm API key
extern cfg_string cfg_api_key;
// Configuration variable for Last.fm API secret
extern cfg_string cfg_api_secret;
// Configuration variable for Last.fm session key
extern cfg_string cfg_session_key;
// Configuration variable for Last.fm username
extern cfg_string cfg_username;
// Configuration variable for enabling/disabling scrobbling
extern cfg_bool cfg_enabled;
// Configuration variable for scrobble threshold percentage
extern cfg_int cfg_scrobble_percent;
// Configuration variable for enabling debug logging
extern cfg_bool cfg_debug_enabled;
} // namespace foo_lastfm
