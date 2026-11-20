// EH3D-NODE-PNP-HEAD - core/head_config.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// Every tuned value of the node lives in one settings file (head.cfg), never in the code. A value out of range or a key nobody knows is reported and the default stays,
// so a typing mistake never turns a safety limit off. The defaults are the conservative ones: the calibration of the pressure sensors is off, and when the server is lost
// the pump stops and the valves close.
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "cfg_text.hpp"

namespace pnp_head {

constexpr int kMaxNozzles = 6;

enum class SafeVacuum { Off, Hold };

struct Settings {
    // the link
    std::string name = "pnp-head";
    std::string token;                  // write-only: never shown, never logged; empty: the node refuses every command that moves or changes something
    std::uint16_t port = 80;            // 1..65535
    // the head
    int nozzles = 2;                    // 1..6
    // the Z axis of every nozzle, in mm: 0 is the lowest point of the travel and the numbers go up
    double z_min_mm = 0.0;              // -100..100
    double z_max_mm = 40.0;             // 1..300
    double z_safe_mm = 35.0;            // the height the nozzles go to when the node is safe
    double z_speed_mm_s = 30.0;         // 1..300
    // the rotation of every nozzle, in degrees
    double rot_min_deg = -180.0;        // -3600..0
    double rot_max_deg = 180.0;         // 0..3600
    double rot_speed_deg_s = 360.0;     // 10..3600
    // the interlocks
    double rotate_min_z_mm = 10.0;      // a nozzle is not turned below this height (a part on the board or in the tape could be hit)
    double release_max_z_mm = 5.0;      // a part is released only at or below this height (it would fall from higher)
    // the vacuum, in kPa below the air pressure
    double vacuum_target_kpa = 50.0;    // 5..95: reached within vacuum_settle_ms means a part is held
    double vacuum_hold_min_kpa = 35.0;  // 3..95, below the target: a held part whose vacuum falls under this for vacuum_loss_ms is lost
    int vacuum_settle_ms = 600;         // 50..5000
    int vacuum_loss_ms = 100;           // 10..5000
    int blow_ms = 80;                   // 10..2000: the puff that lets a part go
    int pump_max_run_s = 600;           // 5..86400: the longest the pump may run without a stop
    // the pressure sensors (one per nozzle): kPa = (raw - pressure_zero) * pressure_scale
    double pressure_zero = 0.0;         // -1000..1000
    double pressure_scale = 0.1;        // 0.0001..100
    double pressure_max_kpa = 101.0;    // 20..200: a reading above this cannot be real
    int pressure_stuck_ms = 3000;       // 500..60000: a reading that does not change at all for this long while a valve is open is a dead sensor
    int pressure_samples = 4;           // 1..32: readings averaged
    bool calibration_enabled = false;   // the calibration of the sensors changes what the limits mean: off until a person switches it on
    // the lights, in percent, at the start
    int light_up_pct = 0;               // 0..100
    int light_down_pct = 0;             // 0..100
    // the watch over the server and what the node does when it is lost
    int link_lost_ms = 2000;            // 500..600000
    bool safe_z_retract = true;         // go up to z_safe_mm in the safe state
    SafeVacuum safe_vacuum = SafeVacuum::Off;  // off: the pump stops and the valves close (a part may fall); hold: they stay as they were
};

struct Problem {
    std::string key;
    std::string code;  // unknown_key, bad_value, out_of_range
};

inline const char* safe_vacuum_text(SafeVacuum v) { return v == SafeVacuum::Hold ? "hold" : "off"; }

namespace detail {

inline void set_int(const std::string& key, const std::string& v, long long lo, long long hi, int& target, std::vector<Problem>& out) {
    long long n = 0;
    if (!cfg::parse_int(v, lo, hi, n)) { out.push_back({key, "out_of_range"}); return; }
    target = static_cast<int>(n);
}

inline void set_double(const std::string& key, const std::string& v, double lo, double hi, double& target, std::vector<Problem>& out) {
    double d = 0;
    if (!cfg::parse_double(v, lo, hi, d)) { out.push_back({key, "out_of_range"}); return; }
    target = d;
}

inline void set_bool(const std::string& key, const std::string& v, bool& target, std::vector<Problem>& out) {
    bool b = false;
    if (!cfg::parse_bool(v, b)) { out.push_back({key, "bad_value"}); return; }
    target = b;
}

inline bool name_ok(const std::string& n) {
    if (n.empty() || n.size() > 32) return false;
    for (char c : n)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
    return true;
}

}  // namespace detail

/// Reads the text of a settings file into `s` (which holds the defaults or the values read before). Returns what was refused.
inline std::vector<Problem> parse_settings(const std::string& text, Settings& s) {
    std::vector<Problem> out;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string::npos) end = text.size();
        std::string key, v;
        if (cfg::split_line(text.substr(pos, end - pos), key, v)) {
            long long n = 0;
            if (key == "name") { if (detail::name_ok(v)) s.name = v; else out.push_back({key, "bad_value"}); }
            else if (key == "token") { if (v.size() <= 64) s.token = v; else out.push_back({key, "out_of_range"}); }
            else if (key == "port") { if (cfg::parse_int(v, 1, 65535, n)) s.port = static_cast<std::uint16_t>(n); else out.push_back({key, "out_of_range"}); }
            else if (key == "nozzles") detail::set_int(key, v, 1, kMaxNozzles, s.nozzles, out);
            else if (key == "z_min_mm") detail::set_double(key, v, -100, 100, s.z_min_mm, out);
            else if (key == "z_max_mm") detail::set_double(key, v, 1, 300, s.z_max_mm, out);
            else if (key == "z_safe_mm") detail::set_double(key, v, -100, 300, s.z_safe_mm, out);
            else if (key == "z_speed_mm_s") detail::set_double(key, v, 1, 300, s.z_speed_mm_s, out);
            else if (key == "rot_min_deg") detail::set_double(key, v, -3600, 0, s.rot_min_deg, out);
            else if (key == "rot_max_deg") detail::set_double(key, v, 0, 3600, s.rot_max_deg, out);
            else if (key == "rot_speed_deg_s") detail::set_double(key, v, 10, 3600, s.rot_speed_deg_s, out);
            else if (key == "rotate_min_z_mm") detail::set_double(key, v, -100, 300, s.rotate_min_z_mm, out);
            else if (key == "release_max_z_mm") detail::set_double(key, v, -100, 300, s.release_max_z_mm, out);
            else if (key == "vacuum_target_kpa") detail::set_double(key, v, 5, 95, s.vacuum_target_kpa, out);
            else if (key == "vacuum_hold_min_kpa") detail::set_double(key, v, 3, 95, s.vacuum_hold_min_kpa, out);
            else if (key == "vacuum_settle_ms") detail::set_int(key, v, 50, 5000, s.vacuum_settle_ms, out);
            else if (key == "vacuum_loss_ms") detail::set_int(key, v, 10, 5000, s.vacuum_loss_ms, out);
            else if (key == "blow_ms") detail::set_int(key, v, 10, 2000, s.blow_ms, out);
            else if (key == "pump_max_run_s") detail::set_int(key, v, 5, 86400, s.pump_max_run_s, out);
            else if (key == "pressure_zero") detail::set_double(key, v, -1000, 1000, s.pressure_zero, out);
            else if (key == "pressure_scale") detail::set_double(key, v, 0.0001, 100, s.pressure_scale, out);
            else if (key == "pressure_max_kpa") detail::set_double(key, v, 20, 200, s.pressure_max_kpa, out);
            else if (key == "pressure_stuck_ms") detail::set_int(key, v, 500, 60000, s.pressure_stuck_ms, out);
            else if (key == "pressure_samples") detail::set_int(key, v, 1, 32, s.pressure_samples, out);
            else if (key == "calibration_enabled") detail::set_bool(key, v, s.calibration_enabled, out);
            else if (key == "light_up_pct") detail::set_int(key, v, 0, 100, s.light_up_pct, out);
            else if (key == "light_down_pct") detail::set_int(key, v, 0, 100, s.light_down_pct, out);
            else if (key == "link_lost_ms") detail::set_int(key, v, 500, 600000, s.link_lost_ms, out);
            else if (key == "safe_z_retract") detail::set_bool(key, v, s.safe_z_retract, out);
            else if (key == "safe_vacuum") { if (v == "off") s.safe_vacuum = SafeVacuum::Off; else if (v == "hold") s.safe_vacuum = SafeVacuum::Hold; else out.push_back({key, "bad_value"}); }
            else out.push_back({key, "unknown_key"});
        }
        pos = end + 1;
    }
    // the settings must also agree with each other; a group that does not goes back to the defaults, the safe way
    Settings d;
    if (s.vacuum_hold_min_kpa >= s.vacuum_target_kpa) { out.push_back({"vacuum_hold_min_kpa", "out_of_range"}); s.vacuum_hold_min_kpa = d.vacuum_hold_min_kpa; s.vacuum_target_kpa = d.vacuum_target_kpa; }
    if (!(s.z_min_mm < s.z_max_mm) || s.z_safe_mm < s.z_min_mm || s.z_safe_mm > s.z_max_mm) { out.push_back({"z_safe_mm", "out_of_range"}); s.z_min_mm = d.z_min_mm; s.z_max_mm = d.z_max_mm; s.z_safe_mm = d.z_safe_mm; }
    if (s.rotate_min_z_mm > s.z_max_mm) { out.push_back({"rotate_min_z_mm", "out_of_range"}); s.rotate_min_z_mm = s.z_max_mm; }  // turn only when fully up: the safest
    if (s.release_max_z_mm < s.z_min_mm) { out.push_back({"release_max_z_mm", "out_of_range"}); s.release_max_z_mm = d.release_max_z_mm; }
    return out;
}

/// The text of the settings as a file (the token is never written).
inline std::string settings_text(const Settings& s) {
    char b[2400];
    std::snprintf(b, sizeof b,
                  "name = %s\nport = %u\nnozzles = %d\nz_min_mm = %g\nz_max_mm = %g\nz_safe_mm = %g\nz_speed_mm_s = %g\nrot_min_deg = %g\nrot_max_deg = %g\nrot_speed_deg_s = %g\n"
                  "rotate_min_z_mm = %g\nrelease_max_z_mm = %g\nvacuum_target_kpa = %g\nvacuum_hold_min_kpa = %g\nvacuum_settle_ms = %d\nvacuum_loss_ms = %d\nblow_ms = %d\n"
                  "pump_max_run_s = %d\npressure_zero = %g\npressure_scale = %g\npressure_max_kpa = %g\npressure_stuck_ms = %d\npressure_samples = %d\ncalibration_enabled = %d\n"
                  "light_up_pct = %d\nlight_down_pct = %d\nlink_lost_ms = %d\nsafe_z_retract = %d\nsafe_vacuum = %s\n",
                  s.name.c_str(), static_cast<unsigned>(s.port), s.nozzles, s.z_min_mm, s.z_max_mm, s.z_safe_mm, s.z_speed_mm_s, s.rot_min_deg, s.rot_max_deg, s.rot_speed_deg_s,
                  s.rotate_min_z_mm, s.release_max_z_mm, s.vacuum_target_kpa, s.vacuum_hold_min_kpa, s.vacuum_settle_ms, s.vacuum_loss_ms, s.blow_ms, s.pump_max_run_s,
                  s.pressure_zero, s.pressure_scale, s.pressure_max_kpa, s.pressure_stuck_ms, s.pressure_samples, s.calibration_enabled ? 1 : 0, s.light_up_pct, s.light_down_pct,
                  s.link_lost_ms, s.safe_z_retract ? 1 : 0, safe_vacuum_text(s.safe_vacuum));
    return b;
}

}  // namespace pnp_head
