# Changelog

All notable changes to this project are documented here.

## [Unreleased]

- **The shared lessons check reads only what is part of the repository:** what git ignores, and folders such as `managed_components` that a firmware build downloads, are no longer analysed, so the local validation no longer fails over downloaded code.

## [0.0.3] - Keeps listening while an order waits and says its link time (simulator only)

- **The head says how long it goes without being asked:** `GET /api/status` has `link_lost_ms`, the value of the settings the head runs with, with or without the token, so a server can tell when it asks too slowly. Two new tests, on the simulator only; nothing has run on a board.
- **The READMEs say what the node is today:** it works in simulation with tested logic, with no firmware and no hardware; they no longer call it a scaffold.
- **A long order no longer ends in the safe state while the server is there:** a move, a turn or a `vacuum_settle_ms` longer than `link_lost_ms` made the head safe although the link was good, because nothing was heard while the node waited. It now goes on listening, and a request with the token counts; the server has to keep asking while it waits. While an order waits, a stop and letting go of the X and Y axes are carried out and other orders are `busy`. The watch itself is unchanged: a link lost in the middle, or a task that did not run for that long, makes the head safe. Not tried on a board: there is no firmware yet.
- **The safe state stops any move (fix):** with `safe_z_retract` off, a nozzle that was moving went on to its target in the safe state. It now stops where it is, and a puff cut short by the safe state is no longer answered as done. Nine new tests.
- **A move that did not arrive is no longer answered as done (fix):** `POST /api/z` and `POST /api/rotate` answered `ok` even when the head went to its safe state or the nozzle got a fault in the middle of the move, so the machine could go on believing the nozzle was where it asked. They now answer `409` with `safe_state`, `fault` or `not_finished`. Two new tests.
- The shared workflow checker (`tools/workflow_hygiene.py`) is the current one of the documentation hub: the filters under a trigger (branches, paths, types) are no longer read as events.

## [0.0.2] - Three buses for the feeders, a pressure sensor or a vacuum switch for every nozzle and the lock of the X and Y axes

- **Two ways to sense the vacuum of a nozzle**, chosen for each nozzle with `nozzle_N_sensor` in `head.cfg`: a pressure sensor read as a number (filtered, calibrated, with a band between the target and the hold limit) or a vacuum switch read as a level (debounced, with its polarity as a setting). The pick, the hold and the loss work the same for both.
- **Alarms of a nozzle:** a part lost, a vacuum not reached, a sensor out of range (a number that cannot be real, or a level that is neither high nor low) and a sensor frozen (a number that does not change, or a switch that says vacuum with nothing pulling), each with a code, shown in the answers until a person acknowledges.
- **Calibration only for the pressure sensors:** a nozzle with a switch refuses it with the reason.
- **Interlock for the X and Y axes of the machine:** the head says in its status when every nozzle is raised to the safe height, which nozzles are not, and grants the permission of `POST /api/xy` only when all are; while the machine holds it, no nozzle is lowered under that height. The height for turning a nozzle plays no part in it, and the tolerance is a setting.
- **Self-check:** an info code when a pick did not reach the vacuum.
- **Documents:** new sections of the contract page on the sensors, the alarms and the interlock; five new bench tests in the hardware page.
- **Tests:** 28 new tests for the sensors and the interlock, 85 in all.

## [0.0.1] - The logic of the node and its simulator

- **The settings of the node:** `head.cfg` with safe defaults (no token, calibration off, the pump and the valves off in the safe state), ranges for every value, and a refusal with the key for a value out of range or a key nobody knows.
- **The pressure sensors:** a filter, a zero and a scale, a reading that cannot be real or does not change as a fault, and a calibration that is off until the settings allow it.
- **Picking and placing:** a pick that tells a part held from nothing, a lost part as a fault, a release with a short puff only when low, a pump shared between nozzles and stopped when the last valve closes or after too long.
- **The axes and the interlocks:** the travel of Z and the turn of each nozzle, no turn when low, no lowering while turning, nothing lowered while a nozzle holds a fault or the head is safe.
- **The safe state:** when the server is not heard the pump stops, the valves close, the nozzles go up and the lights go out, until a person acknowledges; a setting can keep the vacuum instead.
- **The words of the node over HTTP** behind a token, and a self-check with codes.
- **A simulator of the head** with a pump, valves, parts that seal a nozzle or not and sensors with noise, with a fixed seed.
- **57 tests** and a register of 50 requirements that names the test of each; the page `docs/CONTRACT.md` is read by a test and cannot go stale.
- **Ten plain helpers of the firmware base** of EH3D-COMMON, kept identical by its tool; the firmware itself is not written.
