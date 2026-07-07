# Mufuki Project

## What is this slop?
- This is a project where I tried to build my own rapid-trigger keypad, it's pretty bad and still has plenty of bugs. And yes, I used AI so feel free to say the project is bad if you want.

## Short Story
- I used Wokwi to create a Mufuki emulator right in VS Code; you can check it out (if your PC is powerful enough to run the emulator, mine doesn't).

## Updated:
- A litte more Effect for Menu
- Added unit options (mm or normalized)
- (Small gadget) Press and hold F4 for more than 0.3 seconds and less than 0.5 second (in keypad mode) to open the GUI for string input

## On Progress
- random things (i ran out of thing to do)
    - actually remove old variables since the new profile system already has them
    - check out sandbox to download the demo animation

## Planing:
- **REMOVE BLE FROM CODE SINCE IT EATING 30% OF THE FLASH RN**
- More effect for the underglow leds since they are rgb now
- Macro support

## Wiring diagram (you can change it btw)

| S3 Mini Pin  | Component                    |
|--------------|------------------------------|
| 48           | Build-in ws2182b             |
| 11           | Underglow led (ws2812b mode) |
| 1, 2, 3      | Analog input                 |
| 5, 6, 12, 13 | F1, F2, F3, F4 buttons       |
| 8, 9         | I2C OLED display/MPU6050     |

###### User manual can be found [here](https://youtu.be/dQw4w9WgXcQ?si=5hyU3H2vomW0COuk)

-- NoID --
