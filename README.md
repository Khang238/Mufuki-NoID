# Mufuki Project

## Attention!
I just rebuilt the project (physicaly) so this version will use ws2182b as underglow leds instead of the old analog ones, i still leave the option for analog led so that no one will be left out, but this project will have its own configuration option in the code to choose between the two
Also, for some reason my esp32 cant connect to wifi, so i might have to leave web app and wifi related features for later, but i will try to figure it out asap
Actually, i put two wires across the esp32 and somehow make the thing can't connect to wifi, so i rearranged them and now it works, so the web app and wifi related features are back on the table (maybe)

## Updated:
- Setup Screen
- WS2812B Adressable LED compatibility

## In progress:
- Structure code refactor/split to make it more modular and easier to modify

## Planing:
- Web App (soon, maybe next 10 years)
- API App (to replace Web App, if this is better, Web App will be cancelled)
- USB CDC functionality for osu! display if posible
- More effect for the underglow leds since they are rgb now
- Macro support

## Wiring diagram (you can change it btw)

| S3 Mini Pin  | Component                    |
|--------------|------------------------------|
| 48           | Build-in ws2182b             |
| 7            | Underglow led (ws2812b mode) |
| 1, 2, 3      | Analog input                 |
| 7, 6, 5      | Underglow led (analog mode)  |
| 5, 6, 12, 13 | F1, F2, F3, F4 buttons       |
| 8, 9         | I2C OLED display/MPU6050     |

###### User manual can be found [here](https://youtu.be/dQw4w9WgXcQ?si=5hyU3H2vomW0COuk)

-- NoID --