# EH3D-NODE-PNP-HEAD: requirements

This register is the single source of the state of each capability: `STATUS.md`, the README files and the capability matrix of the documentation hub cite it and never state a state that it does not. A state is one of `planned`, `prototype`, `implemented` and `validated`. `validated` means that a person has run the capability on real hardware and the evidence of the manifest says `bench` or `field`; nothing here is validated, because nothing of this node has run on a board.

The register has 50 requirements: 41 implemented, 0 prototype, 9 planned and 0 validated. By progress: 41 done, 0 partial, 5 pending and 4 blocked. High coverage is not proof of correctness, so each row says how it is checked: the test that names it, and what it shows.

## How to read the register

* **ID:** `PNPH-NNN`, never reused and never renumbered; a new capability takes the next number.
* **State:** `implemented` is code that is complete and tested on a computer, `prototype` is part of the capability, `planned` is not written.
* **Progress:** `done`, `partial`, `pending` (can be written without hardware) or `blocked` (waits for hardware or for another program of the suite).
* **Evidence:** `simulator` when the test runs against the simulated head, `host-test` when it checks a calculation or a reader with no simulated world, `none` when nothing is written yet.
* **Test:** the acceptance criterion: `file::name` is the test `name` of `tests/file.cpp`, which must exist. For a capability that is not written the cell says `planned:` and the name of the test that will show it; the day that test exists, the row must change.
* **Limits:** what is not done or not proven; a row that is `partial` or `blocked` always says it.

The check is `tools/test_node.py`: it reads this table, finds each test and fails when a state, a test or a count does not agree.

## Settings

| ID | Requirement | State | Progress | Evidence | Test | Limits |
|---|---|---|---|---|---|---|
| PNPH-001 | The node is a plain settings file; a value out of range or a key nobody knows is refused with its key and the default stays. | implemented | done | host-test | `test_config::a_value_out_of_range_or_a_key_nobody_knows_is_reported_and_the_default_stays` | - |
| PNPH-002 | Settings that disagree with each other (the vacuum limits, the travel of Z and the heights of the interlocks) go back to the defaults, the safe way. | implemented | done | host-test | `test_config::settings_that_disagree_go_back_to_the_defaults` | - |
| PNPH-003 | The token is never written back to the settings text, and it has a limit of length. | implemented | done | host-test | `test_config::the_settings_text_is_read_back_the_same_and_never_holds_the_token` | - |
| PNPH-004 | The defaults are the conservative ones: no token, calibration off, the pump and the valves off in the safe state. | implemented | done | host-test | `test_config::defaults_are_the_conservative_ones` | - |
| PNPH-005 | The example file shows every setting with its default and its range, and it is the defaults. | implemented | done | host-test | `test_config::the_example_file_is_the_defaults_names_every_key_and_has_no_problem` | - |

## The sensors

| ID | Requirement | State | Progress | Evidence | Test | Limits |
|---|---|---|---|---|---|---|
| PNPH-006 | The reading of a pressure sensor is the average of the last readings, turned into kPa with a zero and a scale from the settings. | implemented | done | host-test | `test_config::the_filter_averages_the_last_readings_and_the_scale_turns_them_into_kpa` | The sensor and its ADC are not written. |
| PNPH-007 | A reading that cannot be real is a sensor fault that is not cleared while it lasts. | implemented | done | host-test | `test_config::a_reading_that_cannot_be_real_is_a_sensor_fault_and_is_not_cleared_while_it_lasts` | - |
| PNPH-008 | A reading far below the air pressure is also a sensor fault. | implemented | done | host-test | `test_config::a_negative_reading_far_below_the_air_pressure_is_also_a_sensor_fault` | - |
| PNPH-009 | The calibration of the sensors is refused unless the settings allow it. | implemented | done | host-test | `test_config::calibration_is_refused_unless_it_is_switched_on` | - |
| PNPH-010 | The zero and the scale are set from the present readings, and nonsense is refused. | implemented | done | host-test | `test_config::the_zero_and_the_scale_are_set_from_the_readings` | Not compared with a gauge. |
| PNPH-011 | A calibration is refused while a valve is open or the pump runs. | implemented | done | host-test | `test_config::a_calibration_is_refused_while_a_valve_is_open` | - |
| PNPH-012 | A sensor that does not change at all while a valve is open is a dead sensor. | implemented | done | simulator | `test_head::a_sensor_that_does_not_change_while_a_valve_is_open_is_a_dead_sensor` | - |
| PNPH-013 | A sensor that reads what cannot be real stops that nozzle only. | implemented | done | simulator | `test_head::a_sensor_that_reads_what_cannot_be_real_stops_that_nozzle_only` | - |

## Picking and placing

| ID | Requirement | State | Progress | Evidence | Test | Limits |
|---|---|---|---|---|---|---|
| PNPH-014 | A pick over a part holds it and runs the pump. | implemented | done | simulator | `test_head::a_pick_over_a_part_holds_it_and_runs_the_pump` | The vacuum, the part and the delays are simulated. |
| PNPH-015 | A pick over nothing fails, closes the valve and stops the pump. | implemented | done | simulator | `test_head::a_pick_over_nothing_fails_closes_the_valve_and_stops_the_pump` | - |
| PNPH-016 | A place at the board lets the part go with a puff and the pump stops when it is the last valve. | implemented | done | simulator | `test_head::a_place_at_the_board_lets_the_part_go_with_a_puff_and_the_pump_stops` | - |
| PNPH-017 | A part that falls while held is a lost part and a fault until a person acknowledges. | implemented | done | simulator | `test_head::a_part_that_falls_while_held_is_a_lost_part_and_a_fault_until_acknowledged` | - |
| PNPH-018 | A short dip of the vacuum is not a loss. | implemented | done | simulator | `test_head::a_short_dip_of_the_vacuum_is_not_a_loss` | - |
| PNPH-019 | A pump that does not pull gives a failed pick even over a part. | implemented | done | simulator | `test_head::a_pump_that_does_not_pull_gives_a_failed_pick_even_over_a_part` | - |
| PNPH-020 | Two nozzles share the pump, which stops when the last valve closes. | implemented | done | simulator | `test_head::two_nozzles_share_the_pump_and_it_stops_when_the_last_valve_closes` | - |
| PNPH-021 | A puff of air alone is refused while a part is held. | implemented | done | simulator | `test_head::a_blow_alone_is_refused_while_a_part_is_held` | - |
| PNPH-022 | Two hundred cycles with a few missing parts add up, and a run with the same seed is the same run. | implemented | done | simulator | `test_head::two_hundred_cycles_with_a_few_missing_parts_add_up_and_are_the_same_every_time` | - |

## The interlocks and the safe state

| ID | Requirement | State | Progress | Evidence | Test | Limits |
|---|---|---|---|---|---|---|
| PNPH-023 | With no command the pump is stopped and every valve is closed, and nothing moves. | implemented | done | simulator | `test_head::a_head_with_no_command_for_a_minute_has_done_nothing` | - |
| PNPH-024 | A part is not released from a height and is still held. | implemented | done | simulator | `test_head::a_part_is_not_released_from_a_height_and_is_still_held` | The height of the board is a setting, not a measure. |
| PNPH-025 | A nozzle is turned only when high enough, and is not lowered while it turns. | implemented | done | simulator | `test_head::a_nozzle_is_turned_only_when_high_enough_and_not_lowered_while_turning` | - |
| PNPH-026 | The travel of Z and the turn of each nozzle are kept. | implemented | done | simulator | `test_head::the_ranges_of_the_axes_are_kept` | The soft limits are not tied to real end stops. |
| PNPH-027 | A pump that runs too long is stopped and the nozzles that used it hold a fault. | implemented | done | simulator | `test_head::a_pump_that_runs_too_long_is_stopped_and_the_nozzles_that_used_it_hold_a_fault` | - |
| PNPH-028 | A stop ends the moves and does not drop the part. | implemented | done | simulator | `test_head::a_stop_ends_the_moves_and_does_not_drop_the_part` | This is not an emergency stop. |
| PNPH-029 | When the server is lost the pump stops, the valves close, the nozzles go up and the lights go out. | implemented | done | simulator | `test_head::when_the_server_is_lost_the_pump_stops_the_valves_close_and_the_nozzles_go_up` | A part held at that moment may fall: the setting safe_vacuum can hold it. |
| PNPH-030 | A person acknowledges the safe state and the head works again. | implemented | done | simulator | `test_head::a_person_acknowledges_the_safe_state_and_the_head_works_again` | - |
| PNPH-031 | With the safe vacuum on hold the part is kept when the server is lost. | implemented | done | simulator | `test_head::with_the_safe_vacuum_on_hold_the_part_is_kept_when_the_server_is_lost` | - |
| PNPH-032 | Without the retract the nozzles stay where they are in the safe state (a setting). | implemented | done | simulator | `test_head::without_the_retract_the_nozzles_stay_where_they_are` | - |
| PNPH-033 | A server that keeps asking keeps the head awake. | implemented | done | simulator | `test_head::a_server_that_keeps_asking_keeps_the_head_awake` | - |
| PNPH-034 | The two lights follow the request and keep the one that is left out. | implemented | done | simulator | `test_head::the_lights_follow_the_request_and_keep_the_one_that_is_left_out` | The lights are numbers; no LED driver is written. |

## The node on the network

| ID | Requirement | State | Progress | Evidence | Test | Limits |
|---|---|---|---|---|---|---|
| PNPH-035 | The status is always answered and is valid JSON. | implemented | done | simulator | `test_node::the_status_is_always_answered_and_is_valid_json` | - |
| PNPH-036 | A node without a token refuses everything that moves or changes something, and says so in its check. | implemented | done | simulator | `test_node::a_node_without_a_token_refuses_everything_that_moves_and_says_so` | - |
| PNPH-037 | The token goes in the header or in the query, and a wrong or a shorter one is refused. | implemented | done | host-test | `test_node::the_token_goes_in_the_header_or_in_the_query_and_a_wrong_one_is_refused` | - |
| PNPH-038 | A bad request is a 400 with the word that is wrong, and an unknown path or verb is a 404 or a 405. | implemented | done | host-test | `test_node::a_bad_request_is_a_400_with_the_word_that_is_wrong` | - |
| PNPH-039 | The calibration is refused by default with the reason, and allowed when the settings say so. | implemented | done | simulator | `test_node::the_calibration_is_refused_by_default_with_the_reason` | - |
| PNPH-040 | A nozzle that holds a fault and a safe head show in the check of the node. | implemented | done | simulator | `test_node::a_nozzle_that_holds_a_fault_and_a_safe_head_show_in_the_check` | - |
| PNPH-041 | The words of the status are the words of the contract page, and every code, route and refusal of the node is in it. | implemented | done | host-test | `test_node::the_status_words_are_the_words_of_the_page_of_the_contract` | - |

## Not written yet

| ID | Requirement | State | Progress | Evidence | Test | Limits |
|---|---|---|---|---|---|---|
| PNPH-042 | The firmware for the ESP32-S3 (network, panel, update, storage) on the shared base. | planned | pending | none | `planned:test_node::the_firmware_answers_the_words_of_the_contract` | Only ten plain helpers of the shared base are imported; the network, panel and update parts of the base are not. |
| PNPH-043 | The outputs of the pump, the valves and the puff of air, with a driver for each and a read-back that tells that a valve really opened. | planned | blocked | none | `planned:test_head::an_output_that_does_not_follow_is_a_fault` | Waits for a pump, valves and drivers on a bench. |
| PNPH-044 | The reading of the pressure sensors through their converter, with the noise of a real line. | planned | blocked | none | `planned:test_config::the_converter_gives_the_raw_reading_of_each_sensor` | Waits for the sensors on a bench; the limits of the settings are unproven. |
| PNPH-045 | The drivers of the Z and rotation axes (steppers with a UART driver, with the current, the micro-steps and a sensorless stall homing). | planned | blocked | none | `planned:test_head::the_axes_home_and_repeat_to_the_same_position` | Waits for the motors; today the axes are a model. |
| PNPH-046 | Generic actuators set by pin (on or off, PWM, inverted, a longest time) for whatever else a head has. | planned | pending | none | `planned:test_head::a_generic_actuator_follows_its_pin_and_its_longest_time` | - |
| PNPH-047 | A self-check at the start of the pump, the valves, the sensors, the lights and the end stops, with codes. | planned | pending | none | `planned:test_node::the_self_check_at_the_start_finds_a_valve_that_does_not_open` | The check of the node on itself exists; the one that touches the hardware does not. |
| PNPH-048 | The presence sensors of the nozzles in the rack and the state of the changer. | planned | blocked | none | `planned:test_head::the_rack_tells_which_nozzle_is_on_the_head` | Waits for the rack and its sensors. |
| PNPH-049 | The signal that tells the controller of the machine that the nozzles are lifted, for the interlocks of the moves in X and Y. | planned | pending | none | `planned:test_node::the_status_says_when_every_nozzle_is_lifted` | Which height counts as lifted is not decided. |
| PNPH-050 | A profile for a paste dispenser (a pump, a pulse width, a retraction, a priming and a pressure). | planned | pending | none | `planned:test_head::a_paste_pulse_has_its_width_and_its_retraction` | The decision about paste is open. |
