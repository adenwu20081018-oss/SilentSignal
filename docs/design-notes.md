# SilentSignal: design notes

Detailed reference for the hardware, wiring, calculations and firmware. The [README](../README.md) has the overview.

## Hardware and wiring

Full parts list: the [BOM](../hardware/bom/bomfile.xls) and [readable BOM extract](../hardware/bom/BOM_readable.md).

| Component | Role and supported details | Advantages and trade-offs |
| --- | --- | --- |
| ESP32-S3-WROOM-1 N16R8, U3 | Local acquisition, FFT, classification and UI | Separate tasks/cores support concurrent audio and interface work; radio capability is unused in this application |
| INMP441, U2 | Digital acoustic input; grounded L/R in schematic | Digital acquisition avoids an external analog ADC path; mounting and channel handling still influence signal quality |
| LCD12864 IIC, L1 | 128 × 64 OLED interface; BOM names SSD1306/SSD1315 | Direct mode/event feedback; small display and software-driven bus limit presentation and diagnostics |
| RGB-3528, LED1 | Three colour channels, 330 Ω series resistors shown | Distinct visual categories; actual LED polarity and photographed rework must match firmware |
| CH340K, U4 | USB-to-UART programming/debug path, separate from native USB | Offers serial access; adds components and another USB power path |
| AMS1117, U1 | Regulated rail labelled 3V3 | Simple linear supply architecture; dropout and heat depend on actual part, input voltage and load |
| S8050, Q1; JP1 | NPN motor switch and two-pin external motor connection | GPIO controls a separate load path; requires verified base drive, motor current, protection and supply headroom |
| S8050, Q2/Q3 | Boot/reset interface switching | Supports programming control; verify against fitted hardware and serial adapter behaviour |
| SW-PB4 ×3 | Reset, MODE and CONFIRM | Direct interaction; debounce and short-press interpretation affect operation |
| R8–R10: 330 Ω | RGB current limiting | Limits channel current; actual brightness depends on LED forward voltage and polarity |
| 1 kΩ / 10 kΩ / 0 Ω resistors | Indicators, base networks, pull-ups and links | Component values matter: motor-series R2 = 1 kΩ is an unresolved concern |
| C1/C3/C5/C6: 104; C4: 10 nF; E1–E4: 220 µF/16 V | Local and bulk capacitance | Supports supply decoupling; fitted capacitor chemistry, polarity and regulator stability require verification |
| D1/D3/D7 | Supply isolation and motor flyback path | BOM M7 / SS24 naming conflict prevents a confident part-specific voltage-drop calculation |
| External vibration motor | Tactile notification | Visible in prototype photos; absent exact part number, startup/stall current and rated voltage |

Manufacturer references: [ESP32-S3 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf), [Espressif hardware checklist](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html), and [TDK INMP441 product information](https://product.tdk.com/en/search/sw_piezo/mic/mems-mic/info?part_no=INMP441). Confirm exact fitted regulator, diode, OLED and motor variants before applying part-specific ratings.

### GPIO and peripheral connections

| Function | ESP32 connection | Hardware / review note |
| --- | --- | --- |
| Microphone WS | GPIO15 | I²S word select |
| Microphone SCK | GPIO16 | I²S bit clock; no MCLK output |
| Microphone SD | GPIO17 | I²S input |
| Microphone supply / L/R | 3V3, GND; L/R to GND in schematic | Left channel selected physically; firmware reads both slots |
| OLED SCL / SDA | GPIO1 / GPIO2 | Actual macros and schematic agree; an old header comment says GPIO19/20 and is stale |
| OLED supply | 3V3 / GND shown | Driver emits address byte 0x78, equivalent to 7-bit 0x3C |
| RGB red / green / blue command | GPIO13 / GPIO12 / GPIO11 | Active-high firmware conflicts with common-anode schematic; verify assembled LED |
| Status LED | GPIO10 | Toggles every 50 control ticks; nominal 0.5 s between transitions |
| CONFIRM / MODE | GPIO5 / GPIO6 | Pull-ups enabled; buttons connect input to ground |
| Motor PWM | GPIO7 | Q1 base via R6; motor at JP1; R2 series supply path needs review |
| Native USB D− / D+ | GPIO19 / GPIO20 | Shown through 0 Ω links; distinct from OLED pins |
| Serial programming | TXD0 / RXD0 nets to CH340K | USB bridge is a separate connector/path; verify board port before flashing |
| Boot / reset | GPIO0 / EN | Pull-ups and auto-programming transistors in schematic |
| Power / ground | USB VCC to diode-fed rail, regulator to 3V3; shared GND | Do not infer measured 4.0 V from the rail name; R5 is shown in the USB ground return |

Native USB GPIO19/20 assignments are also documented by [Espressif](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html). Do not follow the stale OLED comment when wiring the board.

## Engineering calculations

Calculated from the firmware constants and schematic values. None of these are measured.

| Quantity | Equation and substitution | Result and significance |
| --- | --- | --- |
| Sample interval | T = 1 / fs = 1 / 16,000 s | 62.5 µs between samples per channel |
| Nyquist frequency | fN = fs / 2 = 16,000 / 2 | 8 kHz theoretical limit; not a verified microphone system bandwidth |
| Frame acquisition | Tf = N / fs = 256 / 16,000 s | 16 ms nominal audio duration; **not measured CPU processing time** |
| FFT spacing | Δf = fs / N = 16,000 / 256 | 62.5 Hz; reported source frequency currently uses integer 62 × bin |
| Bell history | 16 × Tf = 16 × 16 ms | 256 ms sliding history; 8 qualifying frames represent 128 ms of evidence and need not be consecutive |
| Startup warmup | 12 × 16 ms | 192 ms nominal; startup and partial reads affect wall time |
| Knock look-back | 5 × 16 ms | About 80 ms after the candidate frame; not full end-to-end latency |
| Knock cooldown | 10 × 16 ms | 160 ms nominal event suppression interval |
| OTHER colour hold | 125 × 16 ms | About 2 s of successful full-frame cadence; text is not automatically cleared |
| I²S payload rate | 16,000 × 2 slots × 32 bits | 1.024 Mbit/s, or 128,000 bytes/s before peripheral overhead |
| Motor duty | D = 256 / 2¹⁰ | 25% commanded duty; torque, speed and current do not scale exactly with this number |
| Main audio arrays | 3 × 256 × 4 + 512 × 4 + 2 × 256 × 2 bytes | 6,144 bytes for FFT arrays plus active read buffers; excludes history, DMA, stacks, heap and unused helper buffer |
| Requested task stacks | 4,096 + 3,072 + 4,096 bytes | 11,264 bytes requested by three task creations; not measured stack usage |
| RGB resistor current bound | I ≤ 3.3 V / 330 Ω | ≤10 mA ideal rail-based bound; actual current is (rail − LED Vf − driver drop)/R and must be measured |
| Motor-path current bound | I ≤ 4 V / 1,000 Ω, assuming nominal schematic rail and fitted R2 | ≤4 mA before other drops; motivates inspection of actual rework rather than a claim of sufficient drive |

OLED refresh is requested every 100 ms and controls every 10 ms, plus execution and scheduling time. Recognition latency also depends on signal onset, frame alignment, history, acquisition, classifier state and output response. No latency guarantee follows from adding nominal task delays alone.

Regulator loss would be P ≈ (Vin − 3.3 V) × Iload, with quiescent loss added for a more complete estimate. Actual Vin, load current and fitted regulator data are missing, so a numeric thermal result is not justified. Motor current, overall power and runtime likewise remain unmeasured.

## Firmware and signal processing

The source uses three FreeRTOS tasks. `mic_task` runs on core 1 at priority 5, `ui_task` on core 0 at priority 4, and `ctrl_task` on core 0 at priority 6. Shared display/alert state is protected by a critical-section lock. This separation reduces direct interference between display updates and audio processing, but real-time deadlines and stack margins have not been measured.

The microphone driver reads 256 stereo sample pairs, selects the higher-peak channel and extracts peak and mean absolute amplitude. For full frames it removes the mean, applies a Hann window and computes an in-place radix-2 FFT. Spectral concentration uses the strongest bin plus its neighbours; the stored `f0_hz` is a peak-bin estimate, not necessarily a fundamental frequency.

The exact tonal-frame rule in [main.c](../firmware/main/main.c) is:

```c
return frame->peak >= gate_of(noise, 3, 450) &&
       frame->tone_x100 >= 70 &&
       frame->crest_x10 > 0 &&
       frame->crest_x10 <= 40 &&
       frame->f0_hz >= 350 &&
       frame->f0_hz <= 2800;
```

Doorbell recognition requires at least eight tonal frames in a sixteen-frame history, with at most 500 Hz spread among qualifying peak-bin estimates. Knocks require a strong, abrupt amplitude rise followed by decay, sufficient crest ratio, limited tonal concentration and a peak-bin estimate inside the knock range. A cooldown reduces repeat triggers. When a bell is active, knock evaluation is inhibited.

The background estimate updates gradually only for frames below twice its current value. This is conditional amplitude adaptation, not general noise cancellation. A sudden loud background can remain outside the update gate. The [testing and engineering analysis](testing.md) details all thresholds, temporal behaviour, false-positive mechanisms and edge cases.

Alerts latch until confirmation. Mode filtering turns a recognized but excluded target into OTHER. OTHER may update sound text while an older target alert remains active. Acknowledgment stops the motor command and clears the colour, but does not clear the last sound label. Firmware state must therefore be considered when scoring physical tests.
