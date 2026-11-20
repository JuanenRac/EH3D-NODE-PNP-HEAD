// EH3D-NODE-PNP-HEAD - tests/test_config.cpp: the settings file, and the logic of the head fed with readings made by hand (the calibration, the filter, the plausibility).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../core/head.hpp"
#include "../core/head_config.hpp"
#include "check.hpp"

using namespace pnp_head;

namespace {
// ticks a head for `ms` with the same raw reading on every sensor
void feed(Head& h, std::uint32_t& now, std::uint32_t ms, double raw) {
    double r[kMaxNozzles] = {raw, raw, raw, raw, raw, raw};
    for (std::uint32_t t = 0; t < ms; t += 10) { now += 10; h.tick(now, 10, r); }
}
}  // namespace

TEST(defaults_are_the_conservative_ones) {
    Settings s;
    CHECK(s.token.empty());
    CHECK(!s.calibration_enabled);
    CHECK(s.safe_vacuum == SafeVacuum::Off);
    CHECK(s.safe_z_retract);
    CHECK(s.vacuum_hold_min_kpa < s.vacuum_target_kpa);
    CHECK(s.z_min_mm < s.z_safe_mm && s.z_safe_mm <= s.z_max_mm);
    CHECK(s.light_up_pct == 0 && s.light_down_pct == 0);
    CHECK(s.release_max_z_mm < s.rotate_min_z_mm);  // a part is let go low, a nozzle is turned high
}

TEST(a_value_out_of_range_or_a_key_nobody_knows_is_reported_and_the_default_stays) {
    Settings s;
    std::vector<Problem> p = parse_settings("# a comment\nnozzles = 9\nz_speed_mm_s = fast\nspeed = 3\nport = 0\ncalibration_enabled = maybe\nname = Bad Name\n\nblow_ms = 5000\nsafe_vacuum = both\n", s);
    CHECK(p.size() == 8);
    CHECK(s.nozzles == 2 && s.z_speed_mm_s == 30.0 && s.port == 80 && !s.calibration_enabled && s.name == "pnp-head" && s.blow_ms == 80 && s.safe_vacuum == SafeVacuum::Off);
    bool unknown = false;
    for (auto& x : p) if (x.key == "speed" && x.code == "unknown_key") unknown = true;
    CHECK(unknown);
}

TEST(settings_that_disagree_go_back_to_the_defaults) {
    Settings s;
    std::vector<Problem> p = parse_settings("vacuum_target_kpa = 30\nvacuum_hold_min_kpa = 40\n", s);
    CHECK(!p.empty() && s.vacuum_target_kpa == 50.0 && s.vacuum_hold_min_kpa == 35.0);
    Settings t;
    p = parse_settings("z_max_mm = 20\nz_safe_mm = 30\n", t);
    CHECK(!p.empty() && t.z_max_mm == 40.0 && t.z_safe_mm == 35.0);
    Settings u;
    p = parse_settings("z_max_mm = 8\nz_safe_mm = 8\nrotate_min_z_mm = 10\n", u);
    CHECK(!p.empty() && u.rotate_min_z_mm == 8.0 && u.z_max_mm == 8.0);  // the height to turn at cannot be above the travel: it becomes the top of it, the safest
}

TEST(the_settings_text_is_read_back_the_same_and_never_holds_the_token) {
    Settings s;
    s.token = "secret-token";
    s.nozzles = 4; s.calibration_enabled = true; s.safe_vacuum = SafeVacuum::Hold; s.vacuum_target_kpa = 55.5; s.pressure_scale = 0.125; s.pump_max_run_s = 90; s.light_up_pct = 30;
    std::string text = settings_text(s);
    CHECK(text.find("secret-token") == std::string::npos);
    CHECK(text.find("token") == std::string::npos);
    Settings back;
    CHECK(parse_settings(text, back).empty());
    CHECK(back.nozzles == 4 && back.calibration_enabled && back.safe_vacuum == SafeVacuum::Hold && back.vacuum_target_kpa == 55.5 && back.pressure_scale == 0.125 && back.pump_max_run_s == 90 &&
          back.light_up_pct == 30);
    CHECK(settings_text(back) == text);
}

TEST(the_token_is_read_but_limited) {
    Settings s;
    CHECK(parse_settings("token = abc123\n", s).empty() && s.token == "abc123");
    std::string long_token(65, 'x');
    CHECK(!parse_settings("token = " + long_token + "\n", s).empty() && s.token == "abc123");
}

TEST(a_head_with_no_command_does_nothing) {
    Settings s;
    Head h(s);
    std::uint32_t now = 1000;
    h.heard(now);
    feed(h, now, 5000, 0.0);
    CHECK(!h.pump());
    for (int n = 1; n <= h.count(); ++n) CHECK(!h.nozzle(n).valve && !h.nozzle(n).blow && h.nozzle(n).phase == Phase::Idle);
    CHECK(h.pump_run_ms() == 0);
}

TEST(the_filter_averages_the_last_readings_and_the_scale_turns_them_into_kpa) {
    Settings s;
    s.pressure_samples = 4;
    s.pressure_zero = 10.0;
    s.pressure_scale = 0.2;
    Head h(s);
    std::uint32_t now = 1000;
    double r1[kMaxNozzles] = {110, 110, 110, 110, 110, 110};
    for (int i = 0; i < 4; ++i) { now += 10; h.tick(now, 10, r1); }
    CHECK(std::fabs(h.nozzle(1).kpa - 20.0) < 1e-9);  // (110 - 10) * 0.2
    double r2[kMaxNozzles] = {210, 210, 210, 210, 210, 210};
    now += 10; h.tick(now, 10, r2);                    // one reading of four is new: (110 + 110 + 110 + 210) / 4 = 135
    CHECK(std::fabs(h.nozzle(1).kpa - ((135.0 - 10.0) * 0.2)) < 1e-9);
}

TEST(a_reading_that_cannot_be_real_is_a_sensor_fault_and_is_not_cleared_while_it_lasts) {
    Settings s;
    Head h(s);
    std::uint32_t now = 1000;
    h.heard(now);
    feed(h, now, 100, 5000.0);  // 500 kPa
    CHECK(h.nozzle(1).phase == Phase::Fault && h.nozzle(1).fault == Fault::SensorFault);
    CHECK(std::string(fault_code(Fault::SensorFault)) == "HEAD-S01");
    Result r = h.ack(1, now);
    CHECK(!r.ok && r.why == "still_active");
    feed(h, now, 100, 300.0);
    r = h.ack(1, now);
    CHECK(r.ok && r.why == "cleared" && h.nozzle(1).phase == Phase::Idle);
    CHECK(h.ack(1, now).why == "not_held");
}

TEST(a_negative_reading_far_below_the_air_pressure_is_also_a_sensor_fault) {
    Settings s;
    Head h(s);
    std::uint32_t now = 1000;
    feed(h, now, 100, -400.0);  // -40 kPa
    CHECK(h.nozzle(1).fault == Fault::SensorFault);
}

TEST(calibration_is_refused_unless_it_is_switched_on) {
    Settings s;
    Head h(s);
    std::uint32_t now = 1000;
    feed(h, now, 100, 20.0);
    Result r = h.calibrate(1, true, 0);
    CHECK(!r.ok && r.why == "disabled");
    CHECK(!h.calibrated());
    CHECK(h.settings().pressure_zero == 0.0);
}

TEST(the_zero_and_the_scale_are_set_from_the_readings) {
    Settings s;
    s.calibration_enabled = true;
    Head h(s);
    std::uint32_t now = 1000;
    feed(h, now, 100, 20.0);                    // open to the air, the sensor reads 20 counts
    CHECK(h.calibrate(1, true, 0).ok);
    CHECK(h.settings().pressure_zero == 20.0 && h.calibrated());
    feed(h, now, 100, 620.0);                   // a known vacuum of 60 kPa is on the sensor: 600 counts over the zero
    CHECK(h.calibrate(1, false, 60.0).ok);
    CHECK(std::fabs(h.settings().pressure_scale - 0.1) < 1e-9);
    feed(h, now, 100, 520.0);
    CHECK(h.calibrate(1, false, 60.0).ok);      // another reading, another scale
    CHECK(std::fabs(h.settings().pressure_scale - 0.12) < 1e-9);
    feed(h, now, 100, 520.0);
    CHECK(h.calibrate(1, false, 0.5).why == "bad_reading");     // a known vacuum that is nothing
    feed(h, now, 100, 20.0);
    CHECK(h.calibrate(1, false, 60.0).why == "bad_reading");    // no vacuum on the sensor: nothing to scale by
    CHECK(h.calibrate(9, true, 0).why == "bad_nozzle");
}

TEST(a_calibration_is_refused_while_a_valve_is_open) {
    Settings s;
    s.calibration_enabled = true;
    Head h(s);
    std::uint32_t now = 1000;
    h.heard(now);
    feed(h, now, 100, 0.0);
    CHECK(h.pick(1, now).ok);
    CHECK(h.calibrate(1, true, 0).why == "busy");
    CHECK(h.calibrate(2, true, 0).why == "busy");  // the pump is running for the other
}

TEST(the_example_file_is_the_defaults_names_every_key_and_has_no_problem) {
    std::ifstream in(std::string(PNP_HEAD_ROOT) + "/head.cfg.example", std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string example = ss.str();
    CHECK(!example.empty());
    Settings s;
    CHECK(parse_settings(example, s).empty());
    CHECK(settings_text(s) == settings_text(Settings()));
    // every key that the node writes is in the example, and the token is there as a comment
    std::istringstream keys(settings_text(Settings()));
    std::string line;
    while (std::getline(keys, line)) CHECK(example.find("\n" + line.substr(0, line.find(' ')) + " = ") != std::string::npos);
    CHECK(example.find("# token = ") != std::string::npos);
}

TEST(a_comment_after_a_value_is_cut_off_and_a_hash_in_a_token_is_not_possible) {
    Settings s;
    CHECK(parse_settings("nozzles = 3   # a comment\n", s).empty() && s.nozzles == 3);
    Settings t;
    CHECK(parse_settings("token = abc#def\n", t).empty() && t.token == "abc");
}

int main() { return check::run_all("config"); }
