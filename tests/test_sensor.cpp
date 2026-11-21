// EH3D-NODE-PNP-HEAD - tests/test_sensor.cpp: the two ways the vacuum of a nozzle is sensed (a pressure sensor read as a number, a vacuum switch read as a level), the
// states and alarms they raise, and the interlock that lets the machine move in X and Y only with every nozzle raised. The random numbers have a fixed seed.
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../core/head_node.hpp"
#include "check.hpp"

using namespace pnp_head;

#ifndef PNP_HEAD_ROOT
#error "PNP_HEAD_ROOT must be the folder of the repository"
#endif

namespace {

struct Rig {
    Settings s;
    SimPlant plant;
    Node node;
    explicit Rig(Settings in, std::uint32_t seed = 5) : s(in), plant(s, seed), node(s, plant) {
        plant.attach(&node.head());
        node.start();
    }
    Head& head() { return node.head(); }
    const Nozzle& z(int n = 1) { return node.head().nozzle(n); }
    void run_for(std::uint32_t ms) { node.step(ms); }
    Response post(const std::string& target) { return node.handle("POST " + target + " HTTP/1.1", "tok"); }
    Response get(const std::string& target) { return node.handle("GET " + target + " HTTP/1.1", "tok"); }
    void keep_alive() { node.handle("GET /api/nozzle?n=1 HTTP/1.1", "tok"); }
    void low(int n = 1, double mm = 3.0) { post("/api/z?n=" + std::to_string(n) + "&mm=" + std::to_string(mm)); }
    std::string why(const Response& r) {
        eh3d::json::Value v;
        return eh3d::json::parse(r.body, v) ? v.string_or("why", "?") : "unparsed";
    }
};

Settings base() {
    Settings s;
    s.token = "tok";
    return s;
}

/// Nozzle 1 on a vacuum switch, the others on pressure sensors.
Settings switch_on_first() {
    Settings s = base();
    s.sensor[0] = SensorKind::Digital;
    return s;
}

std::string read_file(const std::string& rel) {
    std::ifstream in(std::string(PNP_HEAD_ROOT) + "/" + rel, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// ticks a head for `ms` with the same raw reading on every sensor
void feed(Head& h, std::uint32_t& now, std::uint32_t ms, double raw) {
    double r[kMaxNozzles] = {raw, raw, raw, raw, raw, raw};
    for (std::uint32_t t = 0; t < ms; t += 10) { now += 10; h.tick(now, 10, r); }
}

}  // namespace

// --- the settings -----------------------------------------------------------------------------------------------------------------------------------------------------------

TEST(each_nozzle_has_its_own_sensor_kind_in_the_settings) {
    Settings d;
    for (int i = 0; i < kMaxNozzles; ++i) CHECK(d.sensor[i] == SensorKind::Analog);
    Settings s;
    CHECK(parse_settings("nozzle_1_sensor = digital\nnozzle_3_sensor = digital\nnozzle_6_sensor = analog\n", s).empty());
    CHECK(s.sensor[0] == SensorKind::Digital && s.sensor[1] == SensorKind::Analog && s.sensor[2] == SensorKind::Digital && s.sensor[5] == SensorKind::Analog);
    Settings t;
    CHECK(parse_settings(settings_text(s), t).empty());
    for (int i = 0; i < kMaxNozzles; ++i) CHECK(t.sensor[i] == s.sensor[i]);   // read back the same
    CHECK(settings_text(s).find("nozzle_3_sensor = digital") != std::string::npos);
}

TEST(a_sensor_kind_nobody_knows_or_a_nozzle_that_does_not_exist_is_refused) {
    for (const char* line : {"nozzle_1_sensor = pressure", "nozzle_1_sensor = ", "nozzle_1_sensor = Digital", "nozzle_7_sensor = digital", "nozzle_0_sensor = digital", "nozzle_a_sensor = digital",
                             "nozzle_10_sensor = digital", "nozzle_1_sensors = digital", "digital_debounce_ms = 0", "digital_debounce_ms = 501", "digital_active_high = maybe",
                             "interlock_z_tolerance_mm = -1", "interlock_z_tolerance_mm = 6"}) {
        Settings s;
        CHECK(!parse_settings(std::string(line) + "\n", s).empty());
        CHECK(settings_text(s) == settings_text(Settings()));   // and the default stays
    }
    Settings s;
    std::vector<Problem> p = parse_settings("nozzle_7_sensor = digital\n", s);
    CHECK(p.size() == 1 && p[0].code == "unknown_key");
}

TEST(a_debounce_as_long_as_the_pick_is_put_back_to_the_default) {
    Settings s;
    std::vector<Problem> p = parse_settings("digital_debounce_ms = 500\nvacuum_settle_ms = 400\n", s);   // a level that needs longer than the pick is allowed could never be seen
    CHECK(p.size() == 1 && p[0].key == "digital_debounce_ms");
    CHECK(s.digital_debounce_ms == Settings().digital_debounce_ms && s.vacuum_settle_ms == Settings().vacuum_settle_ms);
    Settings t;
    CHECK(parse_settings("digital_debounce_ms = 100\nvacuum_settle_ms = 400\n", t).empty() && t.digital_debounce_ms == 100 && t.vacuum_settle_ms == 400);
}

TEST(the_example_file_names_the_sensor_of_every_nozzle_and_the_keys_of_the_switches) {
    const std::string example = read_file("head.cfg.example");
    for (int n = 1; n <= kMaxNozzles; ++n) CHECK(example.find("\nnozzle_" + std::to_string(n) + "_sensor = analog") != std::string::npos);
    for (const char* key : {"digital_active_high", "digital_debounce_ms", "interlock_z_tolerance_mm"}) CHECK(example.find(std::string("\n") + key + " = ") != std::string::npos);
    CHECK(example.find("analog or digital") != std::string::npos);
    CHECK(example.find("rotate_min_z_mm is not used") != std::string::npos);
}

// --- the vacuum switch --------------------------------------------------------------------------------------------------------------------------------------------------------

TEST(a_digital_sensor_holds_a_part_the_same_way_and_says_no_pressure) {
    Rig r(switch_on_first());
    r.low();
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    CHECK(r.z().phase == Phase::Holding && r.z().last_result == "held" && r.z().vac_ok && r.z().picks == 1);
    eh3d::json::Value v;
    CHECK(eh3d::json::parse(r.get("/api/nozzle?n=1").body, v));
    const eh3d::json::Value* kpa = v.get("kpa");
    CHECK(kpa != nullptr && kpa->type == eh3d::json::Type::kNull);   // a switch does not measure
    CHECK(v.string_or("sensor", "") == "digital" && v.bool_or("vacuum_ok", false) && v.bool_or("part", false));
    r.keep_alive();
    r.run_for(1500);
    CHECK(r.z().phase == Phase::Holding);   // and goes on holding
}

TEST(a_digital_pick_over_nothing_fails_and_leaves_the_alarm_of_the_pick) {
    Rig r(switch_on_first());
    r.low();
    Response a = r.post("/api/nozzle/pick?n=1");
    CHECK(a.status == 409 && r.why(a) == "vacuum_not_reached");
    CHECK(r.z().phase == Phase::Idle && !r.z().valve && !r.head().pump());   // the valve closed again and the pump stopped
    CHECK(r.z().alarm == Alarm::VacuumNotReached && std::string(alarm_code(r.z().alarm)) == "HEAD-V01" && r.z().failed_picks == 1);
    eh3d::json::Value v;
    CHECK(eh3d::json::parse(r.get("/api/nozzle?n=1").body, v) && v.string_or("alarm", "") == "vacuum_not_reached" && v.string_or("alarm_code", "") == "HEAD-V01" && v.string_or("fault", "") == "none");
    CHECK(eh3d::json::parse(r.node.handle("GET /api/status HTTP/1.1", "").body, v) && v.string_or("selfcheck_codes", "").find("HDN-V01") != std::string::npos);
    r.plant.part[0] = true;   // the next pick starts clean and the alarm goes
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    CHECK(r.z().alarm == Alarm::None && r.z().phase == Phase::Holding);
}

TEST(a_digital_part_that_falls_is_lost_after_the_loss_time_and_the_nozzle_holds_the_fault) {
    Rig r(switch_on_first());
    r.low();
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    r.plant.part[0] = false;   // it falls: the vacuum leaks to the open-nozzle level and the switch lets go
    r.keep_alive();
    r.run_for(600);
    CHECK(r.z().phase == Phase::Fault && r.z().fault == Fault::VacuumLost && r.z().alarm == Alarm::PartLost && r.z().losses == 1);
    CHECK(!r.z().valve && std::string(alarm_code(r.z().alarm)) == "HEAD-V02");
    CHECK(r.post("/api/nozzle/pick?n=1").status == 409);   // no pick until a person acknowledges
    CHECK(r.post("/api/ack?n=1").status == 200 && r.z().alarm == Alarm::None && r.z().phase == Phase::Idle);
}

TEST(a_bouncing_switch_is_filtered_by_the_debounce) {
    Rig r(switch_on_first());
    r.plant.bounce_ms = 8;    // the level flickers for a moment after each change, less than the debounce
    r.low();
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    r.keep_alive();
    r.run_for(2000);
    CHECK(r.z().phase == Phase::Holding && r.z().picks == 1 && r.z().losses == 0 && r.z().fault == Fault::None);
}

TEST(an_active_low_switch_is_read_with_the_other_polarity) {
    Settings s = switch_on_first();
    s.digital_active_high = false;
    Rig r(s);
    r.low();
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200 && r.z().phase == Phase::Holding);
    r.plant.part[0] = false;
    r.keep_alive();
    r.run_for(600);
    CHECK(r.z().fault == Fault::VacuumLost);
    Rig q(s);   // and with nothing pulling the idle level is the other one, not a vacuum
    q.run_for(5000);
    CHECK(q.z().fault == Fault::None && !q.z().vac_ok);
}

TEST(a_level_between_high_and_low_is_a_sensor_out_of_range_until_it_is_gone) {
    Rig r(switch_on_first());
    r.low();
    r.plant.crazy[0] = true;
    r.run_for(100);
    CHECK(r.z().phase == Phase::Fault && r.z().fault == Fault::SensorFault && r.z().alarm == Alarm::SensorRange);
    CHECK(std::string(alarm_code(r.z().alarm)) == "HEAD-S01" && r.z(2).fault == Fault::None);   // that nozzle only
    CHECK(r.post("/api/ack?n=1").status == 409 && r.why(r.post("/api/ack?n=1")) == "still_active");
    r.plant.crazy[0] = false;
    r.run_for(100);
    CHECK(r.post("/api/ack?n=1").status == 200 && r.z().fault == Fault::None);
}

TEST(a_switch_that_says_vacuum_with_nothing_pulling_is_a_stuck_switch) {
    Rig r(switch_on_first());
    r.plant.stuck_on[0] = true;
    for (int i = 0; i < 4; ++i) { r.keep_alive(); r.run_for(1000); }
    CHECK(r.z().phase == Phase::Fault && r.z().fault == Fault::SensorFrozen && r.z().alarm == Alarm::SensorFrozen);
    CHECK(std::string(alarm_code(r.z().alarm)) == "HEAD-S02" && std::string(fault_code(r.z().fault)) == "HEAD-S02");
    CHECK(r.why(r.post("/api/ack?n=1")) == "still_active");   // it still says vacuum
    r.plant.stuck_on[0] = false;
    r.run_for(500);
    CHECK(r.post("/api/ack?n=1").status == 200);
    CHECK(r.z().fault == Fault::None);
}

TEST(a_switch_that_does_not_change_is_not_a_fault_while_a_part_is_held) {
    Rig r(switch_on_first());
    r.low();
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    for (int i = 0; i < 5; ++i) { r.keep_alive(); r.run_for(1000); }   // the level is the same for longer than pressure_stuck_ms, as it should be
    CHECK(r.z().phase == Phase::Holding && r.z().fault == Fault::None);
}

// --- the pressure sensor ------------------------------------------------------------------------------------------------------------------------------------------------------

TEST(an_analog_nozzle_keeps_its_band_between_the_target_and_the_hold_limit) {
    Settings s = base();
    Head h(s);
    std::uint32_t now = 1000;
    feed(h, now, 100, 600.0);   // 60 kPa: above the target of 50
    CHECK(h.nozzle(1).vac_ok);
    feed(h, now, 100, 400.0);   // 40 kPa: inside the band, between the hold limit of 35 and the target: it stays as it was
    CHECK(h.nozzle(1).vac_ok);
    feed(h, now, 100, 300.0);   // 30 kPa: under the hold limit
    CHECK(!h.nozzle(1).vac_ok);
    feed(h, now, 100, 400.0);   // back inside the band: still no, the target is not reached
    CHECK(!h.nozzle(1).vac_ok);
    feed(h, now, 100, 600.0);
    CHECK(h.nozzle(1).vac_ok);
}

TEST(the_filter_of_an_analog_nozzle_averages_the_readings_and_a_switch_has_none) {
    Settings s = base();
    s.pressure_samples = 4;
    Head h(s);
    std::uint32_t now = 1000;
    feed(h, now, 100, 0.0);
    double r[kMaxNozzles] = {400.0, 400.0, 0, 0, 0, 0};
    now += 10;
    h.tick(now, 10, r);   // one reading of 40 kPa among four: the average is a quarter of it
    CHECK(h.nozzle(1).kpa > 9.0 && h.nozzle(1).kpa < 11.0);
    Settings d = base();
    d.sensor[0] = SensorKind::Digital;
    Head g(d);
    now = 1000;
    feed(g, now, 100, 0.0);
    double hi[kMaxNozzles] = {1.0, 1.0, 0, 0, 0, 0};
    now += 10;
    g.tick(now, 10, hi);
    CHECK(g.nozzle(1).kpa == 0.0 && !g.nozzle(1).vac_ok);   // not yet: the level has not stayed
    feed(g, now, 30, 1.0);
    CHECK(g.nozzle(1).vac_ok);                                // a few readings later it has
}

TEST(the_calibration_is_for_the_analog_sensors_only) {
    Settings s = switch_on_first();
    s.calibration_enabled = true;
    Rig r(s);
    r.run_for(200);
    CHECK(r.why(r.post("/api/vacuum/calibrate?n=1&kind=zero")) == "not_analog");
    CHECK(r.post("/api/vacuum/calibrate?n=1&kind=zero").status == 409);
    CHECK(r.post("/api/vacuum/calibrate?n=2&kind=zero").status == 200);   // the other nozzle has a pressure sensor
    CHECK(r.head().calibrated());
}

TEST(a_frozen_pressure_sensor_and_one_out_of_range_have_their_own_alarms) {
    Rig r(base());
    r.low(1);
    r.low(2);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    r.plant.frozen[0] = true;
    r.plant.crazy[1] = true;
    for (int i = 0; i < 4; ++i) { r.keep_alive(); r.run_for(1000); }
    CHECK(r.z(1).fault == Fault::SensorFrozen && r.z(1).alarm == Alarm::SensorFrozen && r.z(1).last_result == "sensor_frozen");
    CHECK(r.z(2).fault == Fault::SensorFault && r.z(2).alarm == Alarm::SensorRange && r.z(2).last_result == "sensor_fault");
}

TEST(an_analog_pick_over_nothing_leaves_the_same_alarm_and_ack_clears_it) {
    Rig r(base());
    r.low();
    CHECK(r.post("/api/nozzle/pick?n=1").status == 409);
    CHECK(r.z().alarm == Alarm::VacuumNotReached && r.z().phase == Phase::Idle);
    CHECK(r.why(r.post("/api/ack?n=1")) == "cleared" && r.z().alarm == Alarm::None);
    CHECK(r.why(r.post("/api/ack?n=1")) == "not_held");   // nothing left to clear
}

TEST(digital_and_analog_nozzles_work_together_on_one_head) {
    Rig r(switch_on_first());
    r.low(1);
    r.low(2);
    r.plant.part[0] = true;
    r.plant.part[1] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    CHECK(r.post("/api/nozzle/pick?n=2").status == 200);
    CHECK(r.z(1).phase == Phase::Holding && r.z(2).phase == Phase::Holding);
    eh3d::json::Value a, b;
    CHECK(eh3d::json::parse(r.get("/api/nozzle?n=1").body, a) && eh3d::json::parse(r.get("/api/nozzle?n=2").body, b));
    CHECK(a.get("kpa")->type == eh3d::json::Type::kNull && b.get("kpa")->type != eh3d::json::Type::kNull && b.number_or("kpa", 0) >= r.s.vacuum_target_kpa);
    CHECK(r.post("/api/nozzle/place?n=1").status == 200 && r.post("/api/nozzle/place?n=2").status == 200);
}

TEST(the_same_seed_gives_the_same_run_with_a_switch_and_another_seed_another) {
    auto run = [](std::uint32_t seed) {
        Settings s = switch_on_first();
        Rig r(s, seed);
        r.plant.bounce_ms = 8;
        r.low(1);
        r.low(2);
        r.plant.part[0] = true;
        r.plant.part[1] = true;
        for (int i = 0; i < 3; ++i) {
            r.post("/api/nozzle/pick?n=1");
            r.post("/api/nozzle/pick?n=2");
            r.keep_alive();
            r.run_for(300);
            r.plant.part[0] = true;
            r.plant.part[1] = true;
            r.post("/api/nozzle/place?n=1");
            r.post("/api/nozzle/place?n=2");
        }
        return std::to_string(r.z(1).picks) + "," + std::to_string(r.z(1).failed_picks) + "," + std::to_string(r.z(2).kpa) + "," + std::to_string(r.node.head().pump_run_ms());
    };
    CHECK(run(7) == run(7));
    bool differs = false;
    for (std::uint32_t seed = 8; seed < 14; ++seed) if (run(seed) != run(7)) differs = true;
    CHECK(differs);
}

// --- the interlock of X and Y -------------------------------------------------------------------------------------------------------------------------------------------------

TEST(the_machine_may_move_in_xy_only_when_every_nozzle_is_raised_to_the_safe_height) {
    Rig r(base());   // the nozzles park at the top, above the safe height
    eh3d::json::Value v;
    CHECK(eh3d::json::parse(r.node.handle("GET /api/status HTTP/1.1", "").body, v));
    CHECK(v.bool_or("xy_clear", false) && v.string_or("xy_blocking", "?").empty() && v.number_or("z_safe_mm", 0) == 35.0 && !v.bool_or("xy_moving", true));
    CHECK(r.why(r.post("/api/xy?moving=1")) == "ok" && r.head().xy_moving());
    CHECK(r.post("/api/xy?moving=0").status == 200 && !r.head().xy_moving());
    r.low(2, 3.0);   // nozzle 2 goes down to pick
    CHECK(!r.head().xy_clear() && r.head().xy_blocking() == "2" && !r.head().raised(2) && r.head().raised(1));
    Response no = r.post("/api/xy?moving=1");
    CHECK(no.status == 409 && r.why(no) == "interlock_xy_nozzle_low" && !r.head().xy_moving());
    CHECK(eh3d::json::parse(r.node.handle("GET /api/status HTTP/1.1", "").body, v) && !v.bool_or("xy_clear", true) && v.string_or("xy_blocking", "") == "2");
    r.low(1, 3.0);
    CHECK(r.head().xy_blocking() == "1,2");
    r.low(1, 36.0);
    r.low(2, 36.0);
    CHECK(r.head().xy_clear() && r.post("/api/xy?moving=1").status == 200);
}

TEST(the_safe_height_has_a_tolerance_and_a_nozzle_is_raised_by_where_it_goes_too) {
    Settings s = base();
    s.interlock_z_tolerance_mm = 0.5;
    Rig r(s);
    r.low(1, 34.6);   // 0.4 under the safe height of 35: inside the tolerance
    CHECK(r.head().raised(1));
    r.low(1, 34.4);   // 0.6 under it
    CHECK(!r.head().raised(1));
    Settings tight = base();
    tight.interlock_z_tolerance_mm = 0.0;
    Rig q(tight);
    q.low(1, 34.9);
    CHECK(!q.head().raised(1));
    // a nozzle that has been told to go down is not raised even before it has moved
    Head h(base());
    CHECK(h.raised(1));
    h.move_z(1, 10.0);
    CHECK(h.nozzle(1).z == 40.0 && !h.raised(1) && !h.xy_clear());
}

TEST(while_the_machine_moves_in_xy_no_nozzle_is_lowered_under_the_safe_height) {
    Rig r(base());
    CHECK(r.post("/api/xy?moving=1").status == 200);
    Response a = r.post("/api/z?n=1&mm=20");
    CHECK(a.status == 409 && r.why(a) == "interlock_xy_moving" && r.z().z_target == 40.0);
    CHECK(r.post("/api/z?n=1&mm=36").status == 200);   // above the safe height is fine
    CHECK(r.post("/api/z?n=2&mm=34.6").status == 200);  // inside the tolerance
    CHECK(r.why(r.post("/api/z?n=2&mm=0")) == "interlock_xy_moving");
    CHECK(r.post("/api/xy?moving=0").status == 200);
    CHECK(r.post("/api/z?n=1&mm=20").status == 200);   // the machine has stopped
}

TEST(rotate_min_z_does_not_decide_the_xy_interlock) {
    for (double rotate_min : {0.0, 10.0, 30.0, 39.0}) {
        Settings s = base();
        s.rotate_min_z_mm = rotate_min;
        Rig r(s);
        r.low(1, 20.0);   // above or under the turning height, always under the safe height of 35
        CHECK(!r.head().xy_clear());
        r.low(1, 36.0);   // above the safe height, under a turning height of 39
        CHECK(r.head().xy_clear());
        CHECK(r.post("/api/xy?moving=1").status == 200);
    }
    const std::string page = read_file("docs/CONTRACT.md");
    CHECK(page.find("`rotate_min_z_mm` is the height for turning a nozzle and plays no part in this interlock") != std::string::npos);
}

TEST(a_raised_nozzle_that_holds_a_fault_does_not_block_xy_but_a_low_one_does) {
    Rig r(base());
    r.plant.crazy[0] = true;
    r.run_for(200);
    CHECK(r.z(1).fault == Fault::SensorFault && r.head().xy_clear());   // it is up, so it is no danger
    r.plant.crazy[0] = false;
    r.low(2, 3.0);
    r.plant.crazy[1] = true;
    r.run_for(200);
    CHECK(r.z(2).fault == Fault::SensorFault && !r.head().xy_clear() && r.head().xy_blocking() == "2");   // a faulted nozzle stays where it is
    CHECK(r.post("/api/z?n=2&mm=36").status == 200);   // going up is always allowed
    CHECK(r.head().xy_clear());
}

TEST(the_permission_to_move_in_xy_is_released_by_a_person_after_the_safe_state) {
    Rig r(base());
    CHECK(r.post("/api/xy?moving=1").status == 200);
    r.run_for(static_cast<std::uint32_t>(r.s.link_lost_ms) + 500);   // the server went quiet
    CHECK(r.head().safe() && r.head().xy_moving() && r.head().xy_clear());   // the nozzles went up; the permission is not taken away behind the machine's back
    CHECK(r.post("/api/ack").status == 200);
    CHECK(!r.head().safe() && !r.head().xy_moving());
}

TEST(the_safe_state_without_retract_leaves_a_low_nozzle_blocking_xy) {
    Settings s = base();
    s.safe_z_retract = false;
    Rig r(s);
    r.low(1, 3.0);
    r.run_for(static_cast<std::uint32_t>(r.s.link_lost_ms) + 500);
    CHECK(r.head().safe() && !r.head().xy_clear() && r.head().xy_blocking() == "1");
}

TEST(a_bad_xy_request_is_a_400_and_a_node_without_a_token_refuses_it) {
    Rig r(base());
    eh3d::json::Value v;
    Response a = r.post("/api/xy?moving=2");
    CHECK(a.status == 400 && eh3d::json::parse(a.body, v) && v.string_or("key", "") == "moving");
    CHECK(r.post("/api/xy").status == 400);
    CHECK(r.node.handle("POST /api/xy?moving=1 HTTP/1.1", "").status == 401);
    CHECK(r.node.handle("GET /api/xy?moving=1 HTTP/1.1", "tok").status == 405);
    Settings open;
    Rig q(open);
    CHECK(q.node.handle("POST /api/xy?moving=1 HTTP/1.1", "tok").status == 401 && !q.head().xy_moving());
}

TEST(the_page_of_the_contract_names_the_sensors_the_alarms_and_the_xy_words) {
    const std::string page = read_file("docs/CONTRACT.md");
    for (const char* word : {"## How the vacuum is sensed", "## The alarms of a nozzle", "`nozzle_1_sensor`", "`digital_debounce_ms`", "`digital_active_high`", "`interlock_z_tolerance_mm`",
                             "`POST /api/xy?moving=1|0`", "`xy_clear`", "`xy_blocking`", "`xy_moving`", "`not_analog`", "`HEAD-V01`", "`HEAD-S02`", "`HDN-V01`"})
        CHECK(page.find(word) != std::string::npos);
    const std::string hardware = read_file("docs/HARDWARE.md");
    for (const char* test : {"H-11", "H-12", "H-13", "H-14", "H-15"}) CHECK(hardware.find(test) != std::string::npos);
}

int main() { return check::run_all("sensor"); }
