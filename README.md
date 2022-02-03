# thump
An automatic ceiling thumper for noisy neighbors using a piezo vibration sensor and pneumatics.

Because nobody has time for broom handles.

## Prototype
<img src="https://user-images.githubusercontent.com/25882507/152401381-dd906b98-2e48-43e9-90b0-22687aab323a.JPG" height="600" />

## Features
- Runs on Arduino microcontrollers
- Monitors neighbors' stomping and triggers a thump based on a configurable stomp threshold
- Accepts IR remote signal to change thump threshold, manually trigger thumps, or arm/disarm
- Uses standard 3000psi air tanks
- **Full-auto thumping**

## Schematic / Components
(schematic and component list to be added)
- Accepts an analog output voltage (PWM from 0V to 5V) from a microcontroller, combined with a low-pass filter for smoothing, to set a reference voltage that acts as a threshold for activating the thump
- Detects ceiling vibration using a piezoelectric sensor, which converts vibration to a high-voltage, low current signal.  Normalizes this voltage then amplifies the signal using one comparator in the LM358 op-amp
- Uses the second comparator in the op-amp to compare the vibration signal voltage to the threshold voltage from the Arduino, activating a transistor that powers a pneumatic solenoid when a thump is required

![Breadboard](https://user-images.githubusercontent.com/25882507/152405217-5c3ae188-3fd7-4c21-a24b-1aaaba5375bf.png)

## Video Example
https://user-images.githubusercontent.com/25882507/152405606-cf46f67b-545c-4870-9b66-d56ac1f80335.MOV
