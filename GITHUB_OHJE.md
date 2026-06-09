# GitHub-ohje

Projektin GitHub-repositorio:

https://github.com/vallutskagg/kastelujarjestelma

## Lataa projekti

```powershell
git clone https://github.com/vallutskagg/kastelujarjestelma.git
cd kastelujarjestelma
```

## Julkaise paikalliset muutokset

Tarkista aina ensin, mita olet julkaisemassa:

```powershell
git status
```

Lisaa muutokset, tee commit ja pushaa:

```powershell
git add .
git commit -m "Kuvaa muutos lyhyesti"
git push
```

`sdkconfig`, `build/`, KiCadin varmuuskopiot, lukitustiedostot ja paikalliset
Gerber-tiedostot on ohitettu `.gitignore`-tiedostossa. Ala lisaa niita
GitHubiin kasin.

## PCB- ja kotelotiedostot

- KiCad-lahteet ovat kansiossa `hardware/pcb/`.
- OpenSCAD-kotelomalli on kansiossa `hardware/enclosure/`.
- GitHubissa naytettavat kuvat ovat `previews/`-kansioissa.
- Julkaise tarkistetut Gerber ZIP- ja STL-tiedostot GitHub Releasen liitteina.
