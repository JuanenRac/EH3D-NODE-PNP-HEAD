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
    CHECK(fault_codes == codes_in(page, "HEAD-", 8));
    for (Fault f : {Fault::VacuumLost, Fault::PumpOverrun, Fault::SensorFault}) CHECK(fault_codes.count(fault_code(f)) == 1);
    for (const char* c : Node::all_codes()) CHECK(std::string(Node::severity(c)).size() > 0);
}

TEST(every_route_and_every_refusal_is_in_the_page) {
    std::string page = read_file("docs/CONTRACT.md");
    for (const char* route : {"/api/status", "/api/nozzle?n=N", "/api/nozzle/pick", "/api/nozzle/place", "/api/nozzle/blow", "/api/z", "/api/rotate", "/api/light", "/api/stop", "/api/ack",
                              "/api/vacuum/calibrate"})
        CHECK(page.find(route) != std::string::npos);
    for (const char* why : {"bad_nozzle", "out_of_range", "safe_state", "fault", "busy", "not_holding", "holding", "interlock_release_high", "interlock_rotate_low", "disabled", "still_active",
                            "no_reading", "bad_reading", "vacuum_not_reached", "held", "placed"})
        CHECK(page.find(std::string("`") + why + "`") != std::string::npos);
}

TEST(the_version_of_the_firmware_is_the_one_of_the_manifest) {
    eh3d::json::Value m;
    CHECK(parse(read_file("eh3d.project.json"), m));
    CHECK(m.string_or("version", "?") == std::string(kVersion));
}

int main() { return check::run_all("node"); }
