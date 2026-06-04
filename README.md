# Mufuki Project

## Updated:
- Profile V2 with programable mapping system
- Some fonts have been changed to make it more good looking
- Extended function (Gamepad and Mouse)

## To do:
- Code cleanup and optimization for macro support
- Find out how to enable USB COM alongside with USB HID for API App

## Planing:
- API App (with config editor, Game Sense, etc)
- USB COM functionality for osu! display if posible
- More effect for the underglow leds since they are rgb now
- Macro support

## Wiring diagram (you can change it btw)

| S3 Mini Pin  | Component                    |
|--------------|------------------------------|
| 48           | Build-in ws2182b             |
| 11           | Underglow led (ws2812b mode) |
| 1, 2, 3      | Analog input                 |
| 7, 6, 5      | Underglow led (analog mode)  |
| 5, 6, 12, 13 | F1, F2, F3, F4 buttons       |
| 8, 9         | I2C OLED display/MPU6050     |

###### User manual can be found [here](https://youtu.be/dQw4w9WgXcQ?si=5hyU3H2vomW0COuk)

-- NoID --