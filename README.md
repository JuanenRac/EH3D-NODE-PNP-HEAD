<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-PNP-HEAD banner" width="100%">
</p>

# 🛠️ EH3D-NODE-PNP-HEAD

<p align="center">🇺🇸 <b>English</b> | <a href="README_spa.md">🇪🇸 Español</a> | <a href="README_fra.md">🇫🇷 Français</a> | <a href="README_ita.md">🇮🇹 Italiano</a> | <a href="README_deu.md">🇩🇪 Deutsch</a> | <a href="README_zho.md">🇨🇳 中文</a> | <a href="README_jpn.md">🇯🇵 日本語</a> | <a href="README_por.md">🇵🇹 Português</a></p>

### The node of the head of an EH3D pick-and-place machine: nozzles, vacuum, puff of air, lights and interlocks, written and tried against a simulator

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20Python-blue.svg" alt="C++17 and Python">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Tests-96-2EA043.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.0.2-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-4CAF50.svg" alt="Maturity">
</p>

---

**Honesty check - what actually runs today:** a scaffold with real, tested logic and no hardware. The rules of the node run on a computer against a simulator (96 tests pass), but nothing here has been built into firmware, flashed or connected to a pump, a valve, a pressure sensor or an axis.

---

> **Safety and security status:** this node is not a safety system. Its interlocks and its safe state are a second line behind the stop, the limits and the interlocks of the machine and its controller, and they never replace them.

## 📋 At a glance

| | |
|---|---|
| Version | `0.0.2` |
| Maturity | Functional |
| Tests | 96 |
| Stack | C++17 (host tests), Python 3.10+ |
| On real hardware | Not run on hardware yet |

**Jump to:** [Overview](#-overview) · [Architecture](#-architecture) · [Quick start](#-quick-start) · [Testing](#-testing) · [Repository Structure](#-repository-structure) · [Related Projects](#-related-projects)

## 🚦 Project status

Each line is one part of this repository and the light says how far it is today. It is written from what the repository can show (tests, builds, runs), not from what is planned.

🟢 Done and verified · 🟡 Works, with known limits · 🟠 Partly done or not verified yet · 🔴 Not done or not working

| Area | State | What it means here |
|---|---|---|
| Logic and simulator | 🟡 | The rules of the node run against a simulator with a fixed seed; 96 tests pass on a computer. |
| Network contract | 🟡 | The words of the node are written in docs/CONTRACT.md and a test fails when the page and the answers differ; the server's plugin does not use this node yet. |
| Firmware | 🟠 | The firmware for the board is not written yet: only the plain helpers of the shared base are imported. |
| Real hardware | 🔴 | Nothing has run on a pump, a valve, a pressure sensor or an axis. |

## 🎯 Overview

* **One node, one head:** it picks and releases parts with the vacuum of each nozzle, tells a lost part from a part that was never there, moves the Z axis and the rotation under interlocks, and goes to a safe state by itself when it loses the server.
* **Two ways to sense the vacuum, and a lock for the axes:** each nozzle has a pressure sensor (filtered, calibrated, with a band) or a vacuum switch (debounced), chosen in `head.cfg`, with alarms for a part lost, a vacuum not reached and a sensor out of range or frozen; the head also says when every nozzle is raised to the safe height, so the machine may move in X and Y, and refuses to lower a nozzle while it does.

## 🧩 Role in EH3D

The program for pick and place plans the jobs; this node is what will carry out a pick and a release on one nozzle when a real machine is connected. Today it exists only as logic and a simulator.

## 🏛️ Architecture

<p align="center">
  <img src="images/architecture.svg" alt="EH3D-NODE-PNP-HEAD architecture: who uses it, what it holds and what it depends on" width="100%">
</p>

How the parts of this repository fit together: who uses it, what it holds and what it depends on. The picture is drawn from `docs/architecture.json` and checked against the code.

## 🔒 Security & Safety

The rules of safety and security of this node; the test that shows each one is named in docs/REQUIREMENTS.md:

* A node without a token refuses every request that moves or changes something; the token is never shown, written back or logged.
* When the server is not heard the node goes to a safe state by itself and moves nothing until a person acknowledges. While an order waits (a long move, a long pick) the node goes on listening: the server has to keep asking, and a link lost in the middle stops the move.
* Every limit is a setting with a conservative default, and a wrong value never loosens one: it is refused and the default stays.

## 🔌 Interfaces

The server, the programs and the phones talk to the node over HTTP in the way of the other nodes of the suite (JSON and a bearer token). The page [docs/CONTRACT.md](docs/CONTRACT.md) is the contract and a test keeps it equal to the answers.

```text
GET  /api/status                         the head and all its nozzles (always answered)
GET  /api/nozzle?n=N                     one nozzle
POST /api/nozzle/pick?n=N                open the valve; answer when the vacuum says if a part is held
POST /api/nozzle/place?n=N               release the part with a puff (only low enough)
POST /api/nozzle/blow?n=N                a puff alone (not while a part is held)
POST /api/z?n=N&mm=H                     move the nozzle to the height H
POST /api/rotate?n=N&deg=D               turn the nozzle (only high enough)
POST /api/light?up=P&down=Q              the two lights, in percent
POST /api/stop                           stop every move (the vacuum is not touched)
POST /api/ack[?n=N]                      clear a fault that is gone (or the safe state, without n)
POST /api/vacuum/calibrate?n=N&kind=zero|scale[&kpa=K]   if the settings allow it
POST /api/xy?moving=1|0                    the machine asks to move in X and Y (only with every nozzle raised) or says it stopped
```

## ⚙️ Configuration

Every tuned value is in `head.cfg`; `head.cfg.example` shows all of them with their defaults and ranges. A value out of range, or a key nobody knows, is reported and the default stays.

## 🚀 Quick start

You need a C++17 compiler, CMake 3.16 or newer and Python 3.10 or newer; there is nothing else to install.

```bash
cmake -S tests -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
python -I tools/ci_validate.py
python -I -m unittest discover -s tools -p "test_*.py"
```

## 🧪 Testing

96 tests: C++ host tests with a fixed seed (they run the simulator and the words of the network and compare the contract page with the answers) and Python tests of the repository. The CI also runs the C++ tests with AddressSanitizer and UBSan.

## 🏗️ Build & Release

The version lives in `eh3d.project.json` and each version has its entry in [CHANGELOG.md](CHANGELOG.md). There is no firmware build yet; when the firmware exists it is built only in GitHub Actions. The software is GPL-3.0-or-later; the mechanical parts of the node are meant to be released under CC-BY-SA and its electronics under CERN-OHL-S (see [docs/HARDWARE.md](docs/HARDWARE.md)).

## 📂 Repository Structure

<p align="center">
  <img src="images/repository-structure.svg" alt="Repository structure of EH3D-NODE-PNP-HEAD" width="100%">
</p>

<details>
<summary>Text version of the tree</summary>

```text
EH3D-NODE-PNP-HEAD/
├── core/
│   ├── head_config.hpp cfg_text.hpp                                                                                                         the settings file and its safe defaults
│   ├── head.hpp                                                                                                                             nozzles, pump and axes; analog or digital vacuum sensing; the X/Y lock
│   ├── head_node.hpp version.hpp                                                                                                            the words of the node on the network
│   ├── head_sim.hpp                                                                                                                         a head on a computer: pump, valves, parts, sensors, switches
│   └── json.hpp auth.hpp semver.hpp backoff.hpp task_health.hpp stored_load.hpp memory_budget.hpp net_text.hpp node_id.hpp civil_time.hpp   plain helpers shared by the nodes, kept identical by a tool
├── tests/
│   ├── check.hpp CMakeLists.txt                                                                                                             the harness of the tests and how they are built
│   ├── test_config.cpp                                                                                                                      the settings file, the filter and the calibration
│   ├── test_head.cpp                                                                                                                        the head on a simulated pump and sensors: picks, losses, interlocks, safe state
│   ├── test_sensor.cpp                                                                                                                      analog and digital sensing, alarms, and the X/Y lock
│   └── test_node.cpp                                                                                                                        the words of the network, the token, the safe state and the contract page
├── docs/
│   ├── CONTRACT.md                                                                                                                          what the node says on the network (and on the chain)
│   ├── REQUIREMENTS.md                                                                                                                      every requirement and the test that shows it
│   ├── SCOPE.md STATUS.md NOT_YET.md                                                                                                        what it is for, how far it is, what waits
│   ├── HARDWARE.md                                                                                                                          preliminary parts, bench tests and licences
│   └── architecture.json structure.json                                                                                                     the data of the two pictures
├── tools/                                                                                                                                   the repository check, its tests and the picture tools
├── images/                                                                                                                                  banner and the Architecture and Repository Structure pictures
├── .github/                                                                                                                                 CI and dashboard workflows, issue and pull-request templates
├── eh3d.project.json                                                                                                                        manifest: version, role, maturity and build command
├── head.cfg.example                                                                                                                         all the settings with their defaults and ranges
├── VERSION                                                                                                                                  the version of the repository
├── CHANGELOG.md                                                                                                                             what changed in each version
├── AGENTS.md                                                                                                                                rules for anyone who changes the repository
├── test.sh test.bat                                                                                                                         run the host tests
├── SECURITY.md SUPPORT.md                                                                                                                   how to report a vulnerability and where to ask
└── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE                                                                                               how to contribute, the code of conduct and the licence
```

</details>

## 🗺️ Roadmap & Known limits

**Known limits today**

* No firmware, pump or valve output, pressure reading or axis driver is written; the simulated vacuum and noise are made up. How the controller of the machine learns that the nozzles are lifted is not decided.

**Next**

* The ESP-IDF firmware on the shared base, the outputs of the pump and the valves and the reading of the pressure sensors, then a first bench test of one nozzle.

## 🌍 Languages

README and texts exist in English, Spanish, French, Italian, German, Chinese, Japanese and European Portuguese.

## 🔗 Related Projects

* [EH3D-PHOENIX-PNP](https://github.com/JuanenRac/EH3D-PHOENIX-PNP): the pick-and-place program that plans the jobs and describes this node
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER): the server whose plugin for pick-and-place machines will stand behind this node
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON): the firmware base that this node takes its plain helpers from
* [EH3D-NODE-PNP-FEEDER](https://github.com/JuanenRac/EH3D-NODE-PNP-FEEDER): the node of the feeders that bring the parts

## 📚 Documentation

* [The conventions of the suite](https://github.com/JuanenRac/EH3D-DOCS/blob/main/docs/CONVENTIONS.md)
* [The contract of the node](docs/CONTRACT.md)
* [Requirements and the test of each](docs/REQUIREMENTS.md)
* [Scope](docs/SCOPE.md), [status](docs/STATUS.md) and [what is not done yet](docs/NOT_YET.md)
* [Hardware: preliminary parts, tests on a bench and licences](docs/HARDWARE.md)
* [Changelog](CHANGELOG.md)

## 🤝 Contributing & Community

* Before a pull request run `python -I tools/ci_validate.py` and the tests of the base: all must pass. See [CONTRIBUTING.md](CONTRIBUTING.md) and the [code of conduct](CODE_OF_CONDUCT.md).
* The README exists in eight languages: a change to the text is made in all eight files, and the two pictures are drawn from `docs/architecture.json` and `docs/structure.json` by `tools/make_architecture.py` and `tools/make_structure.py`.
* Report a vulnerability privately: [SECURITY.md](SECURITY.md). Questions, ideas and reports: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTHOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENSE

Software: GPL-3.0-or-later. See [LICENSE](LICENSE). Hardware: the mechanical parts are meant for CC-BY-SA and the electronics for CERN-OHL-S (see [docs/HARDWARE.md](docs/HARDWARE.md)).
