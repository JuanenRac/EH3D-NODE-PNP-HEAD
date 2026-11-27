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
//  * The vacuum of a nozzle is sensed in one of two ways, chosen for each nozzle in the settings: a pressure sensor read as a number (averaged, calibrated, with a band
//    between the target and the hold limit) or a vacuum switch read as a level (on or off, debounced). The rules of the pick, the hold and the loss are the same for both.
//  * A sensor that reads what cannot be real (a number out of range, a level between high and low), or that does not change at all while a valve is open (a number),
//    or that says vacuum with nothing pulling (a level), is a fault: the vacuum of that nozzle cannot be trusted.
//  * The nozzles are "raised" when they are at or above z_safe_mm (with a small tolerance). The machine asks the head for permission before it moves its X and Y axes
//    and gets it only when every nozzle is raised; while it has it, no nozzle is lowered under that height. rotate_min_z_mm is for turning and is not used for this.
//  * When the server is not heard for link_lost_ms the head is in its safe state: the pump stops, the valves close (unless safe_vacuum is hold), any move stops, the nozzles
//    go up and the lights go out, also in the middle of a command. Nothing moves down or takes a part until a person acknowledges.
#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "head_config.hpp"

namespace pnp_head {

enum class Phase { Idle, Evacuating, Holding, Releasing, Fault };
enum class Fault { None, VacuumLost, PumpOverrun, SensorFault, SensorFrozen };
/// What a nozzle reports as an alarm: the last thing that went wrong with its vacuum or its sensor, until a person acknowledges it or the next pick starts.
enum class Alarm { None, VacuumNotReached, PartLost, SensorRange, SensorFrozen };

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
        case Fault::SensorFrozen: return "HEAD-S02";
    }
    return "none";
}

inline const char* alarm_text(Alarm a) {
    switch (a) {
        case Alarm::None: return "none";
        case Alarm::VacuumNotReached: return "vacuum_not_reached";
        case Alarm::PartLost: return "part_lost";
        case Alarm::SensorRange: return "sensor_out_of_range";
        case Alarm::SensorFrozen: return "sensor_frozen";
    }
    return "none";
}

inline const char* alarm_code(Alarm a) {
    switch (a) {
        case Alarm::None: return "none";
        case Alarm::VacuumNotReached: return "HEAD-V01";
        case Alarm::PartLost: return "HEAD-V02";
        case Alarm::SensorRange: return "HEAD-S01";
        case Alarm::SensorFrozen: return "HEAD-S02";
    }
    return "none";
}

/// Every code an alarm can carry, for the page of the contract and its test.
inline std::vector<const char*> all_alarm_codes() { return {"HEAD-V01", "HEAD-V02", "HEAD-S01", "HEAD-S02"}; }

/// Every code a nozzle can hold, and the one of the safe state, for the page of the contract and its test.
inline std::vector<const char*> all_fault_codes() { return {"HEAD-V02", "HEAD-P01", "HEAD-S01", "HEAD-S02", "HEAD-L01"}; }

struct Result {
    bool ok = true;
    std::string why = "ok";
};

struct Nozzle {
    Phase phase = Phase::Idle;
    Fault fault = Fault::None;
    Alarm alarm = Alarm::None;
    bool vac_ok = false;                  // the vacuum is there: a number at or above the target (until it falls under the hold limit), or a debounced level
    bool level_ok = false;                // a digital sensor: the debounced level
    bool level_candidate = false;         // a digital sensor: the level seen now, before it has stayed long enough
    bool level_invalid = false;           // a digital sensor: the reading is neither high nor low
    std::uint32_t level_since_ms = 0;
    std::uint32_t invalid_since_ms = 0;
    std::uint32_t idle_vac_since_ms = 0;  // a digital sensor: since when it says vacuum with the valve closed (0: it does not)
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

    /// The server was heard (a request with the token): from now on its silence is watched. The silence before it is judged first: an order that arrives after a gap that was
    /// already too long (the task that watches did not run) does not hide the loss.
    void heard(std::uint32_t now_ms) { watch(now_ms); watching_ = true; last_heard_ = now_ms; }

    /// The safe state when the server has been silent for longer than link_lost_ms.
    void watch(std::uint32_t now_ms) {
        if (watching_ && !safe_ && now_ms - last_heard_ > static_cast<std::uint32_t>(cfg_.link_lost_ms)) enter_safe("server_lost");
    }

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
        if (z.alarm == Alarm::VacuumNotReached) z.alarm = Alarm::None;
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
        // while the machine moves in X and Y with the nozzles up, none of them goes under the height where they count as raised
        if (xy_moving_ && mm < cfg_.z_safe_mm - cfg_.interlock_z_tolerance_mm - 1e-9) return no("interlock_xy_moving");
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

    /// A nozzle is raised when it is, and is going to stay, at or above the safe height (less the tolerance).
    bool raised(int n) const {
        const Nozzle& z = nozzle(n);
        const double floor_mm = cfg_.z_safe_mm - cfg_.interlock_z_tolerance_mm - 1e-9;
        return z.z >= floor_mm && z.z_target >= floor_mm;
    }
    /// Every nozzle is raised: the machine may move in X and Y.
    bool xy_clear() const {
        for (int n = 1; n <= cfg_.nozzles; ++n) if (!raised(n)) return false;
        return true;
    }
    /// The numbers of the nozzles that are not raised, separated by commas.
    std::string xy_blocking() const {
        std::string out;
        for (int n = 1; n <= cfg_.nozzles; ++n) {
            if (raised(n)) continue;
            if (!out.empty()) out += ",";
            out += std::to_string(n);
        }
        return out;
    }
    bool xy_moving() const { return xy_moving_; }

    /// The machine asks to move in X and Y (`moving` true) or says it has stopped (false). The permission is given only while every nozzle is raised, and while it lasts no
    /// nozzle is lowered under the safe height. Letting it go is always accepted.
    Result xy_move(bool moving) {
        if (!moving) { xy_moving_ = false; return Result(); }
        if (safe_) return no("safe_state");  // no new movement is allowed until a person has looked at the head
        if (!xy_clear()) return no("interlock_xy_nozzle_low");
        xy_moving_ = true;
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
            xy_moving_ = false;  // a person has looked at the head: the permission to move in X and Y is asked for again
            return Result{true, "cleared"};
        }
        if (!valid(n)) return no("bad_nozzle");
        Nozzle& z = at(n);
        if (z.phase != Phase::Fault) {
            if (z.alarm == Alarm::None) return Result{true, "not_held"};
            z.alarm = Alarm::None;  // the alarm of a pick that did not reach the vacuum
            return Result{true, "cleared"};
        }
        if (z.fault == Fault::SensorFault && sensor_out_of_range(n, z)) return no("still_active");
        if (z.fault == Fault::SensorFrozen && cfg_.sensor[n - 1] == SensorKind::Digital && z.level_ok) return no("still_active");
        z.fault = Fault::None;
        z.alarm = Alarm::None;
        z.phase = Phase::Idle;
        z.since_ms = now;
        z.same_since_ms = now;
        return Result{true, "cleared"};
    }

    /// Sets the zero (the sensor open to the air) or the scale (a known vacuum on it) of one sensor from its present reading. Off unless calibration_enabled.
    Result calibrate(int n, bool zero, double known_kpa) {
        if (!valid(n)) return no("bad_nozzle");
        if (!cfg_.calibration_enabled) return no("disabled");
        if (cfg_.sensor[n - 1] != SensorKind::Analog) return no("not_analog");  // a switch has no scale to set
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
        watch(now);
        for (int i = 0; i < cfg_.nozzles; ++i) step_nozzle(nozzles_[static_cast<std::size_t>(i)], i, now, dt_ms, raw[i]);
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

    /// The reading cannot be real: a number outside what the sensor can give, or a level that is neither high nor low.
    bool sensor_out_of_range(int n, const Nozzle& z) const {
        return cfg_.sensor[n - 1] == SensorKind::Digital ? z.level_invalid : !plausible(z);
    }

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
        z.last_result = f == Fault::VacuumLost ? "vacuum_lost" : f == Fault::PumpOverrun ? "pump_overrun" : f == Fault::SensorFrozen ? "sensor_frozen" : "sensor_fault";
        if (f == Fault::VacuumLost) { ++z.losses; z.alarm = Alarm::PartLost; }
        else if (f == Fault::SensorFault) z.alarm = Alarm::SensorRange;
        else if (f == Fault::SensorFrozen) z.alarm = Alarm::SensorFrozen;
        z.z_target = z.z;  // nothing keeps moving down while a nozzle holds a fault; going up is still allowed
    }

    /// A pressure sensor: an average of the last pressure_samples, turned into kPa, with a band between the target and the hold limit; a sensor that cannot be right is a fault.
    void read_number(Nozzle& z, std::uint32_t now, double raw) {
        const std::size_t want = static_cast<std::size_t>(cfg_.pressure_samples);
        if (z.ring.size() != want) { z.ring.assign(want, raw); z.ring_at = 0; }
        z.ring[z.ring_at] = raw;
        z.ring_at = (z.ring_at + 1) % want;
        double avg = 0;
        for (double v : z.ring) avg += v;
        avg /= static_cast<double>(want);
        z.kpa = (avg - cfg_.pressure_zero) * cfg_.pressure_scale;
        if (z.kpa >= cfg_.vacuum_target_kpa) z.vac_ok = true; else if (z.kpa < cfg_.vacuum_hold_min_kpa) z.vac_ok = false;
        // a sensor that does not change at all while a valve is open is frozen
        if (z.valve) {
            if (raw != z.last_raw) z.same_since_ms = now;
            else if (now - z.same_since_ms >= static_cast<std::uint32_t>(cfg_.pressure_stuck_ms)) raise(z, Fault::SensorFrozen, now);
        } else {
            z.same_since_ms = now;
        }
        z.last_raw = raw;
        if (!plausible(z) && z.phase != Phase::Fault && z.ring.size() == want) raise(z, Fault::SensorFault, now);
    }

    /// A vacuum switch: a level that counts after it has stayed the same for digital_debounce_ms. A level that is neither high nor low for that long is a sensor out of
    /// range; a switch that says vacuum for pressure_stuck_ms while the valve is closed and nothing is pulling is stuck.
    void read_level(Nozzle& z, std::uint32_t now, double raw) {
        const bool high = raw >= 0.8, low = raw <= 0.2;
        const std::uint32_t debounce = static_cast<std::uint32_t>(cfg_.digital_debounce_ms);
        z.kpa = 0.0;  // a switch does not measure
        if (!high && !low) {
            if (!z.level_invalid) { z.level_invalid = true; z.invalid_since_ms = now; }
            else if (now - z.invalid_since_ms >= debounce && z.phase != Phase::Fault) raise(z, Fault::SensorFault, now);
        } else {
            z.level_invalid = false;
            const bool vacuum = cfg_.digital_active_high ? high : low;
            if (vacuum != z.level_candidate) { z.level_candidate = vacuum; z.level_since_ms = now; }
            if (z.level_candidate != z.level_ok && now - z.level_since_ms >= debounce) z.level_ok = z.level_candidate;
        }
        z.vac_ok = z.level_ok;
        const bool pulling = z.valve || z.blow || z.phase == Phase::Holding || z.phase == Phase::Evacuating || z.phase == Phase::Releasing;
        if (z.level_ok && !pulling && z.phase != Phase::Fault) {
            if (z.idle_vac_since_ms == 0) z.idle_vac_since_ms = now == 0 ? 1 : now;
            else if (now - z.idle_vac_since_ms >= static_cast<std::uint32_t>(cfg_.pressure_stuck_ms)) raise(z, Fault::SensorFrozen, now);
        } else {
            z.idle_vac_since_ms = 0;
        }
    }

    /// The vacuum has reached the target (a number), or the switch says it is there.
    bool reached(int index, const Nozzle& z) const { return cfg_.sensor[index] == SensorKind::Digital ? z.level_ok : z.kpa >= cfg_.vacuum_target_kpa; }
    /// The vacuum of a held part is gone: a number under the hold limit, or the switch says no.
    bool gone(int index, const Nozzle& z) const { return cfg_.sensor[index] == SensorKind::Digital ? !z.level_ok : z.kpa < cfg_.vacuum_hold_min_kpa; }

    void enter_safe(const char* reason) {
        safe_ = true;
        safe_reason_ = reason;
        for (Nozzle& z : nozzles_) {
            if (cfg_.safe_vacuum == SafeVacuum::Off) {
                if (z.phase == Phase::Evacuating || z.phase == Phase::Holding) { z.phase = Phase::Idle; z.last_result = "safe_state"; }
                z.valve = false;
                z.blow = false;
            }
            // whatever was moving stops where it is; only the way up to the safe height goes on
            z.z_target = cfg_.safe_z_retract && z.z < cfg_.z_safe_mm ? cfg_.z_safe_mm : z.z;
            z.a_target = z.a;
        }
        light_up_ = 0;
        light_down_ = 0;
        update_pump();
    }

    void step_nozzle(Nozzle& z, int index, std::uint32_t now, std::uint32_t dt_ms, double raw) {
        if (cfg_.sensor[index] == SensorKind::Digital) read_level(z, now, raw); else read_number(z, now, raw);
        // the move
        const double dz = cfg_.z_speed_mm_s * dt_ms / 1000.0;
        if (std::fabs(z.z_target - z.z) <= dz) z.z = z.z_target; else z.z += z.z_target > z.z ? dz : -dz;
        const double da = cfg_.rot_speed_deg_s * dt_ms / 1000.0;
        if (std::fabs(z.a_target - z.a) <= da) z.a = z.a_target; else z.a += z.a_target > z.a ? da : -da;
        // the phase
        switch (z.phase) {
            case Phase::Evacuating:
                if (reached(index, z)) {
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
                    z.alarm = Alarm::VacuumNotReached;
                    ++z.failed_picks;
                }
                break;
            case Phase::Holding:
                if (gone(index, z)) {
                    if (z.loss_since_ms == 0) z.loss_since_ms = now == 0 ? 1 : now;
                    else if (now - z.loss_since_ms >= static_cast<std::uint32_t>(cfg_.vacuum_loss_ms)) raise(z, Fault::VacuumLost, now);
                } else {
                    z.loss_since_ms = 0;
                }
                break;
            case Phase::Releasing:
                if (static_cast<std::int32_t>(now - z.blow_until_ms) >= 0) {  // also right when the clock wraps around (49.7 days)
                    z.blow = false; z.phase = Phase::Idle; z.since_ms = now; }
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
    bool xy_moving_ = false;
};

}  // namespace pnp_head
