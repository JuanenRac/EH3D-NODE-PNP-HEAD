# EH3D-NODE-PNP-HEAD: status

## Current state

EH3D-NODE-PNP-HEAD is the logic of a node, tested against a simulator. It has a settings file, the rules of the head, the words of the node on the network and a simulator, and the interlocks of the head. The tests run on Windows and Linux in the CI. Nothing has moved a real head: the firmware is not written, and nothing has been connected to a motor, a pump, a valve or a sensor.

## Capabilities

The official state of each capability is in `REQUIREMENTS.md`. The register has 50 requirements: 41 implemented, 0 prototype, 9 planned and 0 validated. By progress: 41 done, 0 partial, 5 pending and 4 blocked. This page never states a state that `REQUIREMENTS.md` does not.

## What is simulated

Everything that touches the world: the tape, the motor, the sensors and the bus (feeder), or the pump, the valves, the parts, the sensors and the axes (head). The numbers of the simulation are made up; no value in the settings has been tuned on hardware.

## Open questions

* Which pressure sensors: one analogue sensor per nozzle line is assumed; a digital one would change only the reading, not the logic. Options: one analogue sensor per nozzle (recommended), one digital sensor per nozzle, one shared sensor.
* Which height counts as "nozzles lifted" for the interlocks of the moves in X and Y of the machine: the safe height or the height to turn at.
* Whether the lights of the head are driven here (as written) or by the camera node of the head.
* Whether this node also takes a paste dispenser: the decision about paste is open.
