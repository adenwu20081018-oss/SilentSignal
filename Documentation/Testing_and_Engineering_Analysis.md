# SilentSignal — Testing and Engineering Analysis

## Evidence and current results

The builder has personally tested SilentSignal and reports that its overall recognition performance is quite good for an initial prototype. This is a qualitative assessment of the tested device, not a measured accuracy result. The supplied material does not identify trial counts, distances, sound levels, doorbell models, background conditions, or missed and false detection counts.

| Evidence | What it establishes | What it does not establish |
| --- | --- | --- |
| Builder's first-hand report | Encouraging overall recognition during personal testing | Performance for any individual scenario below; a detection range or accuracy percentage |
| Photograph of `SOUND KNOCK`, `ALERT ON`, red illumination | The assembled device displayed a knock alert state | What sound caused it, latency, repeatability, or motor operation |
| Photograph of `SOUND DOORBELL`, `ALERT ON`, green illumination | The assembled device displayed a doorbell alert state | Generalization to different doorbells or measured recognition performance |
| Photograph with blue illumination | A distinct visible indication exists | The acoustic source or successful rejection of every non-target sound |
| Supplied C source | Implemented classification rules, modes, scheduling, and output commands | That the photographed device ran exactly this revision or that commands produced the intended electrical response |
| Trial logs, labeled audio, demonstration videos | None located in the supplied project tree | No confusion matrix, false-alert rate, or latency distribution can yet be reported |

**No simulated recognition data or invented accuracy percentages are used. All scenario-specific outcomes below remain unmeasured.** Proposed values are experimental settings, not claimed capabilities.

## What the firmware actually tests acoustically

Sources: [main.c](../Firmware/main/main.c), [MIC.c](../Firmware/components/MIC/MIC.c), [MIC.h](../Firmware/components/MIC/MIC.h), and [PWM.c](../Firmware/components/PWM/PWM.c).

SilentSignal reads stereo I²S slots at 16 kHz and selects the channel with the larger peak for each frame. It retains the high 16 bits of each 32-bit slot. A full frame contains 256 samples per channel. Peak amplitude and mean absolute amplitude describe the envelope; mean removal, a Hann window, and a 256-point FFT provide spectral features. These are digital sample units, not calibrated sound-pressure levels.

The field named `f0_hz` is the frequency assigned to the largest FFT bin; it is not a robust fundamental-frequency estimator. `tone_x100` is the energy in that bin and its immediate neighbours divided by total positive-frequency energy. `crest_x10` uses peak divided by **mean absolute amplitude**, not peak divided by RMS. The low-frequency energy feature is calculated but is not used by the current classifier.

| Decision | Implemented criteria | Consequence for physical testing |
| --- | --- | --- |
| Tonal frame | Peak ≥ max(3 × noise, 450), capped at 20,000; concentration ≥ 70%; peak/mean-absolute ratio > 0 and ≤ 4.0; reported peak-bin frequency 350–2,800 Hz | Test stable bells as well as short, changing-pitch, harmonic-rich and out-of-band bells |
| Doorbell event | At least 8 qualifying frames in a 16-frame history; spread between qualifying peak-bin frequencies ≤ 500 Hz | Frames need not be consecutive. Music and repeated electronic beeps may also satisfy this rule |
| Knock event | Peak ≥ max(4 × noise, 700); mean absolute amplitude ≥ max(3 × noise, 120); both gates capped at 20,000; mean rises at least 2× from the preceding frame and subsequently falls below half; concentration < 82%; peak/mean-absolute ratio ≥ 2.2; reported peak-bin frequency 40–2,200 Hz | Impacts with a similar transient shape are potential false positives; soft knocks and smeared reverberant envelopes may be missed |
| Knock timing | Tests history frame 5 using preceding and following frames; 10-frame cooldown; inhibited while `bell_on` | Test rapid knock groups and overlaps with bells separately from isolated knocks |
| Background estimate | First 12 frames select a low nonzero mean; later update `(15 × noise + avg) / 16` only when `0 < avg < 2 × noise`; floor 30 after warmup | Slow drift can be tracked. A sudden loud background may be excluded from adaptation rather than learned |
| OTHER event | Unclassified hot burst lasts at least 8 frames, exceeds the stronger peak gate, then accumulates 5 quiet frames | OTHER is not a complete environmental-sound classifier; brief non-target sounds may produce no new display event |

The exact FFT spacing is 16,000 / 256 = **62.5 Hz**. However, `MIC_SAMPLE_RATE / MIC_FFT_N` uses integer division in the frequency assignment, yielding **62 × bin index**. Frequency labels and threshold comparisons therefore differ slightly from the theoretical bin centres. This is a source-review finding; the supplied firmware has not been edited.

## Repeatable test method

1. Record the firmware hash/revision, board revision and any rework, supply, microphone orientation and height, door material, room, source type, and selected mode. Confirm that the deployed firmware matches the documented source before interpreting source-based predictions.
2. Start with independent acoustic trials in NORMAL mode, with alerts off and the source stopped. Power up in the intended background and allow at least 2 seconds of settling for the protocol. The code's nominal warmup is only 192 ms; the longer wait is a proposed control, not a measured requirement.
3. Mark proposed microphone distances of 0.5, 1, 2 and 3 m from the knock point or doorbell sound source. Use light, ordinary and firm knocks with the same operator and location. Record these as qualitative intensity categories unless a calibrated meter is available. Do not label them with invented dB values.
4. Begin with 10 independent trials per source–distance–intensity condition as an exploratory plan. Randomize the order and retain every attempt, including failures. Repeat selected conditions in another session and room. These counts are a proposed starting point, not statistical validation or completed experiments.
5. Define one isolated knock or one complete doorbell activation as an event. Treat a multi-knock sequence as one separately labelled episode; do not count every physical impact as an independent trial while the alert remains latched. Use a fixed proposed response window from source onset to 1 second after source offset. Record source duration and actual response window, and keep them consistent within each scenario.
6. Record the first new OLED classification and any later label transitions, actual LED colour, OLED alert state and actual vibration separately. A stale label is not a new detection. Use synchronized video or timestamps for latency; otherwise leave latency blank. The current firmware does not log raw classifier decisions, so its OLED is a mode-filtered, persistent observation.
7. Stop the source, acknowledge the alert, and allow quiet before the next independent trial. Require a fresh alert transition. If needed, reset between isolated trials and record that choice. Test acknowledgment during an ongoing sound separately; it deliberately changes rearming behaviour.
8. Run background-only sessions without injected targets, for example 10 minutes each for quiet, conversation and footsteps. Record the actual exposure duration and count distinct new alert activations. Acknowledge and rearm consistently, logging acknowledgment and unavailable time; a motor that stays latched is not many false alarms.
9. Evaluate mode filtering and output operation in dedicated runs. A target intentionally excluded by the selected mode is not an acoustic miss. Report real sources and loudspeaker playback separately; speaker response, volume and position change both the envelope and spectrum.

## Acoustic scenario matrix

**Status for every row: proposed structured test; no condition-specific measured result available.** “Desired” describes the application objective; “expected” describes the current rules and likely failure modes, not a guaranteed outcome.

| ID | Scenario and controlled variation | Desired result and firmware-based expectation | Record / engineering purpose |
| --- | --- | --- | --- |
| A01 | Ordinary single knock in a quiet room at 0.5 and 1 m | Desired: KNOCK and alert. A sharp rise and decay above the adaptive gates should qualify | New label, misses, wrong labels, output states and latency if measurable; establish a repeatable reference |
| A02 | Light / ordinary / firm knocks at 0.5, 1, 2 and 3 m; same door and orientation | Desired: recognize audible target events. Weak distant knocks may fall below peak or mean gates; very strong inputs may distort features if clipping occurs | Report each cell separately; do not claim a maximum range from a single success |
| A03 | Same knock on different door materials and with the door open / closed | Desired: KNOCK. Panel resonance and room reflections can change peak-bin frequency and decay | Door material, state and placement; separate acoustic transfer changes from distance |
| A04 | Two and three knocks at slow and fast spacing | Desired: at least one useful alert per episode. Closely spaced hits may be suppressed by the nominal 160 ms cooldown or alter the required envelope | Episode-level detection and duplicate alerts after acknowledgment; no per-impact accuracy claim |
| A05 | Sustained electronic doorbell tone, quiet room, multiple distances | Desired: DOORBELL. Most favourable when sufficient frames are concentrated and within the accepted band | Bell identity, duration, real source versus playback, frequency evidence if available |
| A06 | Two-tone ding-dong, melodic chime, mechanical bell and short electronic pulse | Desired: DOORBELL for each intended doorbell. Wide pitch changes, broad spectra or insufficient duration can fail the 8-of-16 rule | Per-bell results; NONE/OTHER versus mistaken KNOCK; never pool all bell types into one unexplained result |
| A07 | Knock and bell with steady fan / ventilation noise | Desired: detect targets without background alerts. Raised adaptive gates can suppress weaker targets | Background source, settling procedure, near/far comparison; quiet reference repeated at end |
| A08 | Knock and bell during ordinary and animated conversation | Desired: correct target label. Speech may mask a knock, disrupt tonal concentration or briefly resemble a tone | Target present/absent intervals, speaker position and overlap timing |
| A09 | Quiet room, no intentional event | Desired: no new target alert. The OLED may retain an earlier label after acknowledgment | Fresh activations per recorded exposure time, not the number of frames showing a label |
| A10 | Conversation only, including sustained vowels and laughter | Desired: no knock/bell alert. Sustained voiced energy can sometimes meet bell features; abrupt vocal bursts can resemble impacts | False-alert label and context; distinguish OTHER indication from a target false positive |
| A11 | Footsteps only, on different floors and near the support surface | Desired: no target alert. Individual steps can have an impulsive rise and decay similar to knocks | Distance, floor type, support coupling and false alerts; airborne versus mechanical coupling |
| A12 | Keys dropped, table taps, chair movement, cupboard closure | Desired: reject non-door impacts. The algorithm has no evidence of the impact's physical source | Record confused events individually and retain recordings where permission allows |
| A13 | Television / music / phone tones without targets | Desired: no target alert. Stable musical notes or beeps in the accepted band can satisfy the bell rule | Excerpt identity and playback setup; do not call a playback success a real-doorbell validation |
| A14 | Abrupt transition from quiet to loud background, then back to quiet | Desired: recover useful sensitivity. Updates are gated, so a loud step may not be incorporated into the noise estimate | Recovery timeline, false alerts and misses before/after the transition; instrument noise estimate in a future test build |
| A15 | Knock during a bell; speech overlapping a bell | Desired: useful notification of a target; separate simultaneous labels are not implemented | Bell state suppresses knock evaluation; document precedence and loss of distinct events |
| A16 | Repeat target trials with motor freely suspended, against the enclosure, and on the same surface as the microphone | Desired: motor does not cause fresh target detections or prevent rearming | Feedback-related labels and recurrence after confirmation; compare otherwise identical placement |

## Notification and control test matrix

Physical checks remain pending unless explicitly described in the evidence table. Expected LED colours below are the **firmware's intended mapping**. The supplied schematic appears to use a common-anode RGB connection while `rgb_apply()` drives a selected channel high. Photographs show a wired LED arrangement; reconcile actual wiring and polarity before using colour mismatch to diagnose recognition.

| ID | Action / starting condition | Expected current firmware behaviour | Physical acceptance check |
| --- | --- | --- | --- |
| U01 | Power up without a target | NORMAL, SOUND NONE, ALERT OFF; RGB off command; motor zero duty | Readable OLED; no unintended vibration; verify physical RGB off despite polarity concern |
| U02 | Recognized knock in NORMAL, starting alert-off | SOUND KNOCK, ALERT ON; red command; motor duty 256 | Confirm red illumination and tactile vibration, not merely a software command; photograph supports the displayed knock/red state only |
| U03 | Recognized bell in NORMAL, starting alert-off | SOUND DOORBELL, ALERT ON; green command; same motor duty 256 | Confirm green illumination and tactile vibration; the motor does not encode sound type with a different pattern |
| U04 | Qualifying OTHER event, starting alert-off | SOUND OTHER; blue command for nominally 125 frames; no motor alert | Check blue indication then off; the sound text can persist after blue clears |
| U05 | Stop the source after a target alert | Alert, motor command and target colour remain latched | Verify alert remains until confirmation; silence alone must not be counted as confirmation |
| U06 | Briefly press and release CONFIRM during alert | Clears alert and RGB, requests acknowledgment processing; motor command goes to zero | Vibration stops, ALERT OFF appears; last SOUND label can remain, by design |
| U07 | Confirm while bell or burst is still active, then stop it and present a fresh event | Ongoing-event blocking is requested when burst/bell state is active; block clears on burst reset after quiet | Check no immediate recurrence and that a later event rearms. Quiet/burst coupling makes this an edge-case test, not a guaranteed timing claim |
| U08 | Cycle MODE with short presses | NORMAL → KNOCK → DOORBELL → NORMAL | Exactly one change per accepted release; inspect OLED and actual GPIO/button mapping |
| U09 | KNOCK mode: present knock, then bell as separate reset trials | Knock can alert; recognized bell is mapped to OTHER, with blue only if no alert is already active | Excluded bell must not start vibration. Do not score this intentional exclusion as a recognition miss |
| U10 | DOORBELL mode: present bell, then knock as separate reset trials | Bell can alert; recognized knock is mapped to OTHER | Excluded knock must not start vibration; test both directions independently |
| U11 | Change mode while an alert is already latched | Mode changes without clearing the existing alert | Motor may continue from the previous event. Confirm separately; mode switching is not acknowledgment |
| U12 | OTHER after an unacknowledged target; second target while alert active | OTHER can replace SOUND text while target colour/alert remain; a new allowed target can change colour | Check whether text and colour remain understandable; identify this as state-history behaviour, not automatically a classifier fault |
| U13 | Very short tap, ordinary short press, ≥1 s hold, switch bounce | Approximately 3 polls debounce; short-release acceptance requires hold 3–79 ticks at nominal 10 ms polling; long hold is ignored | Use ordinary 100–300 ms presses as a proposed baseline; verify missed/double actions and long-hold rejection |
| U14 | Microphone read failure on a controlled bench; OLED disconnected on a controlled bench | Read error sets MIC READ FAIL for UI; OLED bit-banged ACK is not read/validated, so software success does not prove display presence | Do not disconnect powered wiring casually; verify fault indication and recovery using a controlled fixture. No robust OLED fault reporting is implemented |

## Engineering interpretation

### What worked well

The builder's experience supports an encouraging first prototype. The photographs independently support the presence of an assembled system, legible target labels and distinct red/green visible alert states. They do not establish a measured false-positive rate, tactile effectiveness or response time.

The implementation also has useful engineering properties. Processing and classification run locally without cloud inference or a smartphone. Time-domain shape and spectral concentration provide complementary evidence: a brief impact differs from a sustained tonal event. Separate FreeRTOS tasks keep display work apart from microphone processing, and alerts latch so a short event can remain noticeable. Mode filtering and acknowledgment provide explicit user control. These are verified implementation features, not experimentally measured usability benefits.

### Factors expected to affect recognition

**Distance and placement.** Lower received amplitude can fail the peak or mean gate even when the spectral shape remains correct. Door transmission, microphone angle, furniture, reflective walls and mounting alter the received waveform. A monotonic distance–performance curve cannot be assumed in a room. The on-screen level bar is scaled against 8,192 sample units and saturates; it is not a calibrated sound-level meter or a clipping detector.

**Background adaptation.** The noise estimate reduces sensitivity to modest changes in background, but adaptation is conditional. An abrupt increase greater than roughly twice the current estimate is not absorbed by the update. Conversely, a gradually rising background can increase thresholds enough to hide a quiet target. The estimate uses mean absolute amplitude while several gates compare peak amplitude, making the chosen multipliers empirical design parameters. A background described merely as “noisy” is insufficient to reproduce a result.

**Sound diversity.** A real doorbell may have harmonics, changing notes or a short attack followed by decay. A dominant harmonic can become the largest FFT bin, and changing dominant harmonics can violate the 500 Hz spread even when the perceived sound is one bell. Knocks vary with door construction and hand contact. Strong reverberation can keep the envelope from falling below half within the tested history interval.

**Acquisition and numerical effects.** Selecting the larger-peak channel can select an unexpected noisy slot if the inactive I²S channel is not well behaved. Partial frames skip FFT extraction but still return a usable frame when samples exist, changing features and breaking assumptions about fixed frame timing. The integer frequency calculation biases reported bin frequencies; a corrected calculation should be tested on the same recorded inputs before changing classification thresholds.

### Why false positives are plausible

A dropped object, footstep or table tap can satisfy the same peak, rise, decay and crest conditions as a door knock. The classifier measures acoustic shape; it does not know which object was struck. A resonant impact may also retain a tonal tail. Likewise, a phone beep, music note or sustained voiced speech can concentrate energy in a few FFT bins for enough of the 16-frame history to resemble a bell. These are engineering explanations derived from the rules, not claims that these false positives were observed in the builder's trials.

Mechanical coupling adds another path: motor vibration and its 1 kHz PWM drive can create acoustic or structural energy near the microphone. Depending on mounting and harmonics, this could interfere with thresholds, tonal classification or rearming. Motor-on and motor-off comparisons are required to establish causality. Do not assume every alert during vibration is feedback: the original sound may still be present.

### Engineering trade-offs

| Choice | Benefit | Cost / testable consequence |
| --- | --- | --- |
| Rule-based DSP | Inspectable decisions, small feature set, no training pipeline | Thresholds encode a limited family of sounds; broader variability needs measured tuning |
| 256-sample frames | Short acquisition windows and modest buffers | 62.5 Hz spectral bins; impacts are sensitive to frame alignment |
| Multi-frame bell rule | Rejects isolated spectral peaks | Adds evidence accumulation; may miss short or rapidly changing chimes |
| Knock cooldown | Reduces repeated detections of one impact | Can merge rapid repeated knocks; episode-level scoring is necessary |
| Higher amplitude gates | Reject more low-level interference | Lose soft or distant targets; a threshold change must report both misses and false alerts |
| Latched vibration | Short events remain actionable | Continuous energy use and possible microphone feedback until confirmation |
| Single vibration duty | Simple control | No tactile distinction between knock and bell; 25% PWM command does not prove reliable motor starting |
| Software I²C OLED | Flexible pin assignment, simple local driver | GPIO timing consumes CPU; absent ACK validation hides bus failures |
| Local-only operation | Works without network availability and does not stream household audio | Notification reach depends on where the user and device are located |

### Hardware questions that affect interpretation

The assembled photographs show wire rework and components whose arrangement differs from the schematic representation. Treat the functioning prototype and the documented PCB as related evidence that must be reconciled, not interchangeable validation.

- **LED polarity:** common-anode schematic versus active-high selection in code; verify actual LED pinout and any rework.
- **Motor supply:** the schematic shows R2 = 1 kΩ in series with the motor connector's supply. At a nominal 4 V, the ideal upper bound through that resistor is 4 V / 1,000 Ω = 4 mA before motor and transistor drops. This is a calculated schematic implication, not measured motor current. Inspect whether the assembled board replaces or bypasses it; obtain motor rated, startup and stall current.
- **Regulator headroom:** the rail is labelled 4V before an AMS1117 and 3V3 after it. A net name is not a measurement. Verify actual diode type, loaded input, output voltage, dropout and temperature using the fitted regulator's datasheet. No current budget or battery runtime can be established from the available files.
- **BOM ambiguity:** diode comments say M7, library references say SS24, and the description calls them Schottky. Resolve fitted part identity. The BOM lacks the external motor's part number and electrical ratings.
- **Visual provenance:** the original pre-existing schematic export names a KiCad working file in its title block; the private upload copy has that file-path footer removed. It is a supplied reference export of an Altium-origin project, not newly generated or certified against the original design during this review. No design conversion was performed here.

## Prioritized improvements and how to evaluate them

| Priority | Proposed improvement | Evidence required before claiming a benefit |
| --- | --- | --- |
| 1 | Reconcile assembled hardware, LED polarity, motor path and source revision; record rework | GPIO-to-output bench checks, current/rail measurements, revision photographs |
| 1 | Add optional timestamped feature and raw-label logging, distinct event IDs and mode/ack records | Compare logged decisions with synchronized source/video annotations; keep logging overhead measured |
| 1 | Collect a labelled corpus of real knocks, diverse bells and hard negatives across rooms and distances | Separate tuning sessions from held-out evaluation; split by source/room/session so replayed clips do not leak into both sets |
| 2 | Correct frequency-bin arithmetic and handle partial frames explicitly | Before/after replay on identical clips, edge cases near frequency gates, runtime measurement |
| 2 | Improve noise estimation with robust background tracking and separate onset/offset thresholds | Quiet-to-noisy transitions; show changes in both target recall and false alerts per hour |
| 2 | Add temporal repetition, spectral flux or band-energy features; use the currently unused low-band feature only if useful | Evaluate hard negatives such as footsteps and object drops; avoid tuning only to successful examples |
| 2 | Isolate motor mechanically and assess electrical decoupling; consider bounded vibration patterns | Compare same source and placement with/without motor, rearm tests and current traces; account for any listening blind time |
| 2 | Separate “last detected sound” from “active alert” in the UI and define mode-change semantics | User checks for mixed histories, new events during alerts and acknowledgment during continuous sounds |
| 3 | Evaluate a TinyML classifier against the rule-based baseline | Diverse held-out data, per-class precision/recall, false alerts per hour, RAM/flash use and end-to-end latency; no trained model currently exists in the supplied files |
| 3 | Add enclosure and power optimization | Acoustic-port response, tactile coupling, thermal behaviour, measured idle/alert power and startup reliability |
| 3 | Consider wireless remote notifications | Delivery reliability, local fallback and power impact; wireless notifications are not present functionality |

## Recording and reporting results

Use the empty [trial template](../Results/test_results_template.csv). One row is one independent target or discrete non-target episode, or one timed background-only session. Keep control tests in a separate copy with `trial_type=control`. The [analysis script](../Results/analyze_results.py) excludes control rows from recognition metrics and separates NORMAL, KNOCK and DOORBELL modes.

For NORMAL-mode acoustic trials, use ground-truth classes `knock`, `doorbell`, `other`, `none`. Record a new displayed classification as `knock`, `doorbell`, `other` or `none`; use `none` when no new event appears, even if an old label remains. Record physical `alert_observed` independently. A correct OLED label with no vibration is an output failure, not automatically a classifier miss. Raw classifier accuracy in filtered modes cannot be inferred from the OLED alone.

The script reports a four-class NORMAL-mode confusion matrix, exact-label agreement, per-target precision and recall, wrong-target classifications, no-target-label misses, and physical alert misses for mode-eligible targets. Class-label misses include wrong-target classifications when computing recall. It reports unexpected physical alerts for noneligible event trials and false alert activations per background hour separately. Excluded-mode targets are evaluated against mode policy, not counted as acoustic misses. `other` and `none` remain distinct display labels; a true non-target that produces neither label is a rejection even if four-class exact agreement is lower.

**Definitions:** precision = TP / (TP + FP); recall = TP / (TP + FN); exact-label agreement = matching labels / scored NORMAL acoustic trials; background false alerts/hour = distinct fresh target-alert activations / observed background hours. Always show denominators. Undefined ratios remain unavailable, not zero. Background sessions do not enter the per-event confusion matrix. A zero-alert short session is only that observation, not proof of zero false-positive risk.

No numerical results are available yet. Publish the completed rows, source conditions, repeats, excluded/incomplete records and relevant uncertainty alongside any future summary. Avoid a single “accuracy” value that hides differences between quiet-room knocks, melodic bells and noisy backgrounds.
