Tämä versio sisältää kuparivedot KiCad PCB -tiedostossa.

Tehdyt reititykset:
- GPIO26 -> J3 RELAY IN
- GPIO34 -> R2/C3 -> J4 SOIL AO
- GPIO4 -> R1 pull-up -> J5 DS18B20 DATA
- GPIO27 -> J6 TANK SIG
- GPIO25 -> R3 -> J7 LED+
- 3V3 antureille ja kondensaattoreille
- USB_5V -> J3 relemoduulin VCC
- PUMP_SUPPLY+ -> J10 COM
- J10 NO -> J2 PUMP_OUT+
- GND explicit B.Cu -vedot + GND zone

Huom: aja KiCadissa vielä DRC ja täytä GND zone (B). Tarkista ennen tilausta fyysisen ESP32 DevKitin J8/J9 pinout ja USB-pään suunta.
