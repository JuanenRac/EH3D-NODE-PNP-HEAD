# EH3D-NODE-PNP-HEAD: hardware

Nothing in this page has been built or tried: it is the preliminary shape of the hardware of the node, written so that the first bench has a list to start from. No manufacturer or part number is named, because none has been checked; every row below is `verified: no`.

## Preliminary parts

| Part | What it is for | What it could be | Verified |
|---|---|---|---|
| Controller board | The node: network, the pump, the valves, the lights and the logic | an ESP32-S3 module, as in the other nodes of the suite | no |
| Vacuum pump | Makes the vacuum for the nozzles | a small diaphragm pump; the vacuum it reaches with the hoses and nozzles of the head is not measured | no |
| Valves | One per nozzle: opens the vacuum | solenoid valves with a driver each | no |
| Puff of air | Lets a part go | a second port of the valve or a separate small valve | no |
| Vacuum sensors | One per nozzle line, chosen for each nozzle in the settings (`nozzle_N_sensor`) | a gauge pressure sensor read as a number (analog), or a vacuum switch read as a level (digital); the range needed and the pressure at which a switch makes and lets go are not measured | no |
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
| H-11 | A vacuum switch on a nozzle (`nozzle_N_sensor = digital`): raise the vacuum slowly and let it fall slowly against a reference gauge | The pressure at which the switch makes and the one at which it lets go (its hysteresis); a pick over a real part holds, and the level the switch gives for "vacuum" matches `digital_active_high` |
| H-12 | The same switch on an oscilloscope while the vacuum changes | How long the level bounces; set `digital_debounce_ms` above it and below `vacuum_settle_ms`; no pick is lost to a flicker |
| H-13 | One head with a nozzle on a switch and a nozzle on a pressure sensor, the same part on both | Both hold; the number of the pressure sensor agrees with the point where the switch changes; HEAD-V02 on both when the part is taken away |
| H-14 | Unplug a switch, then put a short on its line, then leave it floating | A level that is neither high nor low is HEAD-S01 for that nozzle only; a switch stuck on is HEAD-S02 after `pressure_stuck_ms`; write down what the electronics gives in each case |
| H-15 | The interlock of X and Y: with one nozzle just under `z_safe_mm`, ask `POST /api/xy?moving=1`; then raise it and ask again; then try to lower a nozzle while the machine moves | The permission is refused with the nozzle down and given with it up; the lowering is refused while the machine holds it; measure the time between the request and the answer, and the height at which the nozzle is really clear of the tallest part on the board |

## Licences

* **Software:** GPL-3.0-or-later (the file `LICENSE`).
* **Mechanical parts** (models, drawings): CC-BY-SA. The version is confirmed when the first model is added.
* **Electronics** (schematics, boards): CERN-OHL-S. The version is confirmed when the first design file is added.

There are no hardware files in this repository yet; the text of each licence goes into the folder of the files it covers when the first one is added.
