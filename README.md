# Watering System

ESP32- ja ESP-IDF-pohjainen automaattinen kastelujarjestelma. Jarjestelma
mittaa maan kosteutta ja ulkolampotilaa, tarkkailee vesisailion tasoa ja ohjaa
vesipumppua releen kautta. Mittaus- ja ohjaustiedot valitetaan MQTT:lla.

Tama README sisaltaa kokoamis-, asennus-, kalibrointi-, kaytto- ja
GitHub-ohjeet, joiden avulla toinen kayttaja voi ottaa projektin kayttoon.

## Sisalto

- [Toimintaperiaate](#toimintaperiaate)
- [Tarvittavat osat](#tarvittavat-osat)
- [Kytkennat](#kytkennat)
- [Ohjelmiston asennus](#ohjelmiston-asennus)
- [Asetukset](#asetukset)
- [Kalibrointi](#kalibrointi)
- [MQTT-kaytto](#mqtt-kaytto)
- [Testaus](#testaus)
- [PCB- ja kotelotiedostot](#pcb--ja-kotelotiedostot)
- [Vianhaku](#vianhaku)

## Toimintaperiaate

ESP32 suorittaa seuraavaa toimintalogiikkaa:

1. Laite kaynnistaa releen pois paalta, alustaa anturit ja yhdistaa 2,4 GHz
   Wi-Fi-verkkoon seka MQTT-brokeriin.
2. Maankosteusanturista luetaan useita ADC-naytteita ja niiden keskiarvo.
3. Jos ADC-arvo ylittaa kuivuusrajan riittavan monta perakkaista kertaa, maa
   tulkitaan kuivaksi.
4. Pumppu kaynnistetaan vain, jos pumpun ohjaus on sallittu, kastelujen valinen
   turva-aika on kulunut ja sailioanturi ei ilmoita veden olevan vahissa.
5. Pumppu kayy asetetun ajan ja sammuu automaattisesti.
6. DS18B20-lampotila, maankosteus, sailion tila ja pumpun tila julkaistaan
   MQTT:lla.
7. Pumppua, mittausta ja kasteluaikaa voi ohjata myos MQTT-komennoilla.

Sailion tila `LOW` estaa seka automaattisen etta manuaalisen pumppauksen.
Tila `UNKNOWN` ei nykyisessa ohjelmassa esta pumppausta. Jos sailioanturi ei ole
kytketty tai toimii vaarin, pumppua saa kayttaa vain valvotusti.

Releohjaus on active-low:

- `GPIO26 = HIGH`: rele ja pumppu pois paalta
- `GPIO26 = LOW`: rele ja pumppu paalla

Kastelujen valinen turva-aika tallennetaan vain ESP32:n muistiin. Laitteen
uudelleenkaynnistys nollaa viimeisen kastelun ajan.

## Projektin rakenne

| Polku | Sisalto |
| --- | --- |
| [`main/`](main/) | ESP32-ohjelmiston lahdekoodi ja asetukset |
| [`hardware/pcb/`](hardware/pcb/) | KiCad-projekti ja piirilevyn lahdetiedostot |
| [`hardware/enclosure/`](hardware/enclosure/) | OpenSCAD-kotelomalli |
| [`PCB_OHJE.md`](PCB_OHJE.md) | PCB:n yksityiskohtaiset suunnitteluohjeet |
| [`GITHUB_OHJE.md`](GITHUB_OHJE.md) | Git- ja GitHub-tyoskentelyohje |
| `pump_test_values_backup_2026-04-10.txt` | Aiemmat pumpun testiasetukset |

## Tarvittavat osat

Pakolliset osat:

- ESP32 DevKit, jota ESP-IDF tukee
- kapasitiivinen maankosteusanturi, analoginen `0-3,0 V` ulostulo
- active-low-yhteensopiva 1-kanavainen 5 V relemoduuli
- 3-5 V DC-vesipumppu ja sopiva letku
- kapasitiivinen sailion tasoanturi digitaalisella ulostulolla
- virtalahde pumpulle ja releelle
- johdot ja liittimet

Suositellut tai valinnaiset osat:

- DS18B20-lampotila-anturi
- `4,7 kOhm` vastus DS18B20 DATA-linjan ja `3V3`:n valiin
- LED ja `330 Ohm` sarjavastus sailion alhaisen tason ilmaisimeksi
- `100 nF` ohituskondensaattorit antureiden syottoihin
- projektin KiCad-piirilevy ja 3D-tulostettu kotelo

Katso tarkempi komponentti- ja liitinluettelo tiedostosta
[`PCB_OHJE.md`](PCB_OHJE.md).

## Kytkennat

### ESP32-signaalit

| Toiminto | ESP32-pinni | Huomio |
| --- | --- | --- |
| Releen ohjaus | `GPIO26` | Active-low |
| Maankosteusanturin AO | `GPIO34` | ADC1 channel 6, enintaan 3,3 V |
| DS18B20 DATA | `GPIO4` | `4,7 kOhm` pull-up vastus `3V3`:een |
| Sailion tasoanturin signaali | `GPIO27` | Oletuksena HIGH tarkoittaa vetta |
| Sailion tason LED | `GPIO25` | Oletuksena LED syttyy, kun sailio on LOW |

### Peruskytkenta

- Kytke ESP32:n, relemoduulin, antureiden ja pumpun virtalahteen maat yhteen.
- Kytke maankosteusanturi `3V3`:een. Ala syota yli 3,3 V signaalia
  `GPIO34`:aan.
- Kytke DS18B20 `3V3`:een ja lisaa DATA-linjalle `4,7 kOhm` pull-up-vastus.
- Kytke relemoduulin `IN` pinniin `GPIO26`.
- Kytke pumpun virtapiiri releen `COM`- ja `NO`-kontaktien kautta.
- Kytke sailioanturi valmistajan pinijarjestyksen mukaisesti. Tarkista
  johdinvarit ennen virran kytkemista.

Tarkat netit, liittimet, jannitteet ja PCB-layout-ohjeet ovat tiedostossa
[`PCB_OHJE.md`](PCB_OHJE.md).

## Ohjelmiston asennus

### 1. Asenna tyokalut

Tarvitset:

- Git
- Espressif ESP-IDF
- USB-ajurin omalle ESP32-kehityskortille
- datansiirtoa tukevan USB-kaapelin

Windowsissa helpoin tapa on asentaa Espressif IDF Installer ja avata sen
luoma **ESP-IDF PowerShell**, jossa `idf.py` on kaytettavissa.

### 2. Lataa projekti

```powershell
git clone https://github.com/KAYTTAJANIMI/watering_system.git
cd watering_system
```

Korvaa osoite taman projektin oikealla GitHub-osoitteella.

### 3. Valitse kohde ja aseta tunnukset

```powershell
idf.py set-target esp32
idf.py menuconfig
```

Avaa valikko **Watering System** ja aseta ainakin:

- `WiFi SSID`
- `WiFi password`
- `MQTT broker URI`
- yksilollinen `MQTT client id`
- MQTT-kayttajatunnus ja salasana, jos broker vaatii ne

Wi-Fi- ja MQTT-salaisuudet tallentuvat paikalliseen `sdkconfig`-tiedostoon.
Tiedosto on ohitettu Gitissa, eika sita pida julkaista GitHubiin.

Jos kaytat julkista MQTT-brokeria, muuta kaikki MQTT-topic-nimet
yksilollisiksi. Muuten muut saman brokerin kayttajat voivat nahda viesteja tai
lahettaa pumppukomentoja.

### 4. Kaanna ja siirra ohjelma

```powershell
idf.py build
idf.py -p COM3 flash monitor
```

Korvaa `COM3` oman ESP32-laitteesi portilla. Sulje sarjamonitori
ESP-IDF:n pikanappaimella `Ctrl+]`.

Onnistuneessa kaynnistyksessa lokissa nakyy muun muassa Wi-Fi- ja
MQTT-yhteys, antureiden alustus seka kosteusmittauksia.

## Asetukset

Asetuksia muokataan komennolla `idf.py menuconfig` valikosta
**Watering System**.

| Asetus | Oletus | Merkitys |
| --- | ---: | --- |
| `WATERING_DRY_THRESHOLD` | `3000` | Taman ylittava ADC-arvo tulkitaan kuivaksi |
| `WATERING_MOISTURE_WET_ADC` | `1500` | 0 prosentin kuivuuden kalibrointipiste |
| `WATERING_MOISTURE_DRY_ADC` | `3200` | 100 prosentin kuivuuden kalibrointipiste |
| `WATERING_PUMP_ON_MS` | `8000` | Pumpun oletuskayntiaika millisekunteina |
| `WATERING_CHECK_INTERVAL_MS` | `60000` | Maankosteuden automaattinen mittausvali |
| `WATERING_DRY_CONSECUTIVE_READS` | `3` | Vaaditut perakkaiset kuivat mittaukset |
| `WATERING_MIN_WATERING_INTERVAL_HOURS` | `72` | Kastelujen valinen turva-aika |
| `WATERING_ADC_SAMPLES` | `8` | Keskiarvoon kaytettavien ADC-naytteiden maara |
| `WATERING_TEMP_READ_INTERVAL_MS` | `10000` | Lampotilan mittausvali |
| `WATERING_MQTT_CMD_ARM_DELAY_MS` | `5000` | MQTT-komentojen esto yhdistamisen jalkeen |

Ensimmainen testaus kannattaa tehda lyhyella pumpun kayntiajalla ja ilman,
etta letku osoittaa kasviin. Aseta lopulliset arvot kasvin, ruukun, pumpun ja
anturin mittausten perusteella.

## Kalibrointi

Maankosteusanturin ADC-arvot riippuvat anturista, maaperasta ja sijoituksesta.
Kalibroi jokainen asennus erikseen:

1. Avaa sarjamonitori komennolla `idf.py monitor`.
2. Mittaa anturi kuivassa tai kuivassa mullassa ja kirjaa lokin
   `Soil ADC avg` -arvo.
3. Mittaa anturi hyvin kosteassa mullassa ja kirjaa arvo.
4. Aseta kuiva arvo kohtaan `WATERING_MOISTURE_DRY_ADC`.
5. Aseta marka arvo kohtaan `WATERING_MOISTURE_WET_ADC`.
6. Valitse `WATERING_DRY_THRESHOLD` kuivan ja maran arvon valilta.
7. Testaa usean paivan ajan valvotusti ennen automaattisen kastelun kayttoa.

Tassa projektissa suurempi ADC-arvo tarkoittaa oletuksena kuivempaa maata.
Anturia ei saa upottaa elektroniikkaosan yli.

## MQTT-kaytto

Oletusbroker on vain kehitysta varten `mqtt://broker.hivemq.com`. Omaan
jatkuvaan kayttoon suositellaan salasanalla ja TLS-yhteydella suojattua
brokeria, esimerkiksi paikallista Mosquitto-palvelinta.

### Julkaistavat topicit

| Topic | Sisalto |
| --- | --- |
| `watering_system/soil` | JSON: kosteus, kuivuusprosentti, lampotila ja ajat |
| `watering_system/soil/raw` | Maankosteuden ADC-raaka-arvo |
| `watering_system/soil/dry_percent` | Kuivuus `0-100` prosenttia |
| `watering_system/temperature/outdoor_c` | DS18B20-lampotila Celsius-asteina |
| `watering_system/status/pump_duration_s` | Pumpun aktiivinen kayntiaika |
| `watering_system/status/pump_enable` | `ON` tai `OFF` |
| `watering_system/status/pump_running` | `ON` tai `OFF` |
| `watering_system/status/tank_level` | `FULL`, `LOW` tai `UNKNOWN` |
| `watering_system/status/tank_level_raw` | `1`, `0` tai `-1` |

Esimerkki `watering_system/soil`-viestista:

```json
{
  "moisture": 2875,
  "dry_percent": 81,
  "temperature_c": 21.50,
  "dry_threshold": 3000,
  "ts": 1781000000,
  "last_watering_ts": 0,
  "last_watering_local": "never"
}
```

### Vastaanotettavat komennot

| Topic | Hyvaksytty viesti | Toiminto |
| --- | --- | --- |
| `watering_system/cmd/pump_duration_s` | kokonaisluku, oletuksena `8-20` | Muuttaa pumpun kayntiaikaa |
| `watering_system/cmd/pump_manual` | `RUN`, `ON`, `1` tai `START` | Kaynnistaa manuaalisen kastelun |
| `watering_system/cmd/pump_enable` | `ON`, `OFF`, `1` tai `0` | Sallii tai estaa pumpun |
| `watering_system/cmd/measure_now` | `READ`, `MEASURE`, `ON` tai `1` | Julkaisee uuden kosteusmittauksen |

Esimerkit Mosquitto-tyokaluilla:

```powershell
mosquitto_sub -h BROKERIN_OSOITE -t "watering_system/#" -v
mosquitto_pub -h BROKERIN_OSOITE -t "watering_system/cmd/measure_now" -m "READ"
mosquitto_pub -h BROKERIN_OSOITE -t "watering_system/cmd/pump_enable" -m "OFF"
```

Pumpun manuaalinen kaynnistys:

```powershell
mosquitto_pub -h BROKERIN_OSOITE -t "watering_system/cmd/pump_manual" -m "RUN"
```

Suorita manuaalinen pumppukomento vain, kun laite ja veden kulku ovat
valvottuja.

## Testaus

Tee testit tassa jarjestyksessa:

1. Tarkista kaikki jannitteet ja polariteetit yleismittarilla ilman ESP32:ta.
2. Kaynnista ESP32 ilman pumppua ja tarkista sarjaloki.
3. Tarkista maankosteuden raaka-arvo kuivassa ja kosteassa mullassa.
4. Tarkista DS18B20:n lampotila.
5. Tarkista, etta sailioanturin tilat `FULL` ja `LOW` vaihtuvat oikein.
6. Tarkista ilman pumppua, etta rele kytkeytyy active-low-logiikalla.
7. Testaa pumppu lyhyesti erillaan kasvista.
8. Tarkista, etta `LOW`-sailiotila estaa pumppauksen.
9. Testaa lopuksi automaattikastelu valvotusti.

Releelle on `menuconfig`-valikossa jatkuva self-test-tila. Kun se on kaytossa,
normaali kastelulogiikka ei kaynnisty. Ala jata self-test-tilaa paalle
varsinaiseen kayttoon.

## PCB- ja kotelotiedostot

### Esikatselut

#### Schematic

![Kastelujarjestelman schematic](hardware/pcb/previews/schematic.png)

*Kuva 1. Kastelujarjestelman KiCad-kytkentakaavio. Kaavio sisaltaa ESP32:n,
maankosteusanturin, DS18B20-lampotila-anturin, sailion tasoanturin, LEDin ja
releen ohjausliitannan.*

#### PCB-layout

![Kastelujarjestelman PCB-layout](hardware/pcb/previews/pcb-layout.png)

*Kuva 2. Kastelujarjestelman kaksipuolinen PCB-layout. Punainen kuvaa
etupuolen kuparia ja sininen takapuolen kuparia.*

#### PCB:n 3D-nakyma

![Kastelujarjestelman PCB 3D-layout](hardware/pcb/previews/3d_pcb_layout.png)

*Kuva 3. KiCadin 3D-layout piirilevysta. Kuvassa nakyvat piirilevyn
komponenttien ja liittimien sijoittelu seka kuparivetojen reititys.*

#### Kotelon 3D-malli

![Kastelujarjestelman kotelo](hardware/enclosure/previews/kotelo.png)

*Kuva 4. OpenSCADilla mallinnettu kotelon pohja ja irrotettava kansi.*

Kuvat nakyvat automaattisesti GitHubissa, kun ne tallennetaan naihin
tiedostopolkuihin ja lisataan Git-versionhallintaan. KiCad-kuvien vientiohjeet
ovat kansiossa [`hardware/pcb/previews/`](hardware/pcb/previews/).

### Mihin tiedostot lisataan

Tallenna muokattavat KiCad-lahdetiedostot tahan kansioon:

```text
hardware/pcb/
```

Nykyiset KiCad-tiedostot:

```text
hardware/pcb/irrigation_esp32_J8J9_KUVAN_MUKAINEN.kicad_sch
hardware/pcb/pcb.kicad_pcb
```

KiCad-projektitiedosto `.kicad_pro` puuttuu viela. Luo projekti KiCadissa ja
tallenna projektitiedosto samaan kansioon. Projektikohtaiset KiCad-symbolit,
footprintit ja kirjastotaulukot kuuluvat myos samaan kansioon.

Tallenna muokattava OpenSCAD-kotelomalli tahan kansioon:

```text
hardware/enclosure/esp32_kastelu_kotelo.scad
```

Kun tiedostot on lisatty ja julkaistu GitHubiin, kayttaja voi:

- selata ja ladata lahdetiedostoja kansioista
  [`hardware/pcb/`](hardware/pcb/) ja
  [`hardware/enclosure/`](hardware/enclosure/)
- ladata koko projektin GitHubin **Code > Download ZIP** -toiminnolla
- kloonata projektin komennolla `git clone`
- avata KiCad-lahteet KiCadissa ja `.scad`-tiedoston OpenSCADissa

GitHub nayttaa `.scad`-tiedoston tekstina. KiCad-tiedostoja voi tarkastella
tekstina, mutta varsinainen schema ja piirilevy avataan KiCadissa.

Jos haluat suunnitelmien olevan helposti tarkasteltavissa suoraan GitHubissa,
vie niista esikatselukuvat ja tallenna ne versionhallintaan:

```text
hardware/pcb/previews/schematic.png
hardware/pcb/previews/pcb-layout.png
hardware/pcb/previews/3d_pcb_layout.png
hardware/enclosure/previews/kotelo.png
```

PNG- ja SVG-kuvat nakyvat suoraan GitHubin sivulla. Lisaa esikatselukuvat
normaalisti Gitiin yhdessa lahdetiedostojen kanssa.

### Valmistus- ja tulostustiedostot

KiCadista generoidut Gerber-, poraus-, BOM- ja pick-and-place-tiedostot
kuuluvat paikallisesti kansioon:

```text
hardware/pcb/generated/
```

OpenSCADista generoidut STL- ja 3MF-tiedostot kuuluvat paikallisesti kansioon:

```text
hardware/enclosure/generated/
```

Nama `generated/`-kansiot on ohitettu Gitissa. Kun suunnitelma on valmis,
pakkaa valmistustiedostot ZIP-tiedostoiksi ja liita ne GitHub Releasen
ladattaviksi. Talla tavalla:

- GitHub-repositorio sisaltaa aina muokattavat KiCad- ja OpenSCAD-lahteet
- Release sisaltaa tiettyyn versioon kuuluvat valmiit Gerber- ja STL-tiedostot
- toinen kayttaja voi joko muokata suunnitelmaa tai ladata valmiin version

Tiedostojen lisaaminen Gitiin:

```powershell
git add hardware/pcb hardware/enclosure README.md
git status
git commit -m "Add PCB design and enclosure model"
git push
```

Tarkempi julkaisuohje on tiedostossa [`GITHUB_OHJE.md`](GITHUB_OHJE.md).

## Vianhaku

**`idf.py`-komentoa ei loydy**

Avaa Espressif-asennuksen luoma ESP-IDF PowerShell tai aktivoi ESP-IDF-
ymparisto ennen komentojen suorittamista.

**ESP32 ei yhdista Wi-Fiin**

Varmista, etta verkko on 2,4 GHz ja SSID seka salasana on asetettu
`idf.py menuconfig` -valikossa.

**MQTT-yhteys ei toimi**

Tarkista brokerin URI, portti, tunnukset, palomuuri ja TLS-asetukset.

**Maankosteusarvo ei muutu**

Tarkista anturin `AO`, `VCC` ja `GND`, mittaa `AO` yleismittarilla ja varmista,
etta signaali on kytketty `GPIO34`:aan.

**Pumppu ei kaynnisty**

Tarkista MQTT:n `pump_enable`, sailion tila, releen active-low-logiikka,
pumpun erillinen syotto ja yhteinen maa.

**Pumppu kaynnistyy vaarin pain**

Nykyinen ohjelma olettaa active-low-releen. Ala jatka kayttoa ennen kuin releen
logiikka ja turvallinen alkutila on korjattu.

**DS18B20 ei anna lampotilaa**

Tarkista johdinjarjestys ja `4,7 kOhm` pull-up-vastus DATA-linjan ja `3V3`:n
valilla.

## Turvallisuus

- Testaa laite ja pumppu aina valvotusti ennen automaattista kayttoa.
- Varmista yhteinen maa, oikeat kayttojannitteet ja riittava virtalahde.
- Suojaa elektroniikka vedelta ja suunnittele vuodon seuraukset etukateen.
- Ala syota yli 3,3 V jannitetta ESP32:n GPIO-pinneihin.
- Ala kytke 230 V verkkovirtaa taman projektin releeseen ilman asianmukaista
  sahkosuunnittelua, eristyksia ja kotelointia.
- Julkisessa MQTT-brokerissa kuka tahansa voi mahdollisesti nahda tai lahettaa
  viesteja. Kayta tuotantokaytossa suojattua brokeria ja yksilollisia topic-
  nimia.

## Lisenssi

Projektissa ei ole viela lisenssitiedostoa. Ennen julkista julkaisua lisaa
projektiin sopiva `LICENSE`, jotta muut kayttajat tietavat, miten ohjelmistoa,
PCB-suunnitelmaa ja kotelomallia saa kayttaa.
