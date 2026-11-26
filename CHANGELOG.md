# Changelog

All notable changes to this project are documented here.

## [Unreleased]

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
