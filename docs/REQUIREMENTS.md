# EH3D-NODE-PNP-HEAD: requirements

This register is the single source of the state of each capability: `STATUS.md`, the README files and the capability matrix of the documentation hub cite it and never state a state that it does not. A state is one of `planned`, `prototype`, `implemented` and `validated`. `validated` means that a person has run the capability on real hardware and the evidence of the manifest says `bench` or `field`; nothing here is validated, because nothing of this node has run on a board.

The register has 89 requirements: 80 implemented, 0 prototype, 9 planned and 0 validated. By progress: 80 done, 0 partial, 4 pending and 5 blocked. High coverage is not proof of correctness, so each row says how it is checked: the test that names it, and what it shows.

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
| PNPH-044 | The reading of the pressure sensors through their converter, and of the vacuum switches through their inputs, with the noise of a real line. | planned | blocked | none | `planned:test_config::the_converter_gives_the_raw_reading_of_each_sensor` | Waits for the sensors and the switches on a bench; the limits of the settings are unproven. |
| PNPH-045 | The drivers of the Z and rotation axes (steppers with a UART driver, with the current, the micro-steps and a sensorless stall homing). | planned | blocked | none | `planned:test_head::the_axes_home_and_repeat_to_the_same_position` | Waits for the motors; today the axes are a model. |
| PNPH-046 | Generic actuators set by pin (on or off, PWM, inverted, a longest time) for whatever else a head has. | planned | pending | none | `planned:test_head::a_generic_actuator_follows_its_pin_and_its_longest_time` | - |
| PNPH-047 | A self-check at the start of the pump, the valves, the sensors, the lights and the end stops, with codes. | planned | pending | none | `planned:test_node::the_self_check_at_the_start_finds_a_valve_that_does_not_open` | The check of the node on itself exists; the one that touches the hardware does not. |
| PNPH-048 | The presence sensors of the nozzles in the rack and the state of the changer. | planned | blocked | none | `planned:test_head::the_rack_tells_which_nozzle_is_on_the_head` | Waits for the rack and its sensors. |
| PNPH-049 | The signal that tells the controller of the machine that the nozzles are lifted as a line on a pin, for a controller that does not speak HTTP. | planned | blocked | none | `planned:test_node::the_xy_clear_line_follows_the_nozzles` | Waits for the board; the signal exists today as `xy_clear` in the status and the permission of `POST /api/xy`. |
| PNPH-050 | A profile for a paste dispenser (a pump, a pulse width, a retraction, a priming and a pressure). | planned | pending | none | `planned:test_head::a_paste_pulse_has_its_width_and_its_retraction` | The decision about paste is open. |

## The sensors of the nozzles

| ID | Requirement | State | Progress | Evidence | Test | Limits |
|---|---|---|---|---|---|---|
| PNPH-051 | Each nozzle has its own sensor kind in the settings (analog or digital), analog is the default, and the kinds are read back the same. | implemented | done | host-test | `test_sensor::each_nozzle_has_its_own_sensor_kind_in_the_settings` | - |
| PNPH-052 | A sensor kind nobody knows, or a nozzle that does not exist, is refused with its key and the default stays. | implemented | done | host-test | `test_sensor::a_sensor_kind_nobody_knows_or_a_nozzle_that_does_not_exist_is_refused` | - |
| PNPH-053 | A debounce as long as the pick is allowed to take goes back to the default. | implemented | done | host-test | `test_sensor::a_debounce_as_long_as_the_pick_is_put_back_to_the_default` | - |
| PNPH-054 | The example file names the sensor of every nozzle and the keys of the switches. | implemented | done | host-test | `test_sensor::the_example_file_names_the_sensor_of_every_nozzle_and_the_keys_of_the_switches` | - |
| PNPH-055 | A digital sensor holds a part the same way as an analog one and says no pressure. | implemented | done | simulator | `test_sensor::a_digital_sensor_holds_a_part_the_same_way_and_says_no_pressure` | The switch is simulated with made-up pressures; no real switch has been tried. |
| PNPH-056 | A digital pick over nothing fails, closes the valve and leaves the alarm of the pick. | implemented | done | simulator | `test_sensor::a_digital_pick_over_nothing_fails_and_leaves_the_alarm_of_the_pick` | The switch is simulated. |
| PNPH-057 | A digital part that falls is lost after the loss time and the nozzle holds the fault. | implemented | done | simulator | `test_sensor::a_digital_part_that_falls_is_lost_after_the_loss_time_and_the_nozzle_holds_the_fault` | The switch is simulated. |
| PNPH-058 | A bouncing switch is filtered by the debounce. | implemented | done | simulator | `test_sensor::a_bouncing_switch_is_filtered_by_the_debounce` | The bounce is made up; the real one is measured on a bench. |
| PNPH-059 | An active-low switch is read with the other polarity. | implemented | done | simulator | `test_sensor::an_active_low_switch_is_read_with_the_other_polarity` | The switch is simulated. |
| PNPH-060 | A level between high and low is a sensor out of range until it is gone. | implemented | done | simulator | `test_sensor::a_level_between_high_and_low_is_a_sensor_out_of_range_until_it_is_gone` | What a real unplugged line gives is a bench test. |
| PNPH-061 | A switch that says vacuum with nothing pulling is a stuck switch. | implemented | done | simulator | `test_sensor::a_switch_that_says_vacuum_with_nothing_pulling_is_a_stuck_switch` | The switch is simulated. |
| PNPH-062 | A switch whose level does not change while a part is held is not a fault. | implemented | done | simulator | `test_sensor::a_switch_that_does_not_change_is_not_a_fault_while_a_part_is_held` | - |
| PNPH-063 | An analog nozzle keeps its band between the target and the hold limit. | implemented | done | host-test | `test_sensor::an_analog_nozzle_keeps_its_band_between_the_target_and_the_hold_limit` | - |
| PNPH-064 | The filter of an analog nozzle averages the readings and a switch has none. | implemented | done | host-test | `test_sensor::the_filter_of_an_analog_nozzle_averages_the_readings_and_a_switch_has_none` | - |
| PNPH-065 | The calibration is for the analog sensors only; a switch is refused with not_analog. | implemented | done | simulator | `test_sensor::the_calibration_is_for_the_analog_sensors_only` | - |
| PNPH-066 | A frozen pressure sensor and one out of range have their own alarms and codes. | implemented | done | simulator | `test_sensor::a_frozen_pressure_sensor_and_one_out_of_range_have_their_own_alarms` | - |
| PNPH-067 | An analog pick over nothing leaves the same alarm, and the acknowledgement clears it. | implemented | done | simulator | `test_sensor::an_analog_pick_over_nothing_leaves_the_same_alarm_and_ack_clears_it` | - |
| PNPH-068 | Digital and analog nozzles work together on one head. | implemented | done | simulator | `test_sensor::digital_and_analog_nozzles_work_together_on_one_head` | The sensors are simulated. |
| PNPH-069 | The same seed gives the same run with a switch and another seed another run. | implemented | done | simulator | `test_sensor::the_same_seed_gives_the_same_run_with_a_switch_and_another_seed_another` | - |
| PNPH-070 | The page of the contract names the sensors, the alarms and the words of the interlock, and the page of the hardware lists the bench test of each. | implemented | done | host-test | `test_sensor::the_page_of_the_contract_names_the_sensors_the_alarms_and_the_xy_words` | - |

## The interlock of the machine in X and Y

| ID | Requirement | State | Progress | Evidence | Test | Limits |
|---|---|---|---|---|---|---|
| PNPH-071 | The machine may move in X and Y only when every nozzle is raised to the safe height; the status says which nozzles block it. | implemented | done | simulator | `test_sensor::the_machine_may_move_in_xy_only_when_every_nozzle_is_raised_to_the_safe_height` | The node says whether the axes may move; it does not stop the axes of the machine. |
| PNPH-072 | The safe height has a tolerance, and a nozzle is raised by where it is going as well as where it is. | implemented | done | simulator | `test_sensor::the_safe_height_has_a_tolerance_and_a_nozzle_is_raised_by_where_it_goes_too` | The height at which a nozzle is really clear of the board is a bench test. |
| PNPH-073 | While the machine moves in X and Y no nozzle is lowered under the safe height. | implemented | done | simulator | `test_sensor::while_the_machine_moves_in_xy_no_nozzle_is_lowered_under_the_safe_height` | - |
| PNPH-074 | The height for turning a nozzle plays no part in the interlock of X and Y, whatever its value. | implemented | done | simulator | `test_sensor::rotate_min_z_does_not_decide_the_xy_interlock` | - |
| PNPH-075 | A raised nozzle that holds a fault does not block the axes, a low one does, and going up is always allowed. | implemented | done | simulator | `test_sensor::a_raised_nozzle_that_holds_a_fault_does_not_block_xy_but_a_low_one_does` | - |
| PNPH-076 | The permission to move is released by a person after the safe state. | implemented | done | simulator | `test_sensor::the_permission_to_move_in_xy_is_released_by_a_person_after_the_safe_state` | - |
| PNPH-077 | Without the retract of the safe state, a low nozzle keeps blocking the axes. | implemented | done | simulator | `test_sensor::the_safe_state_without_retract_leaves_a_low_nozzle_blocking_xy` | - |
| PNPH-078 | A bad request for the permission is a 400, and a node without a token refuses it. | implemented | done | host-test | `test_sensor::a_bad_xy_request_is_a_400_and_a_node_without_a_token_refuses_it` | - |
| PNPH-079 | A move or a turn longer than the link watch finishes while the server is heard. | implemented | done | simulator | `test_node::a_move_longer_than_the_link_watch_arrives_while_the_server_is_heard` | - |
| PNPH-080 | A vacuum wait longer than the link watch ends by the vacuum while the server is heard. | implemented | done | simulator | `test_node::a_vacuum_wait_longer_than_the_link_watch_ends_by_the_vacuum_while_the_server_is_heard` | - |
| PNPH-081 | A link lost in the middle of a move or a turn stops it, and the nozzle goes up. | implemented | done | simulator | `test_node::a_link_lost_half_way_stops_the_move_and_the_nozzle_goes_up` | The time it takes on a board is a bench test. |
| PNPH-082 | Without the retract, a link lost in the middle of a move stops the nozzle where it is. | implemented | done | simulator | `test_node::a_link_lost_half_way_without_the_retract_stops_the_nozzle_where_it_is` | - |
| PNPH-083 | A link lost during the vacuum wait closes the valve and stops the pump. | implemented | done | simulator | `test_node::a_link_lost_during_the_vacuum_wait_closes_the_valve_and_stops_the_pump` | - |
| PNPH-084 | A task that did not run for longer than the link watch ends in the safe state, even if the server asks right after. | implemented | done | simulator | `test_node::a_task_that_hangs_ends_in_the_safe_state_even_if_the_server_asks_right_after` | A task that never runs again needs the watchdog of the board, with the firmware. |
| PNPH-085 | A request with a wrong token does not keep the link. | implemented | done | simulator | `test_node::a_read_with_a_wrong_token_during_a_move_does_not_keep_the_link` | - |
| PNPH-086 | While an order waits, a stop is carried out and another order is busy. | implemented | done | simulator | `test_node::during_a_wait_a_stop_is_carried_out_and_another_order_is_busy` | - |
| PNPH-087 | A puff cut short by the safe state is not answered as done. | implemented | done | simulator | `test_node::a_puff_cut_short_by_the_safe_state_is_not_answered_as_done` | - |
| PNPH-088 | The status says the time of the watch over the server, with or without the token: the value of the settings the head runs with. | implemented | done | host-test | `test_node::the_status_says_the_link_time_of_the_settings_with_or_without_the_token` | - |
| PNPH-089 | The time the status says is the time the head keeps, and reading the status without the token does not keep the link. | implemented | done | simulator | `test_node::the_link_time_of_the_status_is_the_time_the_head_keeps` | The time it takes on a board is a bench test. |
