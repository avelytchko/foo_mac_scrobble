//
//  safe_log_utils.h
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#pragma once
#include <string>

// Masks sensitive data (e.g., API keys, session keys) for safe logging
inline std::string redact_secret(const std::string& value) {
    if (value.empty()) return "<empty>";
    if (value.length() <= 6) return "******";
    return value.substr(0, 2) + "****" + value.substr(value.length() - 2);
}
