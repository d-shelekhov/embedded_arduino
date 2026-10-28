# Traffic Light with Countdown Display (ESP32)

A traffic light that follows the Ukrainian traffic rules (ПДР України), driven by a hardware timer, with a one-digit seven-segment countdown display built from separate LEDs and resistors.

## Features

- Phase cycle by the rules: green → green blink → yellow → red → red+yellow → green.
- Special mode: blinking yellow (unregulated intersection), enabled with one constant.
- All timing comes from a hardware timer interrupt (1 ms tick), with no `delay()`.
- The countdown display shows the seconds left until the color changes.
- The interrupt handler only counts down and sets a flag. All the real work runs in `loop()`.

## Hardware

| Part | Quantity | Notes |
|---|---|---|
| ESP32-S3 board | 1 | e.g. ESP32-S3-DevKitC-1 (onboard RGB LED on GPIO 48) |
| Red, yellow, green LEDs | 1 each | Traffic light |
| Resistors for the traffic-light LEDs | 3 | As in the lesson circuit |
| 3 mm LEDs for the display | 14 | 2 per segment |
| 330 Ω resistors for the display | 14 | 1 per display LED |
| Breadboard | 1 | At least 30 columns (400 points) for the display |
| Jumper wires | ~30 | |

### Pin map

| Function | GPIO |
|---|---|
| Red LED | 17 |
| Yellow LED | 15 |
| Green LED | 16 |
| Segment a | 4 |
| Segment b | 5 |
| Segment c | 6 |
| Segment d | 7 |
| Segment e | 8 |
| Segment f | 9 |
| Segment g | 10 |
| Display power | 3V3 |

### Display wiring

```
 aaa
f   b
f   b
 ggg
e   c
e   c
 ddd
```

- The display is common anode: all LED anodes (long legs) go to 3V3.
- Each LED cathode (short leg) goes through its own 330 Ω resistor to the segment's GPIO.
- The two LEDs of one segment are in parallel, each with its own resistor. Do not share one resistor between them.
- A segment is on when its pin is `LOW`.
- Current is about 3.3 mA per LED, about 6.6 mA per GPIO pin.

The breadboard layout is in `two_leds_per_segment_breadboard.svg`.

## Files

| File | What it is |
|---|---|
| `traffic_light_countdown_oop.ino` | **Main version.** Classes `TrafficLight` and `CountdownDisplay`; the ISR only sets a flag; register writes for the traffic-light LEDs |
| `traffic_light_countdown_flag_reg.ino` | Same logic without classes |
| `traffic_light_countdown_flag.ino` | Without classes, `digitalWrite` instead of register writes |
| `traffic_light_countdown_v2.ino` | Earlier version: phase logic runs inside the ISR |
| `*_marked.ino` | Copies with `[NEW]` / `[CHANGED]` / `[REMOVED]` markers against the previous version |
| `one_digit_test.ino` | Display test: each segment in turn, then digits 0–9 |
| `two_leds_per_segment_breadboard.svg` | Breadboard layout for the display |
| `traffic_light_countdown_explained.md` | Function-by-function explanation |

## Build and upload

1. Install the Arduino IDE and the **arduino-esp32 2.x** board package. The timer calls (`timerBegin(0, 80, true)`, `timerAlarmWrite`, `timerAlarmEnable`) use the 2.x API; core 3.x changed it.
2. Select your ESP32-S3 board and port.
3. Open `traffic_light_countdown_oop.ino` and upload.
4. Open the Serial Monitor at 115200 baud to see phase changes.

If the compiler cannot find `soc/gpio_reg.h`, delete that `#include` line. `Arduino.h` usually includes it already.

## Settings

All settings are constants at the top of the sketch.

| Constant | Default | Meaning |
|---|---|---|
| `UNREGULATED_MODE` | `false` | `true` = start in blinking-yellow mode |
| `DURATION_GREEN_MS` | 4000 | Steady green |
| `DURATION_YELLOW_MS` | 2000 | Yellow |
| `DURATION_RED_MS` | 5000 | Red |
| `DURATION_RED_YELLOW_MS` | 1500 | Red+yellow |
| `BLINK_HALF_PERIOD_MS` | 300 | Half-period of blinking |
| `GREEN_BLINK_COUNT` | 3 | Number of green blinks |

The display shows one digit, so each phase must stay at 9 seconds or less. For green this means steady green plus blinking.

## What the display shows

| Phase | Duration | Digits |
|---|---|---|
| Green + blink | 5.8 s | 6, 5, 4, 3, 2, 1 |
| Yellow | 2 s | 2, 1 |
| Red | 5 s | 5, 4, 3, 2, 1 |
| Red + yellow | 1.5 s | off |
| Blinking yellow | — | off |

Seconds are rounded up: 1..1000 ms shows 1, 1001..2000 ms shows 2. The first green digit lasts 0.8 s because the total green time is 5.8 s. Set `DURATION_GREEN_MS = 4200` to make every digit a full second.

## How it works

```
Hardware timer (1 ms)
    │
    ▼
onTimer() ──► trafficLight.tick()        [ISR, IRAM_ATTR]
                │ decrement counters
                │ set isPhaseEnded flag
                ▼
loop() ──► trafficLight.update()
             │ flag set? -> handlePhaseEnd() -> startPhase()
             │                                   └─► traffic-light LEDs
             ▼
          trafficLight.getDisplayLeftMs()
             ▼
          countdownDisplay.showDigit(seconds) ──► display segments
```

How the code follows the lesson's interrupt rules:

- **The ISR is minimal.** It only decrements counters and sets a flag.
- **Shared variables are `volatile`.** Only the three fields shared with the ISR are; `loop()` reads them under a spinlock (`portENTER_CRITICAL` / `portENTER_CRITICAL_ISR`).
- **No `delay()`** anywhere in the sketch.
- **`IRAM_ATTR`** is on every function the ISR calls: `onTimer()` and `tick()`.
- **No `Serial` or library calls in the ISR.** Serial output and pin writes happen in `loop()`.

## Testing

1. Upload `one_digit_test.ino` first. Every LED must light when its segment is on, and no LED may light with a different segment. Digits 0 and 8 must look different.
2. Then upload the traffic light and compare the digits with the table above.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Display stays dark | Check 3V3 to the anode rails. Some breadboards have rails split in the middle; use the link wire. |
| Board keeps rebooting | The board is a classic ESP32, not an S3. GPIO 6–11 are used by flash there; move the segments to other pins. |
| One LED of a segment lights with a different segment | Its resistor sits on the wrong breadboard strip. Example: B1 lighting only when segment a is on means B1's resistor is on segment a's node instead of segment b's. |
| One LED never lights | The LED is reversed (long leg must go to 3V3) or its resistor is not in the LED's cathode strip. |
| Compile errors in `timerBegin` / `timerAlarmWrite` | The board package is arduino-esp32 3.x. Install 2.x or port the timer calls to the 3.x API. |

## Status

The logic was checked on a computer with a simulated 1 ms timer. It has not yet been verified as a full ESP32 build on real hardware.