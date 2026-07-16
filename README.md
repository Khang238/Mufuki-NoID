# Mufuki Project

## What is this slop?
This is a project where I tried to build my own rapid-trigger keypad, it's pretty bad and still has plenty of bugs. And yes, I used AI (like 40%) so feel free to say the project is bad if you want.

If you want to know what is it look like: [Mufuki v2 Preview](https://www.youtube.com/watch?v=Go5QbSOd0wY)

## What do it have:
- Dynamic Actuation (rapid trigger-ish)
- Macro system
- Display animation/GIF
- Run as keyboard, mouse and gamepad
- Noise filter for handmade build

Bluetooth/BLE connectivity has currently been disabled in the code because it consumes excessive flash memory, causes the ESP32 to overheat, and results in unstable connections and operation; if you need to use Mufuki with a phone, tablet, or other handheld device, I recommend using an OTG or C-to-C cable.

## Updated:
- Alway on display with clock (still buggy, requires internet connection)
- Macro support with anti-cheat

## On Progress
- random things (i ran out of thing to do)
    - actually remove old variables since the new profile system already has them

## Planing:
- True Rapid Trigger (will be named `Vector` instead of RT)
- More effect for the underglow leds since they are rgb now
- osu! mode, like Display scores, hit, etc

## Wiring diagram (you can change it btw)

| S3 Mini Pin  | Component                    |
|--------------|------------------------------|
| 48           | Build-in ws2182b             |
| 11           | Underglow led (ws2812b mode) |
| 1, 2, 3      | Analog input                 |
| 5, 6, 12, 13 | F1, F2, F3, F4 buttons       |
| 8, 9         | I2C OLED display/MPU6050     |

###### User manual/wiki can be found [he](https://youtu.be/dQw4w9WgXcQ?si=5hyU3H2vomW0COuk)[re](https://github.com/Khang238/Mufuki-NoID/wiki)

-- NoID --
