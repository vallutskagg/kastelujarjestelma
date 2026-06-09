Korjattu KiCad-paketti

Tämä versio käyttää J8/J9 1x19 female-headereitä ja ESP32-WROOM-32 38-pin -kuvan mukaista pinoutia.

Tärkeimmät korjaukset:
- J8 pin 1 = 3V3
- J8 pin 5 = GPIO34 / SOIL_AO
- J8 pin 9 = GPIO25 / TANK_LED
- J8 pin 10 = GPIO26 / RELAY
- J8 pin 11 = GPIO27 / TANK_SIG
- J8 pin 14 = GND
- J8 pin 19 = USB_5V
- J9 pin 1 = GND
- J9 pin 7 = GND
- J9 pin 13 = GPIO4 / DS18B20

Avaa .kicad_pro KiCadissa ja aja ERC/DRC ennen valmistusta.
