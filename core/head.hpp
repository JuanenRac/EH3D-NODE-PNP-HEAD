// EH3D-NODE-PNP-HEAD - core/head.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the head does with its nozzles: the logic that the firmware runs and that the simulator runs on a computer. It takes commands and the raw readings of the pressure
// sensors and decides what the pump, the valves, the puffs of air, the Z axes and the rotations must do. No hardware is touched here: the outputs are read by the caller.
//
// The rules:
//  * Nothing runs by itself. With no command the pump is stopped and the valves are closed; the pump runs only while a valve is open, and never longer than pump_max_run_s.
//  * A pick opens the valve of one nozzle and waits for the vacuum: if it reaches the target in time a part is held, if not the pick failed and the valve closes again.
//    A held part whose vacuum falls under the hold limit for vacuum_loss_ms is lost: the valve closes and the nozzle holds a fault until a person acknowledges it.
//  * A part is released (valve closed, a short puff) only when the nozzle is at or below release_max_z_mm; a nozzle is turned only when it is at or above rotate_min_z_mm.
//  * A sensor that reads what cannot be real, or does not change at all while a valve is open, is a fault: the vacuum of that nozzle cannot be trusted.
//  * When the server is not heard for link_lost_ms the head is in its safe state: the pump stops, the valves close (unless safe_vacuum is hold), the nozzles go up and the
//    lights go out. Nothing moves down or takes a part until a person acknowledges.
#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "head_config.hpp"

namespace pnp_head {

enum class Phase { Idle, Evacuating, Holding, Releasing, Fault };
enum class Fault { None, VacuumLost, PumpOverrun, SensorFault };

inline const char* phase_text(Phase p) {
    switch (p) {
        case Phase::Idle: return "idle";
        case Phase::Evacuating: return "evacuating";
        case Phase::Holding: return "holding";
        case Phase::Releasing: return "releasing";
        case Phase::Fault: return "fault";
    }
    return "unknown";
}

/// The code of a fault as the programs read it; a code is never reused for another meaning.
inline const char* fault_code(Fault f) {
    switch (f) {
        case Fault::None: return "none";
        case Fault::VacuumLost: return "HEAD-V02";
        case Fault::PumpOverrun: return "HEAD-P01";
        case Fault::SensorFault: return "HEAD-S01";
    }
    return "none";
}

/// Every code a nozzle can hold, and the one of the safe state, for the page of the contract and its test.
inline std::vector<const char*> all_fault_codes() { return {"HEAD-V02", "HEAD-P01", "HEAD-S01", "HEAD-L01"}; }

struct Result {
    bool ok = true;
    std::string why = "ok";
};

struct Nozzle {
    Phase phase = Phase::Idle;
    Fault fault = Fault::None;
    bool valve = false;
    bool blow = false;
    double z = 0.0, z_target = 0.0;
    double a = 0.0, a_target = 0.0;
    double kpa = 0.0;                     // the filtered reading
    std::string last_result = "none";     // held, vacuum_not_reached, placed, vacuum_lost, ...
    std::uint32_t since_ms = 0;           // since when it is in this phase
    std::uint32_t loss_since_ms = 0;      // since when the held vacuum is under the limit (0: it is not)
    std::uint32_t blow_until_ms = 0;
    std::uint32_t same_since_ms = 0;
    double last_raw = 0.0;
    std::vector<double> ring;
    std::size_t ring_at = 0;
    unsigned long picks = 0, failed_picks = 0, places = 0, losses = 0;
};

class Head {
public:
    explicit Head(const Settings& s) : cfg_(s), nozzles_(static_cast<std::size_t>(s.nozzles)) {
        for (Nozzle& n : nozzles_) { n.z = n.z_target = cfg_.z_max_mm; }  // parked at the top
        light_up_ = cfg_.light_up_pct;
        light_down_ = cfg_.light_down_pct;
    }

    const Settings& settings() const { return cfg_; }
    int count() const { return cfg_.nozzles; }
    const Nozzle& nozzle(int n) const { return nozzles_[static_cast<std::size_t>(n - 1)]; }
    bool pump() const { return pump_; }
    bool safe() const { return safe_; }
    const std::string& safe_reason() const { return safe_reason_; }
    int light_up() const { return light_up_; }
    int light_down() const { return light_down_; }
    bool calibrated() const { return calibrated_; }
    std::uint32_t pump_run_ms() const { return pump_run_ms_; }

    /// The server was heard (a request with the token): from now on its silence is watched.
    void heard(std::uint32_t now_ms) { watching_ = true; last_heard_ = now_ms; }

    // ----- commands -----

    Result pick(int n, std::uint32_t now) {
        Result r;
        if (!valid(n)) return no("bad_nozzle");
        Nozzle& z = at(n);
        if (safe_) return no("safe_state");
        if (z.phase == Phase::Fault) return no("fault");
        if (z.phase != Phase::Idle) return no("busy");
        z.valve = true;
        z.phase = Phase::Evacuating;
        z.since_ms = now;
        z.loss_since_ms = 0;
        z.same_since_ms = now;
        z.last_result = "evacuating";
        update_pump();
        return r;
    }

    Result place(int n, std::uint32_t now) {
        if (!valid(n)) return no("bad_nozzle");
        Nozzle& z = at(n);
        if (safe_) return no("safe_state");
        if (z.phase == Phase::Fault) return no("fault");
        if (z.phase != Phase::Holding) return no("not_holding");
        if (z.z > cfg_.release_max_z_mm + 1e-9) return no("interlock_release_high");
        z.valve = false;
        z.blow = true;
        z.blow_until_ms = now + static_cast<std::uint32_t>(cfg_.blow_ms);
        z.phase = Phase::Releasing;
        z.since_ms = now;
        z.last_result = "placed";
        ++z.places;
        update_pump();
        return Result();
    }

    /// A puff of air alone (to clear a nozzle); refused while the nozzle holds a part.
    Result blow(int n, std::uint32_t now) {
        if (!valid(n)) return no("bad_nozzle");
        Nozzle& z = at(n);
        if (safe_) return no("safe_state");
        if (z.phase == Phase::Fault) return no("fault");
        if (z.phase == Phase::Holding) return no("holding");
        if (z.phase != Phase::Idle) return no("busy");
        z.blow = true;
        z.blow_until_ms = now + static_cast<std::uint32_t>(cfg_.blow_ms);
        z.phase = Phase::Releasing;
        z.since_ms = now;
        z.last_result = "blown";
        return Result();
    }

    Result move_z(int n, double mm) {
        if (!valid(n)) return no("bad_nozzle");
        if (!(mm >= cfg_.z_min_mm && mm <= cfg_.z_max_mm)) return no("out_of_range");
        Nozzle& z = at(n);
        const bool down = mm < z.z_target - 1e-9;
        if (down && safe_) return no("safe_state");
        if (down && z.phase == Phase::Fault) return no("fault");
        // a nozzle that is turning is not lowered under the height where turning is allowed
        if (down && mm < cfg_.rotate_min_z_mm - 1e-9 && std::fabs(z.a - z.a_target) > 1e-9) return no("interlock_rotate_low");
        z.z_target = mm;
        return Result();
    }

    Result rotate(int n, double deg) {
        if (!valid(n)) return no("bad_nozzle");
        if (!(deg >= cfg_.rot_min_deg && deg <= cfg_.rot_max_deg)) return no("out_of_range");
        Nozzle& z = at(n);
        if (safe_) return no("safe_state");
        if (z.phase == Phase::Fault) return no("fault");
        if (z.z < cfg_.rotate_min_z_mm - 1e-9 || z.z_target < cfg_.rotate_min_z_mm - 1e-9) return no("interlock_rotate_low");
        z.a_target = deg;
        return Result();
    }

    /// Stops every move where it is. The vacuum is left as it is: a stop must not drop the parts.
    Result stop() {
        for (Nozzle& z : nozzles_) { z.z_target = z.z; z.a_target = z.a; }
        return Result();
    }

    Result set_light(int up, int down) {
        if ((up != -1 && (up < 0 || up > 100)) || (down != -1 && (down < 0 || down > 100))) return no("out_of_range");
        if (safe_ && (up > 0 || down > 0)) return no("safe_state");
        if (up != -1) light_up_ = up;
        if (down != -1) light_down_ = down;
        return Result();
    }

    /// n = 0: the safe state of the head; n > 0: the fault of one nozzle (only when its cause is gone).
    Result ack(int n, std::uint32_t now) {
        if (n == 0) {
            if (!safe_) return Result{true, "not_held"};
            if (!watching_ || now - last_heard_ > static_cast<std::uint32_t>(cfg_.link_lost_ms)) return no("still_active");
            safe_ = false;
            safe_reason_ = "none";
            return Result{true, "cleared"};
        }
        if (!valid(n)) return no("bad_nozzle");
        Nozzle& z = at(n);
        if (z.phase != Phase::Fault) return Result{true, "not_held"};
        if (z.fault == Fault::SensorFault && !plausible(z)) return no("still_active");
        z.fault = Fault::None;
        z.phase = Phase::Idle;
        z.since_ms = now;
        z.same_since_ms = now;
        return Result{true, "cleared"};
    }

    /// Sets the zero (the sensor open to the air) or the scale (a known vacuum on it) of one sensor from its present reading. Off unless calibration_enabled.
    Result calibrate(int n, bool zero, double known_kpa) {
        if (!valid(n)) return no("bad_nozzle");
        if (!cfg_.calibration_enabled) return no("disabled");
        Nozzle& z = at(n);
        if (z.phase != Phase::Idle || z.valve || pump_) return no("busy");
        if (z.ring.empty()) return no("no_reading");
        double raw = 0;
        for (double v : z.ring) raw += v;
        raw /= static_cast<double>(z.ring.size());
        if (zero) {
            if (!(raw >= -1000 && raw <= 1000)) return no("bad_reading");
            cfg_.pressure_zero = raw;
        } else {
            double span = raw - cfg_.pressure_zero;
            if (!(known_kpa > 1 && known_kpa <= 200) || std::fabs(span) < 1e-6) return no("bad_reading");
            double scale = known_kpa / span;
            if (!(scale >= 0.0001 && scale <= 100)) return no("bad_reading");
            cfg_.pressure_scale = scale;
        }
        for (Nozzle& o : nozzles_) { o.ring.clear(); o.ring_at = 0; }
        calibrated_ = true;
        return Result();
    }

    // ----- time -----

    /// Moves time on by `dt_ms`. `raw` holds the reading of the pressure sensor of each nozzle, in the order of the nozzles.
    void tick(std::uint32_t now, std::uint32_t dt_ms, const double* raw) {
        if (watching_ && !safe_ && now - last_heard_ > static_cast<std::uint32_t>(cfg_.link_lost_ms)) enter_safe("server_lost");
        for (int i = 0; i < cfg_.nozzles; ++i) step_nozzle(nozzles_[static_cast<std::size_t>(i)], now, dt_ms, raw[i]);
        update_pump();
        if (pump_) pump_run_ms_ += dt_ms; else pump_run_ms_ = 0;
        if (pump_ && pump_run_ms_ > static_cast<std::uint32_t>(cfg_.pump_max_run_s) * 1000u) {
            for (Nozzle& z : nozzles_) if (z.valve) raise(z, Fault::PumpOverrun, now);
            update_pump();
            pump_run_ms_ = 0;
        }
    }

private:
    static Result no(const char* why) { return Result{false, why}; }
    bool valid(int n) const { return n >= 1 && n <= cfg_.nozzles; }
    Nozzle& at(int n) { return nozzles_[static_cast<std::size_t>(n - 1)]; }

    bool plausible(const Nozzle& z) const { return z.kpa <= cfg_.pressure_max_kpa && z.kpa >= -10.0; }

    void update_pump() {
        bool any = false;
        for (const Nozzle& z : nozzles_) if (z.valve) any = true;
        pump_ = any;
    }

    void raise(Nozzle& z, Fault f, std::uint32_t now) {
        if (z.phase == Phase::Fault) return;
        z.fault = f;
        z.phase = Phase::Fault;
        z.valve = false;
        z.blow = false;
        z.since_ms = now;
        z.loss_since_ms = 0;
        z.last_result = f == Fault::VacuumLost ? "vacuum_lost" : f == Fault::PumpOverrun ? "pump_overrun" : "sensor_fault";
        if (f == Fault::VacuumLost) ++z.losses;
        z.z_target = z.z;  // nothing keeps moving down while a nozzle holds a fault; going up is still allowed
    }

    void enter_safe(const char* reason) {
        safe_ = true;
        safe_reason_ = reason;
        for (Nozzle& z : nozzles_) {
            if (cfg_.safe_vacuum == SafeVacuum::Off) {
                if (z.phase == Phase::Evacuating || z.phase == Phase::Holding) { z.phase = Phase::Idle; z.last_result = "safe_state"; }
                z.valve = false;
                z.blow = false;
            }
            if (cfg_.safe_z_retract && z.z_target < cfg_.z_safe_mm) z.z_target = cfg_.z_safe_mm;
            z.a_target = z.a;
        }
        light_up_ = 0;
        light_down_ = 0;
        update_pump();
    }

    void step_nozzle(Nozzle& z, std::uint32_t now, std::uint32_t dt_ms, double raw) {
        // the reading: an average of the last pressure_samples
        const std::size_t want = static_cast<std::size_t>(cfg_.pressure_samples);
        if (z.ring.size() != want) { z.ring.assign(want, raw); z.ring_at = 0; }
        z.ring[z.ring_at] = raw;
        z.ring_at = (z.ring_at + 1) % want;
        double avg = 0;
        for (double v : z.ring) avg += v;
        avg /= static_cast<double>(want);
        z.kpa = (avg - cfg_.pressure_zero) * cfg_.pressure_scale;
        // a sensor that cannot be right
        if (z.valve) {
            if (raw != z.last_raw) z.same_since_ms = now;
            else if (now - z.same_since_ms >= static_cast<std::uint32_t>(cfg_.pressure_stuck_ms)) raise(z, Fault::SensorFault, now);
        } else {
            z.same_since_ms = now;
        }
        z.last_raw = raw;
        if (!plausible(z) && z.phase != Phase::Fault && z.ring.size() == want) raise(z, Fault::SensorFault, now);
        // the move
        const double dz = cfg_.z_speed_mm_s * dt_ms / 1000.0;
        if (std::fabs(z.z_target - z.z) <= dz) z.z = z.z_target; else z.z += z.z_target > z.z ? dz : -dz;
        const double da = cfg_.rot_speed_deg_s * dt_ms / 1000.0;
        if (std::fabs(z.a_target - z.a) <= da) z.a = z.a_target; else z.a += z.a_target > z.a ? da : -da;
        // the phase
        switch (z.phase) {
            case Phase::Evacuating:
                if (z.kpa >= cfg_.vacuum_target_kpa) {
                    z.phase = Phase::Holding;
                    z.since_ms = now;
                    z.loss_since_ms = 0;
                    z.last_result = "held";
                    ++z.picks;
                } else if (now - z.since_ms >= static_cast<std::uint32_t>(cfg_.vacuum_settle_ms)) {
                    z.valve = false;
                    z.phase = Phase::Idle;
                    z.since_ms = now;
                    z.last_result = "vacuum_not_reached";
                    ++z.failed_picks;
                }
                break;
            case Phase::Holding:
                if (z.kpa < cfg_.vacuum_hold_min_kpa) {
                    if (z.loss_since_ms == 0) z.loss_since_ms = now == 0 ? 1 : now;
                    else if (now - z.loss_since_ms >= static_cast<std::uint32_t>(cfg_.vacuum_loss_ms)) raise(z, Fault::VacuumLost, now);
                } else {
                    z.loss_since_ms = 0;
                }
                break;
            case Phase::Releasing:
                if (now >= z.blow_until_ms) { z.blow = false; z.phase = Phase::Idle; z.since_ms = now; }
                break;
            default: break;
        }
    }

    Settings cfg_;
    std::vector<Nozzle> nozzles_;
    bool pump_ = false;
    std::uint32_t pump_run_ms_ = 0;
    bool safe_ = false;
    std::string safe_reason_ = "none";
    bool watching_ = false;
    std::uint32_t last_heard_ = 0;
    int light_up_ = 0, light_down_ = 0;
    bool calibrated_ = false;
};

}  // namespace pnp_head
