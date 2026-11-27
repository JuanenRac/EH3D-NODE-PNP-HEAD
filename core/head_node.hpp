// EH3D-NODE-PNP-HEAD - core/head_node.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the node says on the network. It speaks HTTP on one port, with the same words as the other nodes of the suite (the token as `Authorization: Bearer <token>` or `?token=`):
//   GET  /api/status                       the head and all its nozzles as one line of JSON (always answered)
//   GET  /api/nozzle?n=N                   one nozzle (needs the token)
//   POST /api/nozzle/pick?n=N              opens the valve of the nozzle and answers when the vacuum says whether a part is held
//   POST /api/nozzle/place?n=N             releases the part (valve closed, a short puff) and answers when it is done; only at or below the release height
//   POST /api/nozzle/blow?n=N              a puff of air alone; refused while the nozzle holds a part
//   POST /api/z?n=N&mm=H                   moves the nozzle to the height H and answers when it is there
//   POST /api/rotate?n=N&deg=D             turns the nozzle to the angle D and answers when it is there; only at or above the turning height
//   POST /api/light?up=P&down=Q            the two lights, in percent (a word that is left out stays as it is)
//   POST /api/stop                         stops every move; the vacuum is not touched
//   POST /api/ack[?n=N]                    clears the fault of a nozzle whose cause is gone, or (without n) the safe state of the head
//   POST /api/vacuum/calibrate?n=N&kind=zero|scale[&kpa=K]   sets the zero or the scale of a sensor from its reading; only if the settings allow it and the sensor is analog
//   POST /api/xy?moving=1|0                the machine asks to move in X and Y (granted only while every nozzle is raised to z_safe_mm) or says it stopped
// The words are built here so that the server, the desktop programs and the tests read the same ones. A node without a token refuses every request that moves or changes
// something. If the server is not heard for `link_lost_ms` the head goes to its safe state (see head.hpp).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "auth.hpp"
#include "head.hpp"
#include "head_sim.hpp"
#include "json.hpp"
#include "version.hpp"

namespace pnp_head {

struct Request {
    std::string method;
    std::string path;
    std::string query;
};

struct Response {
    int status = 200;
    std::string body;
};

/// Splits `POST /api/nozzle/pick?n=2 HTTP/1.1` (the first line of a request).
inline bool parse_request_line(const std::string& line, Request& out) {
    std::size_t a = line.find(' ');
    if (a == std::string::npos || a == 0) return false;
    std::size_t b = line.find(' ', a + 1);
    // the version of HTTP ends the line: without it the line was cut short, and `mm=35` may have arrived as `mm=3`
    if (b == std::string::npos || line.compare(b + 1, 5, "HTTP/") != 0) return false;
    std::string target = line.substr(a + 1, b - a - 1);
    if (target.empty() || target[0] != '/') return false;
    out.method = line.substr(0, a);
    std::size_t q = target.find('?');
    out.path = q == std::string::npos ? target : target.substr(0, q);
    out.query = q == std::string::npos ? "" : target.substr(q + 1);
    return true;
}

inline std::string query_value(const std::string& query, const std::string& key) {
    std::size_t pos = 0;
    while (pos <= query.size()) {
        std::size_t end = query.find('&', pos);
        if (end == std::string::npos) end = query.size();
        std::string pair = query.substr(pos, end - pos);
        std::size_t eq = pair.find('=');
        if (eq != std::string::npos && pair.substr(0, eq) == key) return pair.substr(eq + 1);
        pos = end + 1;
    }
    return "";
}

/// Whether the query names `key` at all, with a value or without: a field that is there and cannot be read is refused, never taken as left out.
inline bool query_has(const std::string& query, const std::string& key) {
    std::size_t pos = 0;
    while (pos <= query.size()) {
        std::size_t end = query.find('&', pos);
        if (end == std::string::npos) end = query.size();
        std::string pair = query.substr(pos, end - pos);
        if (pair.substr(0, pair.find('=')) == key) return true;
        pos = end + 1;
    }
    return false;
}

inline std::string error_body(const char* error, const char* key = nullptr) {
    eh3d::json::Writer w;
    w.begin_object().field("error", error);
    if (key) w.field("key", key);
    w.end_object();
    return w.str();
}

class Node {
public:
    Node(const Settings& settings, Io& io) : head_(settings), io_(io) {}

    Head& head() { return head_; }
    const Head& head() const { return head_; }

    void start() {
        started_ = io_.now_ms();
        head_.heard(started_);
    }

    /// Lets time pass in steps of 10 ms: the sensors are read and the head decides. The firmware calls this all the time; the commands call it while they wait.
    void step(std::uint32_t ms) {
        while (ms > 0) {
            std::uint32_t dt = ms > 10 ? 10 : ms;
            io_.wait(dt);
            double raw[kMaxNozzles] = {0, 0, 0, 0, 0, 0};
            io_.read_raw(raw, head_.count());
            head_.tick(io_.now_ms(), dt, raw);
            // after the tick, never before: a silence that was already too long (a link that is gone, a task that did not run) is judged before anything new is heard
            if (waiting_) io_.listen();
            ms -= dt;
        }
    }

    Response handle(const std::string& request_line, const std::string& bearer) {
        Request r;
        if (!parse_request_line(request_line, r)) return {400, error_body("bad_request")};
        const bool status = r.path == "/api/status";
        if (status && r.method != "GET") return {405, error_body("bad_method")};
        if (!status && !known(r)) return {404, error_body("not_found")};
        const Settings& cfg = head_.settings();
        const bool authorized = !cfg.token.empty() && eh3d::auth::same_text(cfg.token, !bearer.empty() ? bearer : query_value(r.query, "token"));
        if (authorized) head_.heard(io_.now_ms());
        if (status) return {200, status_json()};
        if (!authorized) return {401, error_body("unauthorized")};
        if (r.path == "/api/nozzle") {
            if (r.method != "GET") return {405, error_body("bad_method")};
            int n = 0;
            if (!nozzle_of(r, n)) return {400, error_body("bad_request", "n")};
            return {200, nozzle_json(n)};
        }
        if (r.method != "POST") return {405, error_body("bad_method")};
        // while a command waits, the others are heard (they count for the link) but only a stop and letting go of the X and Y axes are carried out
        if (waiting_ && r.path != "/api/stop" && !(r.path == "/api/xy" && query_value(r.query, "moving") == "0")) return finish(Result{false, "busy"}, 0);
        if (r.path == "/api/stop") return finish(head_.stop(), 0);
        if (r.path == "/api/light") {
            long long up = -1, down = -1;
            std::string u = query_value(r.query, "up"), d = query_value(r.query, "down");
            if (query_has(r.query, "up") && !cfg::parse_int(u, 0, 100, up)) return {400, error_body("bad_request", "up")};
            if (query_has(r.query, "down") && !cfg::parse_int(d, 0, 100, down)) return {400, error_body("bad_request", "down")};
            return finish(head_.set_light(static_cast<int>(up), static_cast<int>(down)), 0);
        }
        if (r.path == "/api/xy") {
            const std::string m = query_value(r.query, "moving");
            if (m != "0" && m != "1") return {400, error_body("bad_request", "moving")};
            return finish(head_.xy_move(m == "1"), 0);
        }
        if (r.path == "/api/ack" && !query_has(r.query, "n")) return finish(head_.ack(0, io_.now_ms()), 0);
        int n = 0;
        if (!nozzle_of(r, n)) return {400, error_body("bad_request", "n")};
        if (r.path == "/api/ack") return finish(head_.ack(n, io_.now_ms()), n);
        if (r.path == "/api/nozzle/pick") {
            Result res = head_.pick(n, io_.now_ms());
            if (!res.ok) return finish(res, n);
            // wait for the vacuum to say if a part is held
            const std::uint32_t limit = static_cast<std::uint32_t>(head_.settings().vacuum_settle_ms) + 200;
            for (std::uint32_t t = 0; t < limit && head_.nozzle(n).phase == Phase::Evacuating; t += 10) wait_for(10);
            const Nozzle& z = head_.nozzle(n);
            Result out;
            out.ok = z.phase == Phase::Holding;
            out.why = z.last_result;
            return finish(out, n);
        }
        if (r.path == "/api/nozzle/place" || r.path == "/api/nozzle/blow") {
            Result res = r.path == "/api/nozzle/place" ? head_.place(n, io_.now_ms()) : head_.blow(n, io_.now_ms());
            if (!res.ok) return finish(res, n);
            wait_for(static_cast<std::uint32_t>(head_.settings().blow_ms) + 20);
            // the safe state closes the puff short (unless safe_vacuum is hold): that is not a puff that was done
            if (head_.safe() && head_.settings().safe_vacuum == SafeVacuum::Off) return finish(Result{false, "safe_state"}, n);
            Result out;
            out.ok = head_.nozzle(n).phase == Phase::Idle;
            out.why = out.ok ? head_.nozzle(n).last_result : "not_finished";
            return finish(out, n);
        }
        if (r.path == "/api/z") {
            double mm = 0;
            if (!cfg::parse_double(query_value(r.query, "mm"), -1000, 1000, mm)) return {400, error_body("bad_request", "mm")};
            Result res = head_.move_z(n, mm);
            if (!res.ok) return finish(res, n);
            const double travel = std::fabs(head_.nozzle(n).z_target - head_.nozzle(n).z);
            wait_for(static_cast<std::uint32_t>(travel / head_.settings().z_speed_mm_s * 1000.0) + 20);
            return finish(arrived(head_.nozzle(n).z, mm, n), n);
        }
        if (r.path == "/api/rotate") {
            double deg = 0;
            if (!cfg::parse_double(query_value(r.query, "deg"), -100000, 100000, deg)) return {400, error_body("bad_request", "deg")};
            Result res = head_.rotate(n, deg);
            if (!res.ok) return finish(res, n);
            const double travel = std::fabs(head_.nozzle(n).a_target - head_.nozzle(n).a);
            wait_for(static_cast<std::uint32_t>(travel / head_.settings().rot_speed_deg_s * 1000.0) + 20);
            return finish(arrived(head_.nozzle(n).a, deg, n), n);
        }
        if (r.path == "/api/vacuum/calibrate") {
            const std::string kind = query_value(r.query, "kind");
            if (kind != "zero" && kind != "scale") return {400, error_body("bad_request", "kind")};
            double kpa = 0;
            if (kind == "scale" && !cfg::parse_double(query_value(r.query, "kpa"), 1, 200, kpa)) return {400, error_body("bad_request", "kpa")};
            return finish(head_.calibrate(n, kind == "zero", kpa), n);
        }
        return {404, error_body("not_found")};
    }

    /// The codes behind the verdict of the node on itself.
    std::vector<std::string> selfcheck_codes() const {
        std::vector<std::string> codes;
        if (head_.settings().token.empty()) codes.push_back("HDN-S01");
        if (head_.safe()) codes.push_back("HDN-S02");
        bool faulty = false;
        for (int n = 1; n <= head_.count(); ++n) if (head_.nozzle(n).phase == Phase::Fault) faulty = true;
        if (faulty) codes.push_back("HDN-F01");
        bool short_of_vacuum = false;
        for (int n = 1; n <= head_.count(); ++n) if (head_.nozzle(n).alarm == Alarm::VacuumNotReached) short_of_vacuum = true;
        if (short_of_vacuum) codes.push_back("HDN-V01");
        if (head_.settings().calibration_enabled) codes.push_back("HDN-C01");
        return codes;
    }

    /// Every code the node's check can raise, for the page of the contract and its test.
    static std::vector<const char*> all_codes() { return {"HDN-S01", "HDN-S02", "HDN-F01", "HDN-V01", "HDN-C01"}; }

    static const char* severity(const std::string& code) {
        if (code == "HDN-S02") return "fail";
        if (code == "HDN-C01" || code == "HDN-V01") return "info";
        return "warn";
    }

    std::string status_json() const {
        std::vector<std::string> codes = selfcheck_codes();
        const char* verdict = "ok";
        std::string joined;
        for (const std::string& c : codes) {
            if (!joined.empty()) joined += ",";
            joined += c;
            std::string sev = severity(c);
            if (sev == "fail") verdict = "fail";
            else if (sev == "warn" && std::string(verdict) != "fail") verdict = "warn";
        }
        eh3d::json::Writer w;
        w.begin_object();
        w.field("node", head_.settings().name).field("version", kVersion).field("nozzle_count", head_.count());
        w.field("safe", head_.safe()).field("safe_reason", head_.safe_reason()).field("safe_code", head_.safe() ? "HEAD-L01" : "none");
        // the time of the watch over the server, from the settings the head runs with: whoever asks can tell how often it has to
        w.field("link_lost_ms", head_.settings().link_lost_ms);
        w.field("pump", head_.pump()).field("pump_run_s", static_cast<long long>(head_.pump_run_ms() / 1000u));
        w.field("light_up", head_.light_up()).field("light_down", head_.light_down());
        w.field("xy_clear", head_.xy_clear()).field("xy_moving", head_.xy_moving()).field("xy_blocking", head_.xy_blocking());
        w.key("z_safe_mm").number(head_.settings().z_safe_mm, 2);
        w.field("calibrated", head_.calibrated()).field("calibration_allowed", head_.settings().calibration_enabled);
        w.field("uptime_s", static_cast<long long>((io_.now_ms() - started_) / 1000u));
        w.field("selfcheck", verdict).field("selfcheck_codes", joined);
        w.key("nozzles").begin_array();
        for (int n = 1; n <= head_.count(); ++n) w.raw(nozzle_json(n));
        w.end_array();
        w.end_object();
        return w.str();
    }

    std::string nozzle_json(int n) const {
        const Nozzle& z = head_.nozzle(n);
        eh3d::json::Writer w;
        const bool digital = head_.settings().sensor[n - 1] == SensorKind::Digital;
        w.begin_object().field("n", n).field("phase", phase_text(z.phase)).field("fault", fault_code(z.fault));
        w.field("sensor", sensor_text(head_.settings().sensor[n - 1])).field("alarm", alarm_text(z.alarm)).field("alarm_code", alarm_code(z.alarm));
        w.field("valve", z.valve).field("blow", z.blow).field("part", z.phase == Phase::Holding).field("vacuum_ok", z.vac_ok).field("raised", head_.raised(n));
        w.key("kpa");
        if (digital) w.null(); else w.number(z.kpa, 1);
        w.key("z_mm").number(z.z, 2).key("z_target_mm").number(z.z_target, 2);
        w.key("a_deg").number(z.a, 1).key("a_target_deg").number(z.a_target, 1);
        w.field("last_result", z.last_result);
        w.field("picks", static_cast<long long>(z.picks)).field("failed_picks", static_cast<long long>(z.failed_picks));
        w.field("places", static_cast<long long>(z.places)).field("losses", static_cast<long long>(z.losses));
        w.end_object();
        return w.str();
    }

private:
    /// Time passing inside a command: the same as step(), and the node goes on listening.
    void wait_for(std::uint32_t ms) {
        waiting_ = true;
        step(ms);
        waiting_ = false;
    }

    static bool known(const Request& r) {
        static const char* paths[] = {"/api/nozzle", "/api/nozzle/pick", "/api/nozzle/place", "/api/nozzle/blow", "/api/z", "/api/rotate", "/api/light", "/api/stop", "/api/ack",
                                      "/api/vacuum/calibrate", "/api/xy"};
        for (const char* p : paths) if (r.path == p) return true;
        return false;
    }

    bool nozzle_of(const Request& r, int& n) const {
        long long v = 0;
        if (!cfg::parse_int(query_value(r.query, "n"), 1, head_.count(), v)) return false;
        n = static_cast<int>(v);
        return true;
    }

    /// A move is done only when the axis is where it was asked to go: the safe state or a fault in the middle of it sends the axis elsewhere or stops it.
    Result arrived(double now_at, double wanted, int n) const {
        if (std::fabs(now_at - wanted) < 1e-6) return Result();
        if (head_.safe()) return Result{false, "safe_state"};
        return Result{false, head_.nozzle(n).phase == Phase::Fault ? "fault" : "not_finished"};
    }

    static int status_of(const Result& res) {
        if (res.ok) return 200;
        if (res.why == "bad_nozzle" || res.why == "out_of_range") return 400;
        return 409;
    }

    std::string result_body(const Result& res, int n) const {
        eh3d::json::Writer w;
        w.begin_object().field("ok", res.ok).field("why", res.why);
        if (n > 0) w.field("n", n).key("nozzle").raw(nozzle_json(n));
        w.end_object();
        return w.str();
    }

    Response finish(const Result& res, int n) const { return {status_of(res), result_body(res, n)}; }

    Head head_;
    Io& io_;
    std::uint32_t started_ = 0;
    bool waiting_ = false;
};

}  // namespace pnp_head
