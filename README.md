# Mufuki Project

## Attention!
This Version might not be stable!

## Short Story
I just rebuilt the project (physicaly) so this version will use ws2182b as underglow leds instead of the old analog ones, i still leave the option for analog led so that no one will be left out, but this project will have its own configuration option in the code to choose between the two
Also, for some reason my esp32 cant connect to wifi, so i might have to leave web app and wifi related features for later.
It is very high chance that web app will be cancelled to switch to API app because my Mufuki build right now is too unstable in terms of wifi connection, but since API app can cover almost everything (like Game Sense), i think ill try hard to see
Also im doing a profile v2 so things will be a bit messy for a while, i will clean up after (no promise tho)

## Updated:
- Setup Screen
- WS2812B Adressable LED compatibility
- Structure code refactor/split to make it more modular and easier to modify

## In progress:
- Profile V2
- Flexible mapping system (allow to map any input to any output, with some basic math operation in between)
- Base structure refactor for profile v2 and new mapping system to work

## Planing:
- API App (with config editor, Game Sense, etc)
- USB CDC functionality for osu! display if posible
- More effect for the underglow leds since they are rgb now
- Macro support

## Cancelled:
- Web App (to switch to API App, if API App is better)

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