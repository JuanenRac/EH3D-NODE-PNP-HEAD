# AGENTS.md: rules for anyone who changes EH3D-NODE-PNP-HEAD

EH3D-NODE-PNP-HEAD: the logic of a pick-and-place node, tried against a simulator. Software licence: GPL-3.0-or-later; every source file starts with its header.

## 1. Before you start

1. Read `docs/STATUS.md`: what exists, what is missing, what the last change decided.
2. Read `docs/SCOPE.md` (what the node is for and what it never decides) and `docs/REQUIREMENTS.md` (the register of requirements and the test that shows each one).
3. Read `docs/NOT_YET.md` before adding an idea: it may be there on purpose.
4. Read `docs/CONTRACT.md`: it is the contract with the server and the programs, and a test keeps it equal to the answers.

## 2. Build and test

- The host tests need a C++17 compiler and CMake: `cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host --output-on-failure`. The CI runs them also with AddressSanitizer and UBSan.
- Run before every commit: `python -I tools/ci_validate.py`, `python -I tools/make_structure.py --check --readmes`, `python -I tools/make_architecture.py --check` and `python -I -m unittest discover -s tools -p "test_*.py"`.
- Firmware is built only in GitHub Actions, never on a machine that is short of memory; this repository has no firmware build yet.
- A change that adds a file adds it to `docs/structure.json` and regenerates the tree and the picture.
- The plain helpers in `core/` that come from the firmware base of EH3D-COMMON (`json.hpp`, `auth.hpp`, `semver.hpp`, `backoff.hpp`, `task_health.hpp`, `stored_load.hpp`, `memory_budget.hpp`, `net_text.hpp`, `node_id.hpp`, `civil_time.hpp`) are never edited here: they are changed in EH3D-COMMON and brought with `sync_firmware_base.py`.

## 3. Code rules

- The logic in `core/` touches no hardware and no clock: the time and the readings are handed in, so that it is tested on a computer.
- A reader never panics and never guesses. What it cannot understand is refused, with the key that is wrong.
- Every value that is tuned (a limit, a tolerance, a time) is a setting in `head.cfg`, not a constant in the code; every option of security has a switch with the conservative value as its default. A new setting is added to `head.cfg.example` in the same change.
- A rule of safety only ever makes a move refused or slower. A change that would relax one after a trouble is a defect: the tests that fail for it must stay.
- The random numbers of a simulation come from a seed, so that a run is the same every time.

## 4. What the node must not do

- It does not move anything unless a request with the token asks, and it does nothing by itself except stop.
- It does not handle objects, arms or robots: it serves a machine that assembles boards.
- Its documents and its code do not name anything private: no dates, no internal notes, no names of people or of tools that wrote the change.

## 5. Evidence

- A state in `docs/REQUIREMENTS.md` is `validated` only when a person has run the capability on the real hardware it names. Nothing is green that has never run on the real thing.
- A requirement names a test that exists; when a requirement changes state, the row and `docs/STATUS.md` change in the same commit, and `tools/test_node.py` says whether they agree.

## 6. Documents and languages

- The README exists in eight languages (English, Spanish, German, French, Italian, Japanese, Chinese and Portuguese of Portugal) and keeps the same structure in all of them.
- The licences: software GPL-3.0-or-later, mechanical parts CC-BY-SA, electronics CERN-OHL-S (see `docs/HARDWARE.md`).
