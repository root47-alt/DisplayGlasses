#pragma once
// Turning the glasses off and on. "Off" is the ESP32's deep sleep: the
// screen and Bluetooth are off and the board draws very little power. A
// press of the button wakes the chip, which then starts up from the
// beginning, as if it had just been plugged in. The battery still charges
// over USB while the glasses are off.

namespace power {

// True if a button on this pin can wake the chip. Only some pins can; the
// Feathers' own buttons can.
bool canWakeFrom(int pin);

// Call at the start of setup(), before reading the button: after waking up,
// it hands the button's pin back from the wake-up circuit.
void afterWake(int pin);

// Sleeps until the button on `pin` is pressed. Never returns.
[[noreturn]] void sleepUntilPressed(int pin);

}  // namespace power
