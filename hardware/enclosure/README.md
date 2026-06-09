# Enclosure

Muokattava OpenSCAD-lahde:

- `esp32_kastelu_kotelo.scad`

Mallin PCB-mitta on `105 x 70 mm`, joka vastaa nykyisen KiCad-PCB:n
reunamittaa. Kotelossa on 3 mm sivuvalys PCB:n jokaiselle reunalle.

OpenSCAD-malli kaantyy onnistuneesti. GitHubissa naytettava OpenSCAD-kuva on
[`previews/kotelo.png`](previews/kotelo.png). Kansiossa oleva
`enclosure.png` on erillinen automaattisesti generoitu renderointi.

## Tarkistettava ennen tulostusta

- Kotelossa ei ole viela PCB:n kiinnitystolppia tai ruuvireikia.
- Aukkojen sijainnit on verrattava fyysiseen ESP32-korttiin ja liittimiin.
- Kannen sisahuulen sopivuus riippuu tulostimen toleransseista.
- Tulosta ensin pieni testiversio tai vain sovitukseen tarvittavat osat.

Generoidut STL-tiedostot kuuluvat `generated/`-kansioon, jota ei tallenneta
Git-versionhallintaan. Julkaise tulostusvalmis STL tarvittaessa GitHub
Releasen liitteena.
