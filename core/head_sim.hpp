// EH3D-NODE-PNP-HEAD - core/head_sim.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// A head on a computer: a pump and valves that make a vacuum in the nozzles, parts that seal the nozzle or not, pressure sensors with a little noise, and faults that can be
// switched on (a part that falls, a pump that does not pull, a nozzle that leaks, a sensor that freezes or reads what cannot be real). The random numbers come from a seed,
// so a run is the same every time. The numbers of the vacuum, of the noise and of the delays are made up for the simulation: nothing here has been compared with a real head.
#pragma once

#include <cstdint>

#include "head.hpp"

namespace pnp_head {

/// A small generator with a seed (xorshift32): the same seed gives the same run.
class Rng {
public:
    explicit Rng(std::uint32_t seed) : s_(seed == 0 ? 0x9E3779B9u : seed) {}
    std::uint32_t next() { s_ ^= s_ << 13; s_ ^= s_ >> 17; s_ ^= s_ << 5; return s_; }
    /// A number from -1 to 1.
    double unit() { return static_cast<double>(next() % 20001u) / 10000.0 - 1.0; }
private:
    std::uint32_t s_;
};

/// What the node needs from the world: the time, a way to let it pass, and the raw readings of the sensors.
class Io {
public:
    virtual ~Io() = default;
    virtual std::uint32_t now_ms() const = 0;
    virtual void wait(std::uint32_t ms) = 0;
    virtual void read_raw(double* raw, int n) = 0;
};

class SimPlant : public Io {
public:
    SimPlant(const Settings& settings, std::uint32_t seed) : rng_(seed), n_(settings.nozzles) {}

    /// The plant sees the head's outputs (the pump, the valves, the puffs) through this pointer.
    void attach(const Head* head) { head_ = head; }

    // the world around the nozzles
    bool part[kMaxNozzles] = {false, false, false, false, false, false};     // a part sits under the nozzle and seals it
    double seal_kpa = 70.0;       // the vacuum with a part on the nozzle
    double leak_kpa = 12.0;       // the vacuum with the valve open and no part
    double noise_kpa = 0.3;       // the size of the noise of the sensors
    double tau_ms = 60.0;         // how fast the vacuum rises and falls
    bool pump_pulls = true;       // false: the pump does not pull (it is dead or its hose is off)
    bool frozen[kMaxNozzles] = {false, false, false, false, false, false};   // the sensor repeats its last reading
    bool crazy[kMaxNozzles] = {false, false, false, false, false, false};    // the sensor reads what cannot be real
    double sensor_zero = 0.0;     // the real zero and scale of the sensors (what a calibration finds)
    double sensor_scale = 0.1;

    std::uint32_t now_ms() const override { return now_; }

    void wait(std::uint32_t ms) override {
        while (ms > 0) {
            std::uint32_t dt = ms > 10 ? 10 : ms;
            now_ += dt;
            for (int i = 0; i < n_; ++i) {
                const Nozzle& z = head_->nozzle(i + 1);
                double target = 0.0;
                if (z.blow) { target = 0.0; part[i] = false; vac_[i] = 0.0; }
                else if (head_->pump() && z.valve && pump_pulls) target = part[i] ? seal_kpa : leak_kpa;
                vac_[i] += (target - vac_[i]) * (1.0 - std::exp(-static_cast<double>(dt) / tau_ms));
            }
            ms -= dt;
        }
    }

    void read_raw(double* raw, int n) override {
        for (int i = 0; i < n; ++i) {
            if (frozen[i]) { raw[i] = last_[i]; continue; }
            double kpa = vac_[i] + noise_kpa * rng_.unit();
            raw[i] = kpa / sensor_scale + sensor_zero;
            if (crazy[i]) raw[i] = 5000.0;
            last_[i] = raw[i];
        }
    }

    double vacuum(int i) const { return vac_[i]; }

private:
    Rng rng_;
    int n_;
    const Head* head_ = nullptr;
    std::uint32_t now_ = 1000;
    double vac_[kMaxNozzles] = {0, 0, 0, 0, 0, 0};
    double last_[kMaxNozzles] = {0, 0, 0, 0, 0, 0};
};

}  // namespace pnp_head
