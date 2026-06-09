# PCB design status

Nykyiset KiCad-lahdetiedostot:

- `irrigation_esp32_J8J9_KUVAN_MUKAINEN.kicad_sch`
- `pcb.kicad_pcb`

## Tarkistettu

- KiCad-tiedostomuoto on version 10.0 muodossa.
- PCB:n reunamitta on `105 x 70 mm`.
- Firmware-signaalit ovat PCB:ssa:
  - `GPIO4`: DS18B20
  - `GPIO25`: tankin LED
  - `GPIO26`: rele
  - `GPIO27`: tankin tasoanturi
  - `GPIO34`: maankosteusanturi
- PCB:ssa on kuparivedot ja GND-kuparialueet molemmilla kuparikerroksilla.
- Anturiliittimet `J4-J7` ja ESP32-liittimet `J8/J9` ovat mukana.

## Korjattava ennen PCB:n tilaamista

- KiCad-projektitiedosto `.kicad_pro` puuttuu.
- Scheman ja PCB:n tiedostonimet eivat vastaa toisiaan.
- Pumpun virtaliittimet `J1`, `J2` ja relekontaktin liitin `J10` puuttuvat
  nykyisesta schemasta ja PCB:sta.
- `J3`-releliittimen pinni 3 on PCB:ssa `+3.3V`. Projektin
  `PCB_OHJE.md` suosittelee WayinTopin 5 V relemoduulille `USB_5V`:a.
  Tarkista kaytetyn relemoduulin vaatima jannite ja korjaa schema seka PCB.
- KiCad ERC ja DRC on ajettava KiCadissa ennen valmistustiedostojen vientia.
- ESP32 DevKitin fyysinen pinijarjestys, rivivali ja USB-liittimen sijainti on
  verrattava juuri kaytettavaan korttiin.

Nykyista PCB:ta ei tule tilata ennen naiden kohtien tarkistamista.

## GitHubiin tuotavat KiCad-tiedostot

Tuo projektiin:

- nykyista schemaa vastaava `.kicad_pro`
- `.kicad_sch`
- `.kicad_pcb`
- BOM-tiedosto, jos haluat osaluettelon ladattavaksi
- `component_placement_top.png` ja `pseudo_3d_render.png`, jos haluat
  lisakuvia GitHubiin
- Gerber ZIP GitHub Releasen liitteeksi, kun PCB on tarkistettu

Ala tuo tavalliseen Git-versionhallintaan:

- `.history/` ja `*-backups/`
- `*.kicad_prl` ja `*.kicad_pro.lck`
- `~*.kicad_pro.lck`
- irrallisia Gerber-kansioita, jos julkaiset Gerber ZIP:n Releasessa
- vanhoja tai eri versioon kuuluvia `.kicad_pro`, `.kicad_sch` ja
  `.kicad_pcb`-tiedostoja

`DRC.rpt`, `report.txt`, pinmap- ja kytkentaohjeet ovat valinnaisia. Tuo ne
vain, jos ne ovat ajan tasalla ja auttavat projektin kayttajaa.

## GitHub-esikatselut

Tallenna schematic-, layout- ja 3D-kuvat kansioon [`previews/`](previews/).
Kansion README sisaltaa kuvien vientiohjeet.
