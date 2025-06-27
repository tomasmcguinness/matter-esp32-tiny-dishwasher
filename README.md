# ACME Dishwasher

This code creates a "working" dishwasher that can be connected to and controlled via the Matter Protocol. Powered by the ESP32, it supports most of the Matter Dishwasher devicetype with the mandatory OperationalState cluster as well as the optional OnOff and DishwasherMode clusters.

This is a work in progress and far from complete. 

## Wiring

This code has been built for the XIAO ESP32-C6. Currently, the pinouts are hardcoded.

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

To compile this application, you will need esp-idf v5.4.1 and esp-matter v1.4

```
idf.py set-target esp32-c6
idf.py build flash monitor
```

## Commissioning

To commission the device, follow the instuctions here https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html#commissioning-and-control

I use the `chip-tool` for most testing, since Dishwashers aren't supported in iOS or Android.

Some example commands (based on a NodeId of 0x05)

```
chip-tool onoff on 0x05 0x01
chip-tool operationalstate start 0x05 0x01
chip-tool dishwashermode read supported-modes 0x05 0x01
chip-tool dishwashermode change-to-mode 1 0x05 0x01
```
