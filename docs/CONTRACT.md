# What the node says on the network

The server, the desktop programs and the phones talk to the head through HTTP. The words are built in `core/head_node.hpp`, what the head does with its pump, valves and axes in `core/head.hpp`, and `core/head_sim.hpp` is a head on a computer that gives the very same answers. A host test (`tests/test_node.cpp`) reads this page and fails when a key or a code is in the page and not in the answer, or the other way round, so the page cannot go stale.

Nothing in this page has been checked against a real head, pump, valve or pressure sensor: it is checked against the simulator. The numbers of the simulated vacuum, noise and delays are made up.

The node follows the way of the other nodes of the suite: one line of JSON per answer, a token as `Authorization: Bearer <token>` or `?token=`, a program ignores a key it does not know. It adds nothing to the server's own protocol: the server's plugin for pick-and-place machines asks for a place, and this node is what carries out the pick and the release of one nozzle when a real machine is connected. The camera and the ring of LEDs that look at the part are the work of the camera node of the head (the profile of the other node of the suite); this node has only the two lights of the head.

## `GET /api/status`

Always answered, with or without the token. One line of JSON.

| Key | Type | Meaning |
|---|---|---|
| `node` | text | the name of the node |
| `version` | text | the version of the firmware |
| `nozzle_count` | number | the nozzles of the head, from the settings |
| `safe` | bool | the head is in its safe state: it lost the server and moves nothing down or takes nothing until a person acknowledges |
| `safe_reason` | text | `server_lost`, or `none` |
| `safe_code` | text | `HEAD-L01` while the head is safe, else `none` |
| `link_lost_ms` | number | milliseconds without a request with the token after which the head is safe: the setting `link_lost_ms` it runs with (500 to 600000), said with or without the token |
| `pump` | bool | the pump is running |
| `pump_run_s` | number | seconds the pump has run without a stop |
| `light_up` | number | the light of the upper camera, 0 to 100 |
| `light_down` | number | the light of the lower camera, 0 to 100 |
| `xy_clear` | bool | every nozzle is raised (at or above `z_safe_mm` less `interlock_z_tolerance_mm`, and going to stay there): the machine may move in X and Y |
| `xy_moving` | bool | the machine has the permission to move in X and Y (see `POST /api/xy`) |
| `xy_blocking` | text | the numbers of the nozzles that are not raised, separated by commas; empty when `xy_clear` |
| `z_safe_mm` | number | the safe height, from the settings |
| `calibrated` | bool | a sensor was calibrated since the start |
| `calibration_allowed` | bool | the settings allow the calibration of the sensors |
| `uptime_s` | number | seconds since the start |
| `selfcheck` | text | `ok`, `warn` or `fail` (see below) |
| `selfcheck_codes` | text | the codes behind the verdict, separated by commas; empty when there are none |
| `nozzles` | list | one entry per nozzle, as in the next section |

## `GET /api/nozzle?n=N`

Needs the token. One nozzle.

| Key | Type | Meaning |
|---|---|---|
| `n` | number | 1 to `nozzle_count` |
| `phase` | text | `idle`, `evacuating` (the valve is open and the vacuum is rising), `holding`, `releasing` (the puff of air) or `fault` |
| `fault` | text | the code of the fault it holds, or `none` |
| `sensor` | text | how the vacuum of this nozzle is sensed, from the settings: `analog` (a pressure sensor read as a number) or `digital` (a vacuum switch read as a level) |
| `alarm` | text | the last thing that went wrong with the vacuum or the sensor: `none`, `vacuum_not_reached`, `part_lost`, `sensor_out_of_range` or `sensor_frozen` |
| `alarm_code` | text | the code of the alarm (see below), or `none` |
| `valve` | bool | the valve of this nozzle is open |
| `blow` | bool | the puff of air is on |
| `part` | bool | a part is held (the nozzle is holding) |
| `vacuum_ok` | bool | the vacuum is there: an analog sensor at or above `vacuum_target_kpa` and not yet under `vacuum_hold_min_kpa` since; a digital sensor with its level stable |
| `raised` | bool | the nozzle is at or above the safe height and not going under it |
| `kpa` | number or null | the vacuum, in kPa below the air pressure, filtered; null for a digital sensor, which does not measure |
| `z_mm` | number | the height of the nozzle |
| `z_target_mm` | number | the height it is going to |
| `a_deg` | number | the angle of the nozzle |
| `a_target_deg` | number | the angle it is turning to |
| `last_result` | text | what the last order ended in: `none`, `evacuating`, `held`, `vacuum_not_reached`, `placed`, `blown`, `vacuum_lost`, `pump_overrun`, `sensor_fault`, `sensor_frozen` or `safe_state` |
| `picks` | number | picks that held a part |
| `failed_picks` | number | picks that did not reach the vacuum |
| `places` | number | parts released |
| `losses` | number | parts lost while held |

## The requests that move or change

They need the token (a node with no token refuses them with `401`). A word that does not fit is a `400` with `{"error":"bad_request","key":"<word>"}`. The answer of an order is `{"ok":true,"why":"ok","n":N,"nozzle":{...}}`; when `ok` is false `why` says what stopped it and the status is `409` (`400` for `bad_nozzle` and `out_of_range`).

* `POST /api/nozzle/pick?n=N` opens the valve and answers when the vacuum says if a part is held: `ok` with `why` `held`, or not `ok` with `vacuum_not_reached` (the valve closes again). Refused with `safe_state`, `fault` or `busy`.
* `POST /api/nozzle/place?n=N` releases the part (valve closed, a short puff) and answers when it is done, with `why` `placed`. Refused with `not_holding`, and with `interlock_release_high` while the nozzle is above the release height of the settings.
* `POST /api/nozzle/blow?n=N` a puff of air alone, with `why` `blown`; refused with `holding` while a part is held.
* `POST /api/z?n=N&mm=H` moves the nozzle to the height H and answers when it is there; if the head goes to its safe state or the nozzle gets a fault before it arrives, the answer is not `ok` (`safe_state`, `fault` or `not_finished`). Refused with `out_of_range`, and, when it would go down, with `safe_state`, `fault` or `interlock_rotate_low` (the nozzle is turning and H is under the turning height).
* `POST /api/rotate?n=N&deg=D` turns the nozzle to the angle D and answers when it is there; a turn that does not arrive is answered like a move of the height that does not. Refused with `out_of_range`, `safe_state`, `fault`, and `interlock_rotate_low` while the nozzle is under the turning height (or going there).
* `POST /api/light?up=P&down=Q` sets the lights in percent; a word that is left out stays as it is. In the safe state only 0 is accepted (`safe_state`).
* `POST /api/stop` stops every move where it is. The vacuum is not touched: a stop does not drop a part.
* `POST /api/ack[?n=N]` clears the fault of a nozzle whose cause is gone (`cleared`, `still_active` while a sensor still reads what cannot be real, `not_held` when there was nothing). Without `n` it clears the safe state of the head.
* `POST /api/vacuum/calibrate?n=N&kind=zero|scale[&kpa=K]` sets the zero (the sensor open to the air) or the scale (a known vacuum K, 1 to 200 kPa, on it) of one analog sensor from its present reading. Refused with `disabled` unless the settings allow it, `not_analog` for a nozzle with a digital sensor (a switch has no scale), `busy` while a valve is open or the pump runs, `no_reading` or `bad_reading`.
* A request line without its HTTP version is a line cut short and answers `400`, and so does a field that is named and cannot be read (`n=`, `up=`): it is never taken as left out.
* `POST /api/xy?moving=1|0` is how the machine asks to move in X and Y (1) and says it has stopped (0). The permission is given only while every nozzle is raised; otherwise the answer is `409` with `interlock_xy_nozzle_low` and `xy_blocking` of the status says which nozzles are down. While the machine has the permission, a move of a nozzle to a height under the safe height is refused with `interlock_xy_moving`. Letting the permission go is always accepted. The node does not stop the axes of the machine: it says whether they may move and refuses to lower a nozzle while they do. In the safe state a new permission is refused (`safe_state`) until a person acknowledges; letting it go is still accepted.

## The self-check

The node judges itself and says it in `selfcheck` and `selfcheck_codes`. It never moves anything to do so. The verdict is `fail` when any code of severity `fail` is raised, `warn` when the worst is `warn`, `ok` otherwise (`info` codes do not change it). A code is never reused for another meaning.

| Code | Severity | Raised when |
|---|---|---|
| `HDN-S01` | warn | the node has no token: it refuses every command that moves or changes something |
| `HDN-S02` | fail | the head is in its safe state |
| `HDN-F01` | warn | a nozzle holds a fault |
| `HDN-V01` | info | a nozzle holds the alarm `vacuum_not_reached` |
| `HDN-C01` | info | the calibration of the sensors is allowed in the settings |

## How the vacuum is sensed

Each nozzle has its own sensor kind in the settings, `nozzle_1_sensor` to `nozzle_6_sensor`: `analog` or `digital`. The rules of the pick, the hold and the loss are the same for both; only the way the vacuum is read differs.

| | analog | digital |
|---|---|---|
| What is read | a pressure sensor, as a number | a vacuum switch, as a level |
| Vacuum is there | `kpa` reaches `vacuum_target_kpa` | the level has been the vacuum level for `digital_debounce_ms` (`digital_active_high` says which level that is) |
| Vacuum is gone (held part) | `kpa` under `vacuum_hold_min_kpa` for `vacuum_loss_ms` | the level has been the other one for `digital_debounce_ms` and then for `vacuum_loss_ms` |
| Hysteresis | the band between `vacuum_hold_min_kpa` and `vacuum_target_kpa` | the switch has its own; the node adds the debounce and the loss time |
| Filter | the average of `pressure_samples` readings | the debounce |
| Calibration | the zero and the scale (`POST /api/vacuum/calibrate`) | none: refused with `not_analog` |
| Sensor out of range | a reading that cannot be real (`HEAD-S01`) | a level that is neither high nor low for `digital_debounce_ms` (`HEAD-S01`) |
| Sensor frozen | no change at all for `pressure_stuck_ms` while a valve is open (`HEAD-S02`) | vacuum said for `pressure_stuck_ms` with the valve closed and nothing pulling (`HEAD-S02`) |
| `kpa` in the answers | the filtered vacuum | null |

## The alarms of a nozzle

An alarm is the last thing that went wrong with the vacuum or the sensor of a nozzle. It stays until a person acknowledges it with `POST /api/ack?n=N` or, for `vacuum_not_reached`, until the next pick starts. A nozzle that did not reach the vacuum is not in a fault state: it is idle and can try again.

| Code | Alarm | Raised when |
|---|---|---|
| `HEAD-V01` | `vacuum_not_reached` | a pick did not reach the vacuum within `vacuum_settle_ms` (no part, a part that does not seal, a blocked hose): the valve closed again |
| `HEAD-V02` | `part_lost` | a held part lost its vacuum |
| `HEAD-S01` | `sensor_out_of_range` | the sensor reads what cannot be real |
| `HEAD-S02` | `sensor_frozen` | the sensor does not change when it should, or says vacuum when nothing is pulling |

## The faults of a nozzle

A nozzle that holds a fault takes no pick and no turn, and is not lowered, until it is acknowledged.

| Code | Held when |
|---|---|
| `HEAD-V02` | a held part lost its vacuum (it was under the hold limit for `vacuum_loss_ms`); the valve closes |
| `HEAD-P01` | the pump ran longer than `pump_max_run_s` without a stop; the valves close |
| `HEAD-S01` | the sensor reads what cannot be real: a number out of range, or a level that is neither high nor low |
| `HEAD-S02` | the sensor is frozen: a number that does not change at all for `pressure_stuck_ms` while its valve is open, or a switch that says vacuum with nothing pulling |
| `HEAD-L01` | not a fault of a nozzle: the code of the safe state of the head, given as `safe_code` |

## The interlocks and the safe state

* With no order, the pump is stopped and every valve is closed. The pump runs only while a valve is open, and never longer than `pump_max_run_s`.
* A part is released only at or below `release_max_z_mm`; a nozzle is turned only at or above `rotate_min_z_mm`, and is not lowered under it while it is turning.
* A nozzle that holds a fault, or any nozzle while the head is safe, is not lowered; going up is always allowed.
* The machine may move in X and Y only with every nozzle raised. "Raised" is a height: `z_safe_mm`, less `interlock_z_tolerance_mm`. `rotate_min_z_mm` is the height for turning a nozzle and plays no part in this interlock, whatever its value. The head says it in `xy_clear` and `xy_blocking`, grants the permission of `POST /api/xy?moving=1` only when every nozzle is raised, and while the machine holds it refuses to lower a nozzle under that height (`interlock_xy_moving`). The permission is released with `moving=0`, or by a person's acknowledgement of the safe state.
* When the server (a request with the token) is not heard for `link_lost_ms` the head is safe: the pump stops, the valves close (unless `safe_vacuum` is `hold`), the nozzles go up to `z_safe_mm` (unless `safe_z_retract` is off), the lights go out, and no pick, release, turn or lowering is accepted until a person acknowledges with `POST /api/ack`. The server must therefore ask something with the token at least once in that time, **also while it waits for the answer of an order**. The head says that time in `link_lost_ms` of `GET /api/status`, so a server does not have to guess it; asking for the status without the token reads it and does not count as the server being heard.
* A move, a pick or a puff that lasts longer than `link_lost_ms` does not by itself make the head safe: while the node waits it goes on listening, and a request with the token (a read on a second connection) counts as the server being heard. The watch is the same: if nothing is heard for `link_lost_ms` in the middle of an order, the head is safe at once, the move stops there (the nozzle only goes on up to `z_safe_mm`, unless `safe_z_retract` is off) and the order is answered `safe_state`. A task that did not run for that long is judged the same way before anything new is heard. It is also judged before a request counts: the order that ends a silence that was already too long is refused like any other in the safe state.
* While an order waits, reads are answered, `POST /api/stop` and `POST /api/xy?moving=0` are carried out, and any other order is answered `409` with `busy`.
* The firmware is not written yet: the way it must serve the network while it waits (`Io::listen` in `core/head_sim.hpp`) has only run in the simulator.

## The simulator

`core/head_sim.hpp` is a head on a computer: a pump and valves that make a vacuum, parts that seal a nozzle or not, pressure sensors with a little noise, vacuum switches that make and let go at their own pressures, and faults that can be switched on (a part that falls, a pump that does not pull, a sensor that freezes or reads what cannot be real, a switch that bounces, sticks on or reads a level that is neither high nor low). The random numbers come from a seed, so a run is the same every time. `tests/test_head.cpp` and `tests/test_node.cpp` run it.
