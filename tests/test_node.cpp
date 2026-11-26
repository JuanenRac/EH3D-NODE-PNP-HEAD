// EH3D-NODE-PNP-HEAD - tests/test_node.cpp: the words of the node on the network, its token, and the page of the contract, which this test reads and compares with what the node
// answers, so that the page cannot go stale.
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
    explicit Rig(Settings in, std::uint32_t seed = 3) : s(in), plant(s, seed), node(s, plant) {
        plant.attach(&node.head());
        node.start();
    }
    Response get(const std::string& target, bool with_token = true) { return node.handle("GET " + target + " HTTP/1.1", with_token ? "tok" : ""); }
    Response post(const std::string& target, bool with_token = true) { return node.handle("POST " + target + " HTTP/1.1", with_token ? "tok" : ""); }
};

Settings with_token() {
    Settings s;
    s.token = "tok";
    return s;
}

bool parse(const std::string& text, eh3d::json::Value& v) { return eh3d::json::parse(text, v); }

std::string read_file(const std::string& rel) {
    std::ifstream in(std::string(PNP_HEAD_ROOT) + "/" + rel, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::set<std::string> keys_of_section(const std::string& page, const std::string& heading) {
    std::set<std::string> keys;
    std::size_t at = page.find(heading);
    if (at == std::string::npos) return keys;
    std::size_t end = page.find("\n## ", at + heading.size());
    std::istringstream in(page.substr(at, end == std::string::npos ? std::string::npos : end - at));
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("| `", 0) != 0) continue;
        std::size_t close = line.find('`', 3);
        if (close != std::string::npos) keys.insert(line.substr(3, close - 3));
    }
    return keys;
}

std::set<std::string> codes_in(const std::string& page, const std::string& prefix, std::size_t length) {
    std::set<std::string> codes;
    for (std::size_t at = page.find(prefix); at != std::string::npos; at = page.find(prefix, at + 1)) {
        if (at + length <= page.size()) codes.insert(page.substr(at, length));
    }
    return codes;
}

}  // namespace

TEST(the_status_is_always_answered_and_is_valid_json) {
    Rig r(with_token());
    Response a = r.get("/api/status", false);
    CHECK(a.status == 200);
    eh3d::json::Value v;
    CHECK(parse(a.body, v) && v.is_object());
    CHECK(v.string_or("node", "") == "pnp-head");
    CHECK(v.string_or("version", "") == kVersion);
    CHECK(v.integer_or("nozzle_count", 0, 0, 10) == 2);
    CHECK(v.string_or("selfcheck", "") == "ok" && !v.bool_or("safe", true) && !v.bool_or("pump", true));
    const eh3d::json::Value* nozzles = v.get("nozzles");
    CHECK(nozzles != nullptr && nozzles->is_array() && nozzles->items.size() == 2);
    CHECK(nozzles->items[0].string_or("phase", "") == "idle");
}

TEST(a_node_without_a_token_refuses_everything_that_moves_and_says_so) {
    Settings s;
    Rig r(s);
    CHECK(r.post("/api/nozzle/pick?n=1").status == 401);
    CHECK(r.post("/api/z?n=1&mm=10").status == 401);
    CHECK(r.node.handle("POST /api/stop?token= HTTP/1.1", "").status == 401);
    CHECK(r.get("/api/nozzle?n=1").status == 401);
    eh3d::json::Value v;
    CHECK(parse(r.get("/api/status", false).body, v));
    CHECK(v.string_or("selfcheck", "") == "warn" && v.string_or("selfcheck_codes", "").find("HDN-S01") != std::string::npos);
    CHECK(!r.node.head().pump());
}

TEST(the_token_goes_in_the_header_or_in_the_query_and_a_wrong_one_is_refused) {
    Rig r(with_token());
    CHECK(r.node.handle("GET /api/nozzle?n=1 HTTP/1.1", "tok").status == 200);
    CHECK(r.node.handle("GET /api/nozzle?n=1&token=tok HTTP/1.1", "").status == 200);
    CHECK(r.node.handle("GET /api/nozzle?n=1 HTTP/1.1", "tokk").status == 401);
    CHECK(r.node.handle("GET /api/nozzle?n=1 HTTP/1.1", "to").status == 401);
    CHECK(r.node.handle("GET /api/nozzle?n=1 HTTP/1.1", "").status == 401);
    CHECK(r.node.handle("GET /api/nozzle?n=1&token=nope HTTP/1.1", "").status == 401);
}

TEST(a_bad_request_is_a_400_with_the_word_that_is_wrong) {
    Rig r(with_token());
    eh3d::json::Value v;
    Response a = r.post("/api/nozzle/pick?n=9");
    CHECK(a.status == 400 && parse(a.body, v) && v.string_or("key", "") == "n");
    CHECK(r.post("/api/nozzle/pick").status == 400);
    CHECK(r.post("/api/nozzle/pick?n=0").status == 400);
    Response b = r.post("/api/light?up=200");
    CHECK(b.status == 400 && parse(b.body, v) && v.string_or("key", "") == "up");
    Response c = r.post("/api/vacuum/calibrate?n=1&kind=other");
    CHECK(c.status == 400 && parse(c.body, v) && v.string_or("key", "") == "kind");
    Response d = r.post("/api/vacuum/calibrate?n=1&kind=scale");
    CHECK(d.status == 400 && parse(d.body, v) && v.string_or("key", "") == "kpa");
    Response e = r.post("/api/z?n=1");
    CHECK(e.status == 400 && parse(e.body, v) && v.string_or("key", "") == "mm");
    CHECK(r.node.handle("nonsense", "tok").status == 400);
    CHECK(r.node.handle("GET  HTTP/1.1", "tok").status == 400);
}

TEST(unknown_paths_and_wrong_verbs) {
    Rig r(with_token());
    CHECK(r.get("/api/nothing").status == 404);
    CHECK(r.get("/api/nozzle/pick?n=1").status == 405);
    CHECK(r.post("/api/status").status == 405);
    CHECK(r.post("/api/nozzle?n=1").status == 405);
}

TEST(the_calibration_is_refused_by_default_with_the_reason) {
    Rig r(with_token());
    eh3d::json::Value v;
    Response a = r.post("/api/vacuum/calibrate?n=1&kind=zero");
    CHECK(a.status == 409 && parse(a.body, v) && v.string_or("why", "") == "disabled");
    Settings s = with_token();
    s.calibration_enabled = true;
    Rig q(s);
    q.node.step(200);
    CHECK(q.post("/api/vacuum/calibrate?n=1&kind=zero").status == 200);
    CHECK(parse(q.get("/api/status", false).body, v) && v.bool_or("calibrated", false) && v.bool_or("calibration_allowed", false));
    CHECK(v.string_or("selfcheck_codes", "").find("HDN-C01") != std::string::npos && v.string_or("selfcheck", "") == "ok");
}

TEST(a_nozzle_that_holds_a_fault_and_a_safe_head_show_in_the_check) {
    Rig r(with_token());
    r.plant.crazy[0] = true;
    r.node.step(100);
    eh3d::json::Value v;
    CHECK(parse(r.get("/api/status", false).body, v));
    CHECK(v.string_or("selfcheck", "") == "warn" && v.string_or("selfcheck_codes", "").find("HDN-F01") != std::string::npos);
    Rig q(with_token());
    q.node.step(static_cast<std::uint32_t>(q.s.link_lost_ms) + 500);
    CHECK(parse(q.get("/api/status", false).body, v));
    CHECK(v.bool_or("safe", false) && v.string_or("safe_reason", "") == "server_lost" && v.string_or("safe_code", "") == "HEAD-L01" && v.string_or("selfcheck", "") == "fail");
}

TEST(the_status_words_are_the_words_of_the_page_of_the_contract) {
    std::string page = read_file("docs/CONTRACT.md");
    CHECK(!page.empty());
    Rig r(with_token());
    eh3d::json::Value v;
    CHECK(parse(r.get("/api/status", false).body, v));
    std::set<std::string> in_status(v.names.begin(), v.names.end());
    CHECK(in_status == keys_of_section(page, "## `GET /api/status`"));
    const eh3d::json::Value* nozzles = v.get("nozzles");
    CHECK(nozzles != nullptr && !nozzles->items.empty());
    std::set<std::string> in_nozzle(nozzles->items[0].names.begin(), nozzles->items[0].names.end());
    CHECK(in_nozzle == keys_of_section(page, "## `GET /api/nozzle?n=N`"));
    eh3d::json::Value one;
    CHECK(parse(r.get("/api/nozzle?n=1").body, one));
    CHECK(std::set<std::string>(one.names.begin(), one.names.end()) == in_nozzle);
}

TEST(every_code_is_in_the_page_and_every_code_of_the_page_exists) {
    std::string page = read_file("docs/CONTRACT.md");
    std::set<std::string> node_codes, fault_codes;
    for (const char* c : Node::all_codes()) node_codes.insert(c);
    for (const char* c : all_fault_codes()) fault_codes.insert(c);
    CHECK(node_codes == codes_in(page, "HDN-", 7));
    std::set<std::string> alarm_codes;
    for (const char* c : all_alarm_codes()) alarm_codes.insert(c);
    std::set<std::string> every = fault_codes;
    every.insert(alarm_codes.begin(), alarm_codes.end());
    CHECK(every == codes_in(page, "HEAD-", 8));   // the faults and the alarms are the codes of the page
    for (Alarm a : {Alarm::VacuumNotReached, Alarm::PartLost, Alarm::SensorRange, Alarm::SensorFrozen}) CHECK(alarm_codes.count(alarm_code(a)) == 1);
    for (Fault f : {Fault::VacuumLost, Fault::PumpOverrun, Fault::SensorFault, Fault::SensorFrozen}) CHECK(fault_codes.count(fault_code(f)) == 1);
    for (const char* c : Node::all_codes()) CHECK(std::string(Node::severity(c)).size() > 0);
}

TEST(every_route_and_every_refusal_is_in_the_page) {
    std::string page = read_file("docs/CONTRACT.md");
    for (const char* route : {"/api/status", "/api/nozzle?n=N", "/api/nozzle/pick", "/api/nozzle/place", "/api/nozzle/blow", "/api/z", "/api/rotate", "/api/light", "/api/stop", "/api/ack",
                              "/api/vacuum/calibrate", "/api/xy"})
        CHECK(page.find(route) != std::string::npos);
    for (const char* why : {"bad_nozzle", "out_of_range", "safe_state", "fault", "busy", "not_holding", "holding", "interlock_release_high", "interlock_rotate_low", "disabled", "still_active",
                            "no_reading", "bad_reading", "vacuum_not_reached", "held", "placed", "not_analog", "interlock_xy_nozzle_low", "interlock_xy_moving",
                            "part_lost", "sensor_out_of_range", "sensor_frozen"})
        CHECK(page.find(std::string("`") + why + "`") != std::string::npos);
}

TEST(the_version_of_the_firmware_is_the_one_of_the_manifest) {
    eh3d::json::Value m;
    CHECK(parse(read_file("eh3d.project.json"), m));
    CHECK(m.string_or("version", "?") == std::string(kVersion));
}

TEST(a_move_cut_by_the_safe_state_is_not_answered_as_done) {
    Settings s = with_token();
    s.z_speed_mm_s = 1.0;      // forty millimetres take forty seconds: far longer than link_lost_ms
    s.rot_speed_deg_s = 10.0;  // and half a turn takes eighteen
    Rig r(s);
    eh3d::json::Value v;
    Response down = r.post("/api/z?n=1&mm=0");
    CHECK(down.status == 409 && parse(down.body, v) && !v.bool_or("ok", true) && v.string_or("why", "") == "safe_state");
    CHECK(r.node.head().safe() && r.node.head().nozzle(1).z > 0.0);
    Rig q(s);
    Response turn = q.post("/api/rotate?n=1&deg=180");
    CHECK(turn.status == 409 && parse(turn.body, v) && !v.bool_or("ok", true) && v.string_or("why", "") == "safe_state");
    CHECK(q.node.head().nozzle(1).a < 180.0);
}

TEST(a_move_that_arrives_is_answered_as_done) {
    Rig r(with_token());
    eh3d::json::Value v;
    for (const char* target : {"/api/z?n=1&mm=10.5", "/api/z?n=1&mm=10.5", "/api/z?n=1&mm=40", "/api/rotate?n=1&deg=-33.3", "/api/rotate?n=1&deg=179.9"}) {
        Response a = r.post(target);
        CHECK(a.status == 200 && parse(a.body, v) && v.bool_or("ok", false) && v.string_or("why", "") == "ok");
    }
}

namespace {

// A server that goes on asking while the node is busy: a read with the token every `every_ms`, until `until_ms` of the simulated clock. The clock can also jump once, as when the
// task that runs the head does not get to run for a while.
struct LinkedPlant : SimPlant {
    using SimPlant::SimPlant;
    Node* node = nullptr;
    std::uint32_t every_ms = 500;
    std::uint32_t until_ms = 0xffffffffu;
    std::string bearer = "tok";
    std::uint32_t stall_at_ms = 0, stall_ms = 0;  // 0: never
    std::uint32_t order_at_ms = 0;                // when `order` is sent in the middle of a wait (0: never)
    std::string order;
    Response answer;
    std::uint32_t last = 0;
    std::uint32_t skipped = 0;

    std::uint32_t now_ms() const override { return SimPlant::now_ms() + skipped; }
    void wait(std::uint32_t ms) override {
        SimPlant::wait(ms);
        if (stall_ms != 0 && now_ms() >= stall_at_ms) { skipped += stall_ms; stall_ms = 0; }
    }
    void listen() override {
        if (order_at_ms != 0 && now_ms() >= order_at_ms) { order_at_ms = 0; answer = node->handle(order, "tok"); }
        if (now_ms() > until_ms || now_ms() - last < every_ms) return;
        last = now_ms();
        node->handle("GET /api/nozzle?n=1 HTTP/1.1", bearer);
    }
};

struct Linked {
    Settings s;
    LinkedPlant plant;
    Node node;
    explicit Linked(Settings in) : s(in), plant(s, 3), node(s, plant) {
        plant.node = &node;
        plant.attach(&node.head());
        node.start();
    }
    Response post(const std::string& target) { return node.handle("POST " + target + " HTTP/1.1", "tok"); }
};

// forty millimetres take twenty seconds and half a turn eighteen: far longer than link_lost_ms
Settings slow() {
    Settings s = with_token();
    s.z_speed_mm_s = 2.0;
    s.rot_speed_deg_s = 10.0;
    return s;
}

std::string why_of(const Response& a) {
    eh3d::json::Value v;
    return parse(a.body, v) ? v.string_or("why", "") : "";
}

}  // namespace

TEST(a_move_longer_than_the_link_watch_arrives_while_the_server_is_heard) {
    Linked r(slow());
    const std::uint32_t before = r.plant.now_ms();
    Response down = r.post("/api/z?n=1&mm=20");
    CHECK(down.status == 200 && why_of(down) == "ok");
    Response turn = r.post("/api/rotate?n=1&deg=180");
    CHECK(turn.status == 200 && why_of(turn) == "ok");
    CHECK(r.plant.now_ms() - before > 10u * static_cast<std::uint32_t>(r.s.link_lost_ms));
    CHECK(!r.node.head().safe() && r.node.head().nozzle(1).z == 20.0 && r.node.head().nozzle(1).a == 180.0);
}

TEST(a_vacuum_wait_longer_than_the_link_watch_ends_by_the_vacuum_while_the_server_is_heard) {
    Settings s = with_token();
    s.vacuum_settle_ms = 5000;
    Linked r(s);
    const std::uint32_t before = r.plant.now_ms();
    Response a = r.post("/api/nozzle/pick?n=1");
    CHECK(a.status == 409 && why_of(a) == "vacuum_not_reached");
    CHECK(r.plant.now_ms() - before >= 5000u && !r.node.head().safe());
}

TEST(a_link_lost_half_way_stops_the_move_and_the_nozzle_goes_up) {
    Linked r(slow());
    r.plant.until_ms = r.plant.now_ms() + 3000;
    Response a = r.post("/api/z?n=1&mm=0");
    CHECK(a.status == 409 && why_of(a) == "safe_state");
    CHECK(r.node.head().safe() && r.node.head().safe_reason() == "server_lost");
    CHECK(r.node.head().nozzle(1).z == r.s.z_safe_mm && r.node.head().nozzle(1).z_target == r.s.z_safe_mm);
    Linked q(slow());
    q.plant.until_ms = q.plant.now_ms() + 3000;
    Response turn = q.post("/api/rotate?n=1&deg=180");
    CHECK(turn.status == 409 && why_of(turn) == "safe_state");
    CHECK(q.node.head().nozzle(1).a < 60.0 && q.node.head().nozzle(1).a_target == q.node.head().nozzle(1).a);
}

TEST(a_link_lost_half_way_without_the_retract_stops_the_nozzle_where_it_is) {
    Settings s = slow();
    s.safe_z_retract = false;
    Linked r(s);
    r.plant.until_ms = r.plant.now_ms() + 3000;
    Response a = r.post("/api/z?n=1&mm=0");
    CHECK(a.status == 409 && why_of(a) == "safe_state");
    const double z = r.node.head().nozzle(1).z;
    CHECK(z > 25.0 && z < 35.0 && r.node.head().nozzle(1).z_target == z);
    r.node.step(2000);
    CHECK(r.node.head().nozzle(1).z == z);
}

TEST(a_link_lost_during_the_vacuum_wait_closes_the_valve_and_stops_the_pump) {
    Settings s = with_token();
    s.vacuum_settle_ms = 5000;
    Linked r(s);
    r.plant.until_ms = r.plant.now_ms() + 1000;
    Response a = r.post("/api/nozzle/pick?n=1");
    CHECK(a.status == 409 && why_of(a) == "safe_state");
    CHECK(r.node.head().safe() && !r.node.head().pump() && !r.node.head().nozzle(1).valve);
}

TEST(a_task_that_hangs_ends_in_the_safe_state_even_if_the_server_asks_right_after) {
    Linked r(slow());
    r.plant.every_ms = 100;
    r.plant.stall_at_ms = r.plant.now_ms() + 8000;
    r.plant.stall_ms = static_cast<std::uint32_t>(r.s.link_lost_ms) + 500;
    Response a = r.post("/api/z?n=1&mm=0");
    CHECK(a.status == 409 && why_of(a) == "safe_state");
    CHECK(r.node.head().safe() && r.node.head().nozzle(1).z == r.s.z_safe_mm);
}

TEST(a_read_with_a_wrong_token_during_a_move_does_not_keep_the_link) {
    Linked r(slow());
    r.plant.bearer = "nope";
    Response a = r.post("/api/z?n=1&mm=0");
    CHECK(a.status == 409 && why_of(a) == "safe_state" && r.node.head().safe());
}

TEST(during_a_wait_a_stop_is_carried_out_and_another_order_is_busy) {
    Linked r(slow());
    r.plant.order = "POST /api/light?up=50 HTTP/1.1";
    r.plant.order_at_ms = r.plant.now_ms() + 1000;
    CHECK(r.post("/api/z?n=1&mm=30").status == 200);
    CHECK(r.plant.answer.status == 409 && why_of(r.plant.answer) == "busy" && r.node.head().light_up() == 0);
    r.plant.order = "POST /api/xy?moving=0 HTTP/1.1";
    r.plant.order_at_ms = r.plant.now_ms() + 1000;
    CHECK(r.post("/api/z?n=1&mm=20").status == 200);
    CHECK(r.plant.answer.status == 200);
    r.plant.order = "POST /api/stop HTTP/1.1";
    r.plant.order_at_ms = r.plant.now_ms() + 1000;
    Response a = r.post("/api/z?n=1&mm=0");
    CHECK(r.plant.answer.status == 200);
    CHECK(a.status == 409 && why_of(a) == "not_finished" && r.node.head().nozzle(1).z > 15.0 && !r.node.head().safe());
    CHECK(r.post("/api/light?up=50").status == 200);  // the wait is over: orders are taken again
}

TEST(a_puff_cut_short_by_the_safe_state_is_not_answered_as_done) {
    Settings s = with_token();
    s.blow_ms = 2000;
    s.link_lost_ms = 500;
    Linked r(s);
    r.plant.until_ms = 0;
    Response a = r.post("/api/nozzle/blow?n=1");
    CHECK(a.status == 409 && why_of(a) == "safe_state" && !r.node.head().nozzle(1).blow);
}

int main() { return check::run_all("node"); }
