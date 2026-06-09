# PCB preview images

Tallenna GitHubissa naytettavat KiCad-esikatselut tahan kansioon:

```text
schematic.png
pcb-layout.png
3d_pcb_layout.png
```

## Schematic-kuva

1. Avaa `.kicad_sch` KiCad Schematic Editorissa.
2. Suorita ERC ja korjaa virheet.
3. Valitse **File > Plot**.
4. Vie tai ota kuva ja tallenna se nimella `schematic.png`.

Rajaa kuva niin, etta koko kytkentakaavio nakyy selkeasti.

## PCB-layout-kuva

1. Avaa `.kicad_pcb` KiCad PCB Editorissa.
2. Suorita DRC ja korjaa virheet.
3. Valitse **File > Plot**.
4. Nayta ainakin kerrokset `F.Cu`, `B.Cu`, `F.Silkscreen` ja `Edge.Cuts`.
5. Vie tai ota kuva ja tallenna se nimella `pcb-layout.png`.

## PCB:n 3D-kuva

1. Avaa PCB Editorissa **View > 3D Viewer**.
2. Sovita piirilevy nakymaan kokonaan.
3. Valitse 3D Viewerin kuvankaappaus- tai PNG-vientitoiminto.
4. Tallenna kuva nimella `3d_pcb_layout.png`.

Kun kuvat ovat tassa kansiossa, lisaa ne README-tiedostoon:

```markdown
![Schematic](hardware/pcb/previews/schematic.png)
![PCB layout](hardware/pcb/previews/pcb-layout.png)
![PCB 3D layout](hardware/pcb/previews/3d_pcb_layout.png)
```
