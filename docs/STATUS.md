# EH3D-NODE-PNP-HEAD: status

## Current state

EH3D-NODE-PNP-HEAD is the logic of a node, tested against a simulator. It has a settings file, the rules of the head, the words of the node on the network and a simulator, the interlocks of the head, two ways to sense the vacuum of each nozzle and the permission for the machine to move in X and Y. The tests run on Windows and Linux in the CI. Nothing has moved a real head: the firmware is not written, and nothing has been connected to a motor, a pump, a valve or a sensor.

## Capabilities

The official state of each capability is in `REQUIREMENTS.md`. The register has 89 requirements: 80 implemented, 0 prototype, 9 planned and 0 validated. By progress: 80 done, 0 partial, 4 pending and 5 blocked. This page never states a state that `REQUIREMENTS.md` does not.

## What is simulated

Everything that touches the world: the tape, the motor, the sensors and the bus (feeder), or the pump, the valves, the parts, the sensors and the axes (head). The numbers of the simulation are made up; no value in the settings has been tuned on hardware.

## Open questions

* Which sensor each nozzle has is a setting (`nozzle_N_sensor` = `analog` or `digital`) and both are written and tried against a simulator; which one a machine uses is decided with the bench tests H-11 to H-14 of `HARDWARE.md`.
* "Nozzles lifted" for the interlock of the moves in X and Y is the safe height (`z_safe_mm`, with a small tolerance that is a setting); the height to turn at (`rotate_min_z_mm`) is not used for it (decided).
* Whether the lights of the head are driven here (as written) or by the camera node of the head.
* Whether this node also takes a paste dispenser: the decision about paste is open.
