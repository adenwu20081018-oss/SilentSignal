# Recording real test results

The CSV is deliberately header-only. No trial has been invented. Copy it to a new file before recording observations.

| Field | Meaning / allowed values |
| --- | --- |
| trial_id | Unique episode or session identifier |
| status | `complete`, `incomplete`, or `excluded`; only complete records are scored |
| trial_type | `acoustic` for one independent episode, `background` for timed target-free exposure, `control` for UI/output checks |
| scenario_id | A01–A16 or U01–U14 from the testing document |
| date, firmware_revision, hardware_revision | Actual session date, deployed firmware hash/version, assembled board/rework revision |
| room, source_description, source_type | Room and exact source; `real` or `playback` plus setup details |
| distance_m, intensity, background, microphone_orientation | Measured distance or blank; qualitative knock strength; background and placement |
| mode | `normal`, `knock`, `doorbell` |
| ground_truth | `none`, `other`, `knock`, `doorbell`; assigned from the actual source, never from the device label |
| source_duration_s, response_window_s | Actual duration and predeclared scoring window; keep matched conditions consistent |
| observed_label | First **new** displayed event: `none`, `other`, `knock`, `doorbell`; use `none` if no new event even when stale text remains |
| alert_observed | `yes` or `no` for a fresh physical alert; record motor and LED details separately |
| led_observed, oled_observed, vibration_observed | Actual outputs; use `not_observed` when unknown, never silently assume a command worked |
| latency_ms | Measured onset-to-output latency only; otherwise blank; name output and method in notes |
| confirmation_result, mode_switch_result | `pass`, `fail`, `not_tested` plus details in notes |
| exposure_s | Positive actual background-only listening duration in seconds; required for background rows |
| false_alert_activations | Nonnegative integer fresh target alert activations in a background session; acknowledge consistently |
| rearm_unavailable_s | Time not independently receptive because alert/ack/rearm was in progress; record separately, do not subtract silently |
| evidence_file, notes | Relative recording/log reference, later labels, exclusions, uncertainties and unusual behaviour |

`alert_observed` means a newly observed notification activation, not a retained OLED label. Record which outputs worked in the adjacent fields. If no reliable alert observation exists, mark the row incomplete so it is excluded from the combined analysis. Output correctness still needs manual review: the script does not infer tactile performance from an OLED string.

Run `python Results/analyze_results.py Results/your-records.csv` from the repository root. Output is JSON on standard output; redirect to a new file if desired. It validates required scoring fields, rejects duplicate trial IDs, excludes incomplete/control rows, separates modes and keeps background exposure out of event metrics. Null ratios mean no denominator. It does not pool filtered-mode labels into raw recognition accuracy.

The script reports aggregation by mode. For condition comparisons, pass separate CSV subsets for each distance, background, source type and firmware revision; never compare pooled totals as if their conditions were matched. Preserve the original complete log. Specify whether background exposure includes acknowledgment downtime; this utility uses the full recorded `exposure_s` and reports an operational rate under that acknowledgment procedure.

This is an observational analysis tool, not a firmware replay harness. The accompanying software checks use synthetic in-memory fixtures solely to validate accounting; they are not SilentSignal recognition measurements and are not included in the trial template.
