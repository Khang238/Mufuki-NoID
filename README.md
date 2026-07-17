# Mufuki Project

## About
This is **Mufuki** (or `MFK`, stand for `Multi-Functional Keypad`), a keypad project I made because I love [osu!](https://en.wikipedia.org/wiki/Osu!) but my keyboard are bad (membrane one). Started with that, then I made this, it's not perfect though, but that's why I keep improving it.

If you want to know what is it look like: [Mufuki v2 Preview](https://www.youtube.com/watch?v=Go5QbSOd0wY)

## What do it have:
- 4 Input Handler, with Vector and Dynamic Actuation is the closest thing to Rapid Trigger I have right now
- Macro system
- `4000Hz` Polling rate
- Display animation/GIF
- Run as keyboard, mouse and gamepad
- Noise filter for handmade build

Bluetooth/BLE connectivity has currently been disabled in the code because it consumes excessive flash memory, causes the ESP32 to overheat, and results in unstable connections and operation; if you need to use Mufuki with a phone, tablet, or other handheld device, I recommend using an OTG or C-to-C cable.

## Updated:
- Alway on display with clock (still buggy, requires internet connection)
- Macro support with anti-cheat
- New `Vector` Input Handler

## On Progress
- random things (i ran out of thing to do)
    - fixing some small bugs

## Planing:
- More effect for the backlight leds since they are rgb now
- osu! mode, like Display scores, hit, etc
- Per-key configuration

## Wiring diagram (you can change it btw)

| S3 Mini Pin  | Component                    |
|--------------|------------------------------|
| 48           | Build-in ws2182b             |
| 11           | backlight led (ws2812b mode) |
| 1, 2, 3      | Analog input                 |
| 5, 6, 12, 13 | F1, F2, F3, F4 buttons       |
| 8, 9         | I2C OLED display/MPU6050     |

## Note:
Yes, I used AI to help write about 25% of the code, but you can't blame me for doing the rest of the work myself. I’d love it if there is an AI that could generate a complete keypad for me, with pcb, acrylic cnc, and all the soldering done—while I just handed over my entire wallet, I’d absolutely love that

###### User manual/wiki can be found [h](https://youtu.be/dQw4w9WgXcQ?si=5hyU3H2vomW0COuk)[e](https://github.com/Khang238/Mufuki-NoID/wiki)[r](https://youtu.be/dQw4w9WgXcQ?si=5hyU3H2vomW0COuk)[e](https://github.com/Khang238/Mufuki-NoID/wiki)

-- NoID --
