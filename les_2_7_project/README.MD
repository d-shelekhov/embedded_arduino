# Traffic Light with Countdown Display: How It Works

The program does two things at the same time: the traffic light changes phases, and a one-digit seven-segment display shows the seconds left until the phase ends. A hardware timer does all the counting, and `loop()` only displays the result.

## Overview

The timer calls `countDownPhaseTime()` every 1 ms. That function decrements two counters:

- `timeLeftMs` is the time left until the next action: the end of a phase, or one blink step.
- `phaseLeftMs` is the time left until the color changes. The display reads this one.

When `timeLeftMs` reaches zero, `handlePhaseEnd()` is called. It decides what happens next and turns on the new phase through `startPhase()`.

Phase sequence:

```
GREEN -> GREEN_BLINK -> YELLOW -> RED -> RED_YELLOW -> GREEN
```

## Function by function

### `setup()`

Runs once.

- Configures all pins as outputs (three traffic-light LEDs and seven segments) and blanks the display.
- Creates the timer: a prescaler of 80 gives 1 MHz, so one tick is 1 µs.
- Attaches the `countDownPhaseTime` interrupt.
- Calls `startPhase(GREEN)`, so the initial state is set before the timer starts.
- `timerAlarmWrite(..., 1000, true)` and `timerAlarmEnable` make the timer fire periodically every 1000 µs.

### `countDownPhaseTime()`

Called from the interrupt once per millisecond.

- Decrements `phaseLeftMs` and `timeLeftMs`, but never below zero, to avoid underflow.
- If `timeLeftMs` has just reached zero, calls `handlePhaseEnd()`.
- Deliberately short and does not print to Serial, because that is not allowed inside an interrupt.

### `handlePhaseEnd()`

Answers "what comes next".

- Follows the sequence: green, green blink, yellow, red, red+yellow, then green again.
- During blinking it counts steps: 6 steps of 300 ms (3 blinks, two steps each).
- While steps remain, it calls `toggleBlink()`. When they run out, it switches to yellow.

### `startPhase(State state)`

Begins a new phase.

- Stores the state in `currentState` and raises the `hasStateChanged` flag (for Serial output).
- Turns on the right LEDs via `setLights()`.
- Sets both counters.
- For green, `timeLeftMs` is 4000 ms and `phaseLeftMs` is `GREEN_TOTAL_MS` (5800 ms: steady green plus blinking), so the countdown does not break during blinking.
- For the other phases, both counters equal the phase duration.

### `setLights(red, yellow, green)`

Turns the three LEDs on or off at once. It is just three `digitalWrite` calls.

### `toggleBlink(pin)`

Flips the LED on the given pin and resets `timeLeftMs` to the blink half-period (300 ms). This is how blinking is built from repeated steps.

### `showDisplayDigit(digit)`

Shows a digit on the display.

- Takes the digit's mask from the table. Each bit is one segment: bit 0 is `a`, bit 6 is `g`.
- Loops over the seven segments and decides for each whether it should light.
- A lit segment gets `LOW` on its pin, an unlit one gets `HIGH`, because the anodes are wired to 3V3.
- A value outside 0-9 turns all segments off.

### `getStateName(state)`

Returns the state name as text for `Serial`. It is only used for debugging.

### `loop()`

Runs forever and calculates nothing, it only displays.

1. Inside a critical section (`portENTER_CRITICAL`), copies the data shared with the interrupt: the state-changed flag, the state, and `phaseLeftMs`. This way the interrupt cannot change them in the middle of the read.
2. Converts `phaseLeftMs` to seconds, rounding up: `(leftMs + 999) / 1000`. So 1000 ms shows as 1, and 1001 ms shows as 2. If time has run out, the digit is `-1` (blank the display).
3. Redraws the display only if the digit has changed.
4. If the state has changed, prints it to `Serial`. Serial output lives here, not in the interrupt.

## Example: the red phase

The interrupt fires every millisecond. `startPhase(RED)` turns on the red LED and sets both counters to 5000. With each tick the counters go down, and `loop()` sees `phaseLeftMs`:

| `phaseLeftMs` | Digit shown |
|---|---|
| 5000..4001 | 5 |
| 4000..3001 | 4 |
| 3000..2001 | 3 |
| 2000..1001 | 2 |
| 1000..1 | 1 |

Exactly 5 seconds later `timeLeftMs` reaches 0. The interrupt calls `handlePhaseEnd()`, which calls `startPhase(RED_YELLOW)`; red and yellow light up and the counters become 1500. On its next pass, `loop()` sees the digit 2.

## Digits shown during one full cycle

| Phase | Duration | Digits |
|---|---|---|
| Green + blink | 5.8 s | 6 (0.8 s), 5, 4, 3, 2, 1 |
| Yellow | 2 s | 2, 1 |
| Red | 5 s | 5, 4, 3, 2, 1 |
| Red + yellow | 1.5 s | 2 (1 s), 1 (0.5 s) |

## Notes

- The logic was checked on a computer with a simulated 1 ms timer. The code has not been run on the board.
- GPIO 4-10 are safe on ESP32-S3. On a classic ESP32, GPIO 6-11 are used by flash and cannot be used for segments.