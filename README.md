# ESP32 Tiny Matter Dishwasher

This code creates a "working" dishwasher that can be connected to and controlled via the Matter Protocol. Powered by the ESP32, it supports most of the Matter Dishwasher devicetype with the mandatory OperationalState cluster as well as the optional OnOff and DishwasherMode clusters.

https://tomasmcguinness.com/2025/06/27/matter-building-a-tiny-dishwasher-with-an-esp32/

It has two push buttons and one rotary encoder.

The first push button acts as the on/off button. Push it once to turn the device on and off. At present, it just controls the display.
The second push button as a start/stop/pause/resume button.
The rotary encoder allows the selection of DishwasherMode (aka program)

You can turn the device on, choose a program and press start. The device will start a 30 second countdown and work through five `Phases`. Once it's finished, it will stop. Whilst running, the program can be paused and resumed.

This is a work in progress, so the implementation isn't perfect and might not fully align to the Matter specification (Dead Front behaviour for example - I don't ignore commands when the device is off)

## Why?

I'm really interested in the energy management aspect of the Matter protocol. There aren't any devices on the market to enable me to explore this protocol and besides, I'm not going to buy a new applicance for testing! Having this toy dishwasher will let me play around with how the energe management might work.

## Wiring

This code has been built for the XIAO ESP32-C6. Currently, the pinouts are hardcoded, so if you want to use a different ESP Board, you'll need to change them in the code.

| GPIO     | Usage   |
| -------- | ------- |
| 01 | On Off Button (active high) |
| 02 | Start/Pause/Resume Button (active high) |
| 22 | Display I2C SDA |
| 23 | Display I2C SCL |
| 16 | Display I2C Reset |
| 20 | Rotary Encoder Clk |
| 18 | Rotary Encoder DT |

## Building

To compile this application, you will need esp-idf v5.4.1 and esp-matter v1.4. As I'm using a C6, I set the target accordingly. Once you have setup your esp-matter environment, you can compile it like this.

```
idf.py set-target esp32-c6
idf.py build flash monitor
```

If you are using a diffent ESP32, change the target accordingly.

## Commissioning

To commission the device, follow the instuctions here https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html#commissioning-and-control

## Using

I use the `chip-tool` for most testing, since Dishwashers aren't supported in iOS or Android.

Some example commands (based on a NodeId of 0x05) for turning it on, selecting a program and starting it.

```
chip-tool onoff on 0x05 0x01
chip-tool dishwashermode read supported-modes 0x05 0x01
chip-tool dishwashermode change-to-mode 1 0x05 0x01
chip-tool operationalstate start 0x05 0x01
```

## Things to do

[ ] Display a QR code or setup code if device is uncommissioned.
[ ] Implement the property DeadFront behaviour, where all changes are ignored whilst the device is off (I think!)
[ ] Reset Current Phase to 0 when Operational State is changed to Stopped.

## Feedback please!

If you have any suggestions, I'd love to hear them!

## Little Enclosure

To complete the effect, I've built a small enclosure in foam board.

![IMG_8493](https://github.com/user-attachments/assets/be44feea-0bbc-4ee8-a25a-3436a507302f)
