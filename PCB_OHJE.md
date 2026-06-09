# PCB-ohje kastelujarjestelmalle (ESP32 + rele + maankosteus + DS18B20 + tankkitasoanturi + LED)

Tama ohje vastaa nykyista koodia:
- Rele ohjaus: `GPIO26`
- Maankosteus anturi (analog): `GPIO34` (ADC1_CHANNEL_6)
- DS18B20 data: `GPIO4`
- Kapasitiivinen tankkitasoanturi signaali: `GPIO27`
- Tankkitason LED-indikaattori: `GPIO25`

## 1) Tuotetiedot (DS18B20)

- Kaytossa on DS18B20 digitaalinen lampotila-anturi (1-Wire).
- Anturi toimii yksijohtimisella datavaylalla, joten useita DS18B20-antureita voi liittaa rinnakkain samaan DATA-linjaan.
- Suositeltu kayttojannite: `3.0V - 5.5V` (ESP32-projektissa kayta `3V3`).
- Mittausalue: `-55C ... +125C`
- Tarkkuus: noin `+/-0.5C` alueella `-10C ... +85C`
- Resoluutio: `9-12 bit`, oletus `12 bit`

DS18B20 johdinvarit toimittajan tekstin mukaan:
- `RED -> VCC`
- `GREEN -> DATA`
- `YELLOW -> GND`

WayinTop-kit (tuotekuvaus, tiivistetty):
- Tarkoitus: automaattinen kasvien kastelu maankosteuden perusteella.
- Paketti sisaltaa: kapasitiivinen maankosteusanturi, 1ch 5V relemoduuli, mini vesipumppu, 1 m vinyyliletku.
- Toimintaperiaate: kun maankosteus laskee, ohjaus kaynnistaa pumpun releen kautta.

WayinTop-osien speksit:
- Kapasitiivinen maankosteusanturi:
- Kayttojannite `3.3-5.5VDC`
- Analoginen ulostulo `0-3.0VDC`
- Liitin `PH2.54-3P`
- Koko `98 x 23 mm`
- Pinnit `AO`, `GND`, `VCC`
- 1ch relemoduuli:
- `VCC` = 5V syotto releelle
- `GND` = maa
- Mini pumppu:
- Kayttojannite `3-5VDC`
- Ulosmeno OD `7.5 mm`, ID `4.5 mm`
- Sisaanmeno `5 mm`
- Harjaton DC-rakenne (magneettiohjaus)
- Arvioitu jatkuva kayttoika noin `300 h`
- Vinyyliletku:
- Materiaali `PVC`
- Sisahalkaisija noin `5.54 mm`
- Ulkohalkaisija noin `8.20 mm`
- Pituus `100 cm`

## 2) Turvallisuus ja varoitukset

- Ala ylita syottojannitetta `5.5V`.
- Varmista oikea polariteetti (`VCC` ja `GND` oikein). Vaara kytkenta voi rikkoa anturin.
- Vaikka anturiprobe on kosteudelta suojattu, sita ei suositella jatkuvaan pysyvaan upotukseen vaikeissa olosuhteissa.
- Yhteinen maa (`GND`) on pakollinen: ESP32 + rele + kaikki anturit.
- Jos jokin anturi antaa `5V` signaalin, ala kytke sita suoraan ESP32 GPIO:hon. Kayta jannitejakoa tai level shifteria.
- Jos ohjaat verkkovirtaa (230VAC), tee sille erillinen turvallinen suunnittelu.
- Lue kaytto-ohje ennen asennusta ja varmista, ettei kastelujarjestelma voi aiheuttaa vuotovahinkoa.
- Asenna maankosteusanturi varovasti, jotta anturi ja johdot eivat vaurioidu.

## 3) 3.3V virtastrategia (tama projekti)

Tassa projektissa logiikkapuoli suunnitellaan `3.3V`:lle:
- ESP32: `3V3`
- DS18B20: `3V3`
- Kapasitiivinen maankosteusanturi: `3V3` (anturi tukee 3.3-5.5V)
- Tankkitasoanturi: ensisijaisesti `3V3` (tarkista oma anturiversio)

Rele ja pumppu:
- WayinTopin valmis 1-kanavainen relemoduuli on tyypillisesti `5V` kelalla.
- Relemoduulin syotto otetaan USB:n `5V` linjasta (`USB_5V` / ESP32 DevKit `5V`/`VBUS` pinni).
- Mini pumppu toimii alueella `3-5V`; voit kayttaa `3.3V` tai erillista `5V` syottoa virta- ja tuottovaatimusten mukaan.
- Muut anturit ja logiikka pidetaan `3.3V` kiskossa.
- Kaikissa tapauksissa `GND` on yhteinen.

## 4) Kytkenta (netit)

Nimea verkot (net labels) selkeasti:
- `3V3`
- `USB_5V` (USB:sta tuleva 5V releelle)
- `5V_PUMP` (valinnainen, jos pumppua ajetaan 5V:lla)
- `GND`
- `RELAY_IN_GPIO26`
- `SOIL_AO_GPIO34`
- `DS18B20_DQ_GPIO4`
- `TANK_LEVEL_SIG_GPIO27`
- `TANK_LED_GPIO25`
- `PUMP_SUPPLY+`
- `PUMP_SUPPLY-`
- `PUMP_OUT+`
- `PUMP_OUT-`

Kytke:
- ESP32 `GPIO26` -> relemoduulin `IN`
- ESP32 `GPIO34` -> maankosteusanturin `AO`
- ESP32 `GPIO4` -> DS18B20 `DATA`
- ESP32 `GPIO27` -> tankkianturin `SIGNAL`
- ESP32 `GPIO25` -> LED anodi sarjavastuksen kautta (esim. 330R), LED katodi -> `GND`
- DS18B20 `VCC` -> `3V3`
- DS18B20 `GND` -> `GND`
- `4.7k` vastus DS18B20 `DATA <-> 3V3` (pakollinen)
- Maankosteusanturi `VCC` -> `3V3`
- Maankosteusanturi `GND` -> `GND`
- Tankkitasoanturi:
- `RED -> VCC`
- `BLACK -> GND`
- `YELLOW -> SIGNAL (GPIO27)`
- Relemoduulin `GND` -> `GND`
- Relemoduulin `VCC` -> `USB_5V` (WayinTop 1ch 5V relemoduuli)

Pumpun tehopuoli releen kautta:
- `PUMP_SUPPLY+` (`3.3V` tai `5V_PUMP`) -> rele `COM`
- rele `NO` -> `PUMP_OUT+` (pumpun plus)
- `PUMP_OUT-` -> `PUMP_SUPPLY-`

## 5) Kaytto-ohje (DS18B20)

1. Kytke probe adapteriin johdinvarien mukaan.
2. Tarkista aina moduulin oma silkkipainatus ennen virtojen kytkentaa.
3. Jos kaytat Arduino IDE:ta, kirjastot ovat `OneWire` ja `DallasTemperature`.
4. Oletusresoluutio on 12-bit. Tarvittaessa voit pienentaa resoluutiota nopeampaa mittausta varten.

## 6) Liittimet PCB:lle (samat tyylit kuin kuvassa)

Pakollinen suunnittelusaanto:
- Jokaiselle laitteelle tulee oma erillinen liitin (ei jaettuja anturiliittimia).

- J1: `POWER_IN` 2-pin ruuviliitin (pumpun syotto), suositus `5.08 mm` jako
- J2: `PUMP_OUT` 2-pin ruuviliitin (pumppu), suositus `5.08 mm` jako
- J3: `RELAY_CTRL` 1x3 2.54mm pin header (`IN`, `GND`, `VCC`) kuten kuvan relemoduulissa (Dupont-yhteensopiva)
- J4: `SOIL_SENSOR` 1x3 2.54mm pin header (`AO`, `GND`, `VCC`) yhteensopiva kuvan anturin mukana tulevan kaapelin kanssa
- J5: `DS18B20` 1x3 2.54mm pin header (`DATA`, `VCC`, `GND`) DS18B20 adapterimoduulin signaalipuolelle
- J6: `TANK_LEVEL_SENSOR` 1x3 2.54mm pin header (`SIG`, `VCC`, `GND`) sailioanturin 3-johtimiselle kaapelille
- J7: `TANK_LED` 2-pin liitin (`LED+`, `GND`), suositus 2.54mm pin header
- J8/J9: ESP32-liitannat (devkit-headerit tai suora moduuli footprint)
- J10 (valinnainen): `RELAY_LOAD` 3-pin ruuviliitin (`NC`, `COM`, `NO`) jos relepiiri tehdaan suoraan omalle PCB:lle

Laitekohtainen liitinjako:
- Pumppu -> `J2` (`PUMP_OUT+`, `PUMP_OUT-`)
- Relemoduulin ohjaus -> `J3` (`IN`, `GND`, `VCC`)
- Maankosteusanturi -> `J4` (`AO`, `GND`, `VCC`)
- DS18B20 lampotila-anturi -> `J5` (`DATA`, `VCC`, `GND`)
- Sailio/tankkitasoanturi -> `J6` (`SIG`, `VCC`, `GND`)
- Tankkitason LED -> `J7` (`LED+`, `GND`)

Kuvien osat -> PCB-liitin (suora vastaavuus):
- DS18B20 probe + adapterimoduuli (kuva 1): adapterin 3-pin ohjausliitaanta -> `J5` (2.54mm, Dupont)
- Sailioanturi 3-johtimisella johdolla (kuva 2) -> `J6` (2.54mm, Dupont)
- Kapasitiivinen maankosteusanturi v1.2 (kuva 3) -> `J4` (2.54mm, Dupont / PH2.54-3P kaapeli)
- 1ch relemoduuli (kuva 3): ohjaus `IN/GND/VCC` -> `J3`, kuorma johdetaan moduulin omaan ruuviliittimeen

Releen toteutusvaihtoehdot:
- Vaihtoehto A (helpoin): kayta valmista 1ch 5V relemoduulia johdoilla (`IN/GND/VCC` + moduulin oma ruuviliitin kuormalle).
- Vaihtoehto B (siistein): tuo relepiiri suoraan emolevylle, mutta pidetaan samat ulkoliittimet kuin kuvassa: 1x3 ohjausheader + 3-pin ruuviliitin kuormalle.

## 7) ESP32 kokonaan piirilevylle (pinneineen)

Kuvan mukaisessa ratkaisussa ESP32-DevKit tulee suoraan emolevylle omilla pinneillaan:
- Kayta emolevylla kahta naarasrimaa (female header), joihin ESP32 DevKit painetaan.
- Yleisessa 38-pin DevKitissa: `2 x 19` pin, jako `2.54 mm`, rivien vali tyypillisesti `25.4 mm` (center-to-center).
- Tee silkkipainatukseen selkea merkinta: `USB`-paaty, `EN`, `BOOT`, `3V3`, `GND`.
- Jata USB-liittimen eteen mekaaninen vapaa tila, jotta kaapeli mahtuu paikalleen.
- Ala sijoita korkeita komponentteja EN/BOOT-painikkeiden viereen.
- Lukitse yksi tarkka ESP32-boardimalli ennen tilausta (esim. sama 38-pin malli kuin kuvassa), koska 30-pin/38-pin versiot eivat ole keskenaan suoraan yhteensopivia.

Kaytannon suositus:
- J8: `ESP32_LEFT` 1x19 female header
- J9: `ESP32_RIGHT` 1x19 female header
- Pinijarjestys tehdaan suoraan valitun DevKitin pinoutin mukaan.

## 8) Komponentit

- R1: `4.7k` (DS18B20 pull-up, DATA->3V3)
- R3: `330R` (LED sarjavastus GPIO25 -> LED+)
- C1: `100nF` DS18B20 liittimen lahelle (`VCC`->`GND`)
- C2: `100nF` maankosteusanturin liittimen lahelle (`VCC`->`GND`)
- C4: `100nF` tankkitasoanturin liittimen lahelle (`VCC`->`GND`)
- (Valinnainen) R2: `1k` sarjaan `SOIL_AO_GPIO34`
- (Valinnainen) C3: `100nF` `SOIL_AO_GPIO34` -> `GND` kohinan suodatukseen

WayinTop-kitista huomioitavat lisakomponentit:
- Relemoduuli 5V (jos kaytetaan valmista moduulia)
- Mini DC-pumppu `3-5V`
- PVC-letku `ID noin 5.54 mm`, `OD noin 8.20 mm`, pituus `1 m`

## 9) Layout-ohjeet

- Pida ADC-linja (`SOIL_AO_GPIO34`) kaukana releen ja pumpun tehopoluista.
- Tee yhtenainen GND-taso (ground plane).
- Reitita DS18B20 data lyhyena ja pida `4.7k` vastus lahella ESP32:ta.
- Jata ESP32-moduulin antennin alueen alle ja eteen kupariton keepout (ei kuparia eika tasoja antennin alle).
- Releen kontaktien ja logiikan valiin jata etaisyytta (vahin 2-3 mm DC-jarjestelmassa).
- Kayta pumpun virralle leveampia johtimia:
- signaalit: `0.25 mm`
- 3V3/5V: `0.5 mm`
- pumppulinja: `1.5-2.0 mm` (virran mukaan)

## 10) Mekaaniset tiedot (kuvan anturi)

Kuvan perusteella 3-johtimisen anturin mekaaniset mitat:
- Leveys noin `21 mm`
- Korkeus noin `24.1 mm`
- Kiinnitysreikien halkaisija noin `3 mm`
- Kaapeli: `UL2464 28#3 core`
- Johtimet ilman liitinta, kuorinta noin `5 mm`

Johdinvarit kuvan mukaan:
- `RED -> VCC`
- `BLACK -> GND`
- `YELLOW -> SIGNAL OUTPUT`

## 11) Huolto, varastointi, kierratys

- Pyri valttamaan jyrkkia taivutuksia ja johtoon kohdistuvaa vetoa.
- Sailuta kuivassa, kun laite ei ole kaytossa.
- Kosteissa olosuhteissa tarkista anturin metalliosat saannollisesti hapettumisen varalta.
- Ala havita elektroniikkaa sekajatteeseen, vaan toimita e-jatekierratykseen.

## 12) Firmware-vastaavuus (tarkeat asetukset)

Nykyiset toivotut asetukset:
- `CONFIG_WATERING_CHECK_INTERVAL_MS=3600000` (kosteus 1 h)
- `CONFIG_WATERING_TEMP_READ_INTERVAL_MS=1800000` (lampotila 30 min)
- `CONFIG_WATERING_DRY_THRESHOLD=2800`
- `CONFIG_WATERING_PUMP_ON_MS=8000` (oletuskastelu 8 s)
- `CONFIG_WATERING_PUMP_DURATION_MIN_S=8` (MQTT-saadon minimi 8 s)
- `CONFIG_WATERING_LEVEL_SENSOR_GPIO=27`
- `CONFIG_WATERING_LEVEL_LED_GPIO=25`

Jos vaihdat pinneja PCB:ssa, paivita:
- `CONFIG_WATERING_TEMP_GPIO`
- `CONFIG_WATERING_LEVEL_SENSOR_GPIO`
- `CONFIG_WATERING_LEVEL_LED_GPIO`
- `RELAY_GPIO` (main.c)
- `SOIL_CHANNEL` (main.c, ADC-kanava vastaa valittua GPIO:ta)

MQTT status -topicit tankkitasolle ja LEDille:
- `watering_system/status/tank_level` -> `FULL` / `LOW` / `UNKNOWN`
- `watering_system/status/tank_level_raw` -> `1` / `0` / `-1`

LED toimii paikallisena indikaattorina:
- LED syttyy, kun tankin tila on `LOW` (vetta ei ole riittavasti).

## 13) Ennen tilausta (checklist)

- Tarkista liittimien pinijarjestys silkkipainatuksesta.
- Tarkista, etta pumpulla, releella, maankosteusanturilla, DS18B20:lla ja sailioanturilla on kaikilla oma liitin.
- Tarkista, etta logiikkakiskot ovat `3V3` (ESP32, DS18B20, maankosteusanturi).
- Varmista, etta relemoduulin 5V syotto tulee USB-linjasta (`USB_5V`) ja etta `GND` on yhteinen.
- Tarkista ESP32 DevKitin tarkka pinimaara (30/38) ja mittaa rivivali tyontomitalla ennen PCB-tilausta.
- Varmista, etta USB-kaapeli mahtuu kiinni kun ESP32 on asennettuna emolevylle.
- Tarkista DS18B20 johdinjarjestys juuri omasta anturierasta.
- Tarkista erikseen tankkitasoanturin johdinjarjestys (kuvan mukaan RED/BLACK/YELLOW).
- Varmista relemoduulin triggeritaso (active-low sopii nykyiseen koodiin).
- Tee ensin prototyyppi (1 kpl), testaa ja vasta sitten isompi sarja.
