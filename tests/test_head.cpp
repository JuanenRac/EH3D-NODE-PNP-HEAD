// EH3D-NODE-PNP-HEAD - tests/test_head.cpp: the head on a simulated pump, valves, parts and sensors: a pick that holds, a pick that finds nothing, a part that falls, a pump that
// runs too long, a sensor that dies, the interlocks of height, and the safe state when the server is lost. The random numbers have a fixed seed.
#include <cmath>
#include <string>
#include <vector>

#include "../core/head_node.hpp"
#include "check.hpp"

using namespace pnp_head;

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
    // the server asks something now and then, as it does when it is there
    void keep_alive() { node.handle("GET /api/nozzle?n=1 HTTP/1.1", "tok"); }
};

Settings base() {
    Settings s;
    s.token = "tok";
    return s;
}

bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }

// parks the nozzle low, where it picks and places
void go_low(Rig& r, int n = 1, double mm = 3.0) { r.post("/api/z?n=" + std::to_string(n) + "&mm=" + std::to_string(mm)); }

}  // namespace

TEST(a_pick_over_a_part_holds_it_and_runs_the_pump) {
    Rig r(base());
    go_low(r);
    r.plant.part[0] = true;
    Response a = r.post("/api/nozzle/pick?n=1");
    CHECK(a.status == 200);
    CHECK(r.z().phase == Phase::Holding && r.z().last_result == "held");
    CHECK(r.z().kpa >= r.s.vacuum_target_kpa && r.z().kpa < 90);
    CHECK(r.head().pump() && r.z().valve);
    CHECK(r.z().picks == 1 && r.z().failed_picks == 0);
    r.keep_alive();
    r.run_for(1000);  // it goes on holding
    CHECK(r.z().phase == Phase::Holding);
}

TEST(a_pick_over_nothing_fails_closes_the_valve_and_stops_the_pump) {
    Rig r(base());
    go_low(r);
    Response a = r.post("/api/nozzle/pick?n=1");
    CHECK(a.status == 409);
    CHECK(r.z().phase == Phase::Idle && r.z().last_result == "vacuum_not_reached");
    CHECK(!r.z().valve && !r.head().pump());
    CHECK(r.z().failed_picks == 1 && r.z().picks == 0);
    CHECK(r.z().kpa < r.s.vacuum_target_kpa);
}

TEST(a_place_at_the_board_lets_the_part_go_with_a_puff_and_the_pump_stops) {
    Rig r(base());
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    Response a = r.post("/api/nozzle/place?n=1");
    CHECK(a.status == 200);
    CHECK(r.z().phase == Phase::Idle && r.z().last_result == "placed" && r.z().places == 1);
    CHECK(!r.z().valve && !r.z().blow && !r.head().pump());
    CHECK(!r.plant.part[0]);
    r.run_for(500);
    CHECK(r.z().kpa < 5.0);
}

TEST(a_part_is_not_released_from_a_height_and_is_still_held) {
    Rig r(base());
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    CHECK(r.post("/api/z?n=1&mm=20").status == 200);
    Response a = r.post("/api/nozzle/place?n=1");
    eh3d::json::Value v;
    CHECK(a.status == 409 && eh3d::json::parse(a.body, v) && v.string_or("why", "") == "interlock_release_high");
    CHECK(r.z().phase == Phase::Holding && r.z().valve && r.plant.part[0]);
    CHECK(r.post("/api/nozzle/place?n=2").status == 409);  // nothing held there
}

TEST(a_nozzle_is_turned_only_when_high_enough_and_not_lowered_while_turning) {
    Rig r(base());
    CHECK(near(r.z().z, 40.0));
    CHECK(r.post("/api/rotate?n=1&deg=90").status == 200);
    CHECK(near(r.z().a, 90.0));
    go_low(r);
    eh3d::json::Value v;
    Response a = r.post("/api/rotate?n=1&deg=-90");
    CHECK(a.status == 409 && eh3d::json::parse(a.body, v) && v.string_or("why", "") == "interlock_rotate_low");
    CHECK(near(r.z().a, 90.0));
    CHECK(r.post("/api/z?n=1&mm=20").status == 200);
    // a turn is under way when the nozzle is asked to go down: the order is stopped at the door
    CHECK(r.head().rotate(1, 0.0).ok);
    Result down = r.head().move_z(1, 3.0);
    CHECK(!down.ok && down.why == "interlock_rotate_low");
    r.run_for(1000);
    CHECK(near(r.z().a, 0.0));
}

TEST(the_ranges_of_the_axes_are_kept) {
    Rig r(base());
    CHECK(r.post("/api/z?n=1&mm=41").status == 400);
    CHECK(r.post("/api/z?n=1&mm=-1").status == 400);
    CHECK(r.post("/api/rotate?n=1&deg=181").status == 400);
    CHECK(r.post("/api/rotate?n=1&deg=-181").status == 400);
    CHECK(near(r.z().z_target, 40.0) && near(r.z().a_target, 0.0));
    CHECK(r.post("/api/z?n=1&mm=nonsense").status == 400);
}

TEST(a_part_that_falls_while_held_is_a_lost_part_and_a_fault_until_acknowledged) {
    Rig r(base());
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    r.plant.part[0] = false;  // it drops
    r.run_for(600);
    CHECK(r.z().phase == Phase::Fault && r.z().fault == Fault::VacuumLost);
    CHECK(std::string(fault_code(Fault::VacuumLost)) == "HEAD-V02");
    CHECK(!r.z().valve && !r.head().pump() && r.z().losses == 1);
    Response a = r.post("/api/nozzle/pick?n=1");
    CHECK(a.status == 409);
    CHECK(r.post("/api/nozzle/place?n=1").status == 409);
    CHECK(r.post("/api/z?n=1&mm=1").status == 409);          // nothing goes down while the nozzle holds a fault
    CHECK(r.post("/api/z?n=1&mm=30").status == 200);         // and up is allowed
    CHECK(r.post("/api/ack?n=1").status == 200);
    CHECK(r.z().phase == Phase::Idle && r.z().fault == Fault::None);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
}

TEST(a_short_dip_of_the_vacuum_is_not_a_loss) {
    Settings s = base();
    s.vacuum_loss_ms = 300;
    Rig r(s);
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    r.plant.part[0] = false;
    r.run_for(150);        // the vacuum falls under the limit but not for 300 ms
    r.plant.part[0] = true;
    r.run_for(800);
    CHECK(r.z().phase == Phase::Holding);
}

TEST(a_pump_that_runs_too_long_is_stopped_and_the_nozzles_that_used_it_hold_a_fault) {
    Settings s = base();
    s.pump_max_run_s = 5;
    Rig r(s);
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    for (int i = 0; i < 6; ++i) { r.keep_alive(); r.run_for(1000); }
    CHECK(r.z().phase == Phase::Fault && r.z().fault == Fault::PumpOverrun);
    CHECK(std::string(fault_code(Fault::PumpOverrun)) == "HEAD-P01");
    CHECK(!r.head().pump() && !r.z().valve);
    CHECK(r.z(2).phase == Phase::Idle);  // the other nozzle never used it
}

TEST(a_sensor_that_does_not_change_while_a_valve_is_open_is_a_dead_sensor) {
    Rig r(base());
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    r.plant.frozen[0] = true;
    for (int i = 0; i < 4; ++i) { r.keep_alive(); r.run_for(1000); }
    CHECK(r.z().phase == Phase::Fault && r.z().fault == Fault::SensorFault);
    CHECK(!r.z().valve);
}

TEST(a_sensor_that_reads_what_cannot_be_real_stops_that_nozzle_only) {
    Rig r(base());
    go_low(r, 1);
    go_low(r, 2);
    r.plant.crazy[0] = true;
    r.run_for(200);
    CHECK(r.z(1).fault == Fault::SensorFault && r.z(2).fault == Fault::None);
    r.plant.part[1] = true;
    CHECK(r.post("/api/nozzle/pick?n=2").status == 200);
    CHECK(r.post("/api/nozzle/pick?n=1").status == 409);
    CHECK(r.post("/api/ack?n=1").status == 409);  // the cause is still there
    r.plant.crazy[0] = false;
    r.run_for(200);
    CHECK(r.post("/api/ack?n=1").status == 200);
}

TEST(a_pump_that_does_not_pull_gives_a_failed_pick_even_over_a_part) {
    Rig r(base());
    go_low(r);
    r.plant.part[0] = true;
    r.plant.pump_pulls = false;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 409);
    CHECK(r.z().last_result == "vacuum_not_reached" && !r.head().pump());
}

TEST(two_nozzles_share_the_pump_and_it_stops_when_the_last_valve_closes) {
    Rig r(base());
    go_low(r, 1);
    go_low(r, 2);
    r.plant.part[0] = true;
    r.plant.part[1] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    CHECK(r.post("/api/nozzle/pick?n=2").status == 200);
    CHECK(r.z(1).phase == Phase::Holding && r.z(2).phase == Phase::Holding);
    CHECK(r.post("/api/nozzle/pick?n=1").status == 409);  // busy
    CHECK(r.post("/api/nozzle/place?n=1").status == 200);
    CHECK(r.head().pump() && r.z(2).phase == Phase::Holding && r.z(2).kpa > 40);  // the second one still holds
    CHECK(r.post("/api/nozzle/place?n=2").status == 200);
    CHECK(!r.head().pump());
}

TEST(a_stop_ends_the_moves_and_does_not_drop_the_part) {
    Rig r(base());
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    CHECK(r.head().move_z(1, 20.0).ok);
    r.run_for(100);
    CHECK(r.post("/api/stop").status == 200);
    double z = r.z().z;
    r.run_for(1000);
    CHECK(near(r.z().z, z) && r.z().phase == Phase::Holding && r.head().pump());
}

TEST(a_blow_alone_is_refused_while_a_part_is_held) {
    Rig r(base());
    go_low(r);
    CHECK(r.post("/api/nozzle/blow?n=1").status == 200);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    eh3d::json::Value v;
    Response a = r.post("/api/nozzle/blow?n=1");
    CHECK(a.status == 409 && eh3d::json::parse(a.body, v) && v.string_or("why", "") == "holding");
}

TEST(when_the_server_is_lost_the_pump_stops_the_valves_close_and_the_nozzles_go_up) {
    Rig r(base());
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    CHECK(r.post("/api/light?up=50&down=20").status == 200);
    r.run_for(static_cast<std::uint32_t>(r.s.link_lost_ms) + 2000);
    CHECK(r.head().safe() && r.head().safe_reason() == "server_lost");
    CHECK(!r.head().pump() && !r.z().valve);
    CHECK(r.z().phase == Phase::Idle && r.z().last_result == "safe_state");
    CHECK(near(r.z().z, r.s.z_safe_mm));
    CHECK(r.head().light_up() == 0 && r.head().light_down() == 0);
    CHECK(r.post("/api/nozzle/pick?n=1").status == 409);
    CHECK(r.post("/api/light?up=10").status == 409);
    CHECK(r.post("/api/z?n=1&mm=1").status == 409);
    CHECK(r.post("/api/z?n=1&mm=38").status == 200);  // up is allowed
}

TEST(a_person_acknowledges_the_safe_state_and_the_head_works_again) {
    Rig r(base());
    r.run_for(static_cast<std::uint32_t>(r.s.link_lost_ms) + 500);
    CHECK(r.head().safe());
    eh3d::json::Value v;
    Response a = r.post("/api/ack");  // the request itself is the proof that the server is back
    CHECK(a.status == 200 && eh3d::json::parse(a.body, v) && v.string_or("why", "") == "cleared");
    CHECK(!r.head().safe());
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    CHECK(r.post("/api/ack").status == 200);  // nothing is held now
}

TEST(with_the_safe_vacuum_on_hold_the_part_is_kept_when_the_server_is_lost) {
    Settings s = base();
    s.safe_vacuum = SafeVacuum::Hold;
    Rig r(s);
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    r.run_for(static_cast<std::uint32_t>(r.s.link_lost_ms) + 2000);
    CHECK(r.head().safe() && r.z().phase == Phase::Holding && r.head().pump());
    CHECK(near(r.z().z, r.s.z_safe_mm));  // it still goes up
}

TEST(without_the_retract_the_nozzles_stay_where_they_are) {
    Settings s = base();
    s.safe_z_retract = false;
    Rig r(s);
    go_low(r);
    r.run_for(static_cast<std::uint32_t>(r.s.link_lost_ms) + 500);
    CHECK(r.head().safe() && near(r.z().z, 3.0));
}

TEST(a_server_that_keeps_asking_keeps_the_head_awake) {
    Rig r(base());
    go_low(r);
    r.plant.part[0] = true;
    CHECK(r.post("/api/nozzle/pick?n=1").status == 200);
    for (int i = 0; i < 60; ++i) { r.run_for(1000); r.keep_alive(); }
    CHECK(!r.head().safe());
    CHECK(r.z().phase == Phase::Holding);
}

TEST(the_lights_follow_the_request_and_keep_the_one_that_is_left_out) {
    Rig r(base());
    CHECK(r.post("/api/light?up=40").status == 200);
    CHECK(r.head().light_up() == 40 && r.head().light_down() == 0);
    CHECK(r.post("/api/light?down=70").status == 200);
    CHECK(r.head().light_up() == 40 && r.head().light_down() == 70);
    CHECK(r.post("/api/light?up=101").status == 400);
    CHECK(r.head().light_up() == 40);
}

TEST(two_hundred_cycles_with_a_few_missing_parts_add_up_and_are_the_same_every_time) {
    auto run = [](std::uint32_t seed, unsigned long& picks, unsigned long& failed, unsigned long& places) {
        Rig r(base(), seed);
        go_low(r);
        Rng chance(seed + 1);
        for (int i = 0; i < 200; ++i) {
            r.plant.part[0] = (chance.next() % 100) >= 5;  // one part in twenty is missing from the tape
            Response a = r.post("/api/nozzle/pick?n=1");
            if (a.status == 200) r.post("/api/nozzle/place?n=1");
            if (i % 10 == 0) r.keep_alive();
            r.keep_alive();
        }
        picks = r.z().picks; failed = r.z().failed_picks; places = r.z().places;
    };
    unsigned long p1 = 0, f1 = 0, c1 = 0, p2 = 0, f2 = 0, c2 = 0, p3 = 0, f3 = 0, c3 = 0;
    run(11, p1, f1, c1);
    run(11, p2, f2, c2);
    run(12, p3, f3, c3);
    CHECK(p1 + f1 == 200 && c1 == p1);
    CHECK(f1 > 0 && f1 < 40);
    CHECK(p1 == p2 && f1 == f2 && c1 == c2);
    CHECK(p1 != p3 || f1 != f3);
}

TEST(a_head_with_no_command_for_a_minute_has_done_nothing) {
    Rig r(base());
    for (int i = 0; i < 6; ++i) { r.run_for(10000); r.keep_alive(); }
    CHECK(!r.head().pump() && !r.z(1).valve && !r.z(2).valve);
    CHECK(near(r.z(1).z, 40.0) && near(r.z(1).a, 0.0));
    CHECK(r.z(1).picks == 0 && r.z(1).places == 0);
}

int main() { return check::run_all("head"); }
