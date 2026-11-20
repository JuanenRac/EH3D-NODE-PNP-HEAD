# EH3D-NODE-PNP-HEAD: hardware

Nothing in this page has been built or tried: it is the preliminary shape of the hardware of the node, written so that the first bench has a list to start from. No manufacturer or part number is named, because none has been checked; every row below is `verified: no`.

## Preliminary parts

| Part | What it is for | What it could be | Verified |
|---|---|---|---|
| Controller board | The node: network, the pump, the valves, the lights and the logic | an ESP32-S3 module, as in the other nodes of the suite | no |
| Vacuum pump | Makes the vacuum for the nozzles | a small diaphragm pump; the vacuum it reaches with the hoses and nozzles of the head is not measured | no |
| Valves | One per nozzle: opens the vacuum | solenoid valves with a driver each | no |
| Puff of air | Lets a part go | a second port of the valve or a separate small valve | no |
| Pressure sensors | One per nozzle line | gauge pressure sensors, analogue or digital; the range needed is not measured | no |
| Z actuators | One per nozzle | stepper with a screw, or another; the travel is a setting | no |
| Rotation motors | One per nozzle | steppers with a driver that has a serial interface; the turn is a setting | no |
| Lights | The camera looking up and the one looking down | LEDs with PWM drivers | no |
| Nozzles | Pick the part | a set of sizes; the mechanical parts are CC-BY-SA | no |
| Power supply | The pump, the motors and the electronics | to be sized when the motors are chosen | no |

## Tests on a bench

The node is tried on a computer against a simulator. Each test below is what a person must run on the real hardware before any requirement is called validated; until then the evidence stays `simulator`.

| Test | What to do | What must be seen |
|---|---|---|
| H-01 | Calibrate the zero and the scale of a sensor against a reference gauge | The kPa read agrees with the gauge within the tolerance written down |
| H-02 | A pick over real parts from 0402 up to a large IC | The vacuum reaches the target within the settle time; write the time for each part |
| H-03 | A pick over nothing | `vacuum_not_reached`, the valve closed and the pump stopped |
| H-04 | Take a held part away by hand | HEAD-V02 within the loss time, and the valve closed |
| H-05 | Hold a part for longer than the pump limit | HEAD-P01, the pump stopped and the valves closed |
| H-06 | Z and rotation to their limits and past them | The orders past a limit are refused; the interlocks refuse a turn when low and a release when high |
| H-07 | Release a real part at the board | The part lets go with the puff and is not blown away; tune the puff |
| H-08 | Stop the polls of the server with a part held | The pump stops, the valves close, the nozzles go up and the lights go out; the part falls unless the safe vacuum is on hold |
| H-09 | Unplug a pressure sensor | HEAD-S01 for that nozzle only |
| H-10 | Cut the power during a pick and switch it on again | The head wakes up with the pump off and the valves closed |

## Licences

* **Software:** GPL-3.0-or-later (the file `LICENSE`).
* **Mechanical parts** (models, drawings): CC-BY-SA. The version is confirmed when the first model is added.
* **Electronics** (schematics, boards): CERN-OHL-S. The version is confirmed when the first design file is added.

There are no hardware files in this repository yet; the text of each licence goes into the folder of the files it covers when the first one is added.
