# Changelog

All notable changes to this project are documented here.

## [Unreleased]

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
