# Mufuki Project

## Short Story
- I've added **quite a few** new updates, including **process separation, new UI, easier-to-understand units, layout redesign, etc.** So if you've ever used Mufuki, you might have to relearn how the new UI is laid out.
- The new COM app is only built to its basic state using AI. This part gave me quite a headache because I had to balance cross-platform compatibility and ease of development, so for now, please enjoy the AI ​​slop I created.
- The Wiki will be fixed soon, don't worry.

## Updated:
- New UI Style (not font)
- Menu layout redesign
- Added unit options (mm or normalized)
- ~~(Small gadget) Press and hold F4 for more than 0.5 seconds and less than 1 second (in keypad mode) to open the GUI for string input.~~

Ladies and gentlemen, at the moment I'm writing these `README.md` lines, I just realized I accidentally deleted `main.cpp` and now I have to rewrite it from memory. I'm stupid, so that "little gadget" is gone too :)

## On Progress
- API App (with config editor, Game Sense, etc)
- USB COM functionality for osu! display if posible

## Planing:
- More effect for the underglow leds since they are rgb now
- Macro support
- Mablet (hall effec tablet)

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