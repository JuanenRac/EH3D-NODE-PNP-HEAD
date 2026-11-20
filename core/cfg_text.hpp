// EH3D-NODE-PNP-HEAD - core/cfg_text.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The reading of a settings file made of `key = value` lines with `#` comments. A value that is not a number, or is out of its range, is reported by the caller and the
// default stays: a typing mistake never turns a safety limit off.
#pragma once

#include <cerrno>
#include <cstdlib>
#include <string>

namespace pnp_head {
namespace cfg {

inline std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}

/// Splits one line. False for a blank line, a comment or a line without `=`; a comment after the value is cut off.
inline bool split_line(const std::string& raw, std::string& key, std::string& value) {
    std::string line = trim(raw);
    if (line.empty() || line[0] == '#') return false;
    std::size_t eq = line.find('=');
    if (eq == std::string::npos) return false;
    key = trim(line.substr(0, eq));
    value = trim(line.substr(eq + 1));
    std::size_t hash = value.find('#');  // a `#` starts a comment, also after a value (so a token cannot hold one)
    if (hash != std::string::npos) value = trim(value.substr(0, hash));
    return !key.empty();
}

inline bool parse_int(const std::string& v, long long lo, long long hi, long long& out) {
    if (v.empty()) return false;
    char* end = nullptr;
    errno = 0;
    long long n = std::strtoll(v.c_str(), &end, 10);
    if (errno != 0 || end == v.c_str() || *end != '\0') return false;
    if (n < lo || n > hi) return false;
    out = n;
    return true;
}

inline bool parse_double(const std::string& v, double lo, double hi, double& out) {
    if (v.empty()) return false;
    char* end = nullptr;
    errno = 0;
    double d = std::strtod(v.c_str(), &end);
    if (errno != 0 || end == v.c_str() || *end != '\0') return false;
    if (!(d >= lo && d <= hi)) return false;  // also refuses nan
    out = d;
    return true;
}

inline bool parse_bool(const std::string& v, bool& out) {
    if (v == "1" || v == "true" || v == "on" || v == "yes") { out = true; return true; }
    if (v == "0" || v == "false" || v == "off" || v == "no") { out = false; return true; }
    return false;
}

}  // namespace cfg
}  // namespace pnp_head
