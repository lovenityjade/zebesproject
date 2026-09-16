# Révision lumière et profondeur — 14 septembre 2026

Historique v2 : les sources et la parallaxe sont conservées. Le matériau actuel
utilise un [calque gaussien sur toute la scène](../GAUSSIAN-LAYER.md), sans
appliquer les lumières ponctuelles décrites ci-dessous.

L'ambiance sombre remplace le premier shader fondé sur les pixels clairs.
Les couleurs et contours source restent intacts avec F2 désactivé ; avec les
effets, les blocs de pixels restent entiers mais leur luminosité change.

## Sources identifiées

- Cérès : cinq empreintes de tuiles couvrent les voyants bleus et les quatre
  parties de la lampe ambrée de l'ascenseur. Seuls leurs indices de palette
  lumineux émettent. Les arêtes métalliques et les marques rouges ne sont
  pas prises pour des lampes.
- Huit planches de feu : Norfair Erratic Fireball, Fire Geyser, Nuclear Waffle,
  flammes des quatre définitions Phantoon et Norfair Lava Man. Les empreintes
  des graphismes chargés en VRAM, l'OAM et la palette identifient les pixels.
  Leur intégration est écrite ; les rencontres correspondantes n'ont pas
  encore été validées en jeu.
- Tirs et explosions de Samus : positions et durée de vie natives. Lave et
  acide : type de fluide et hauteur natives. Ces chemins ont un test de
  comportement synthétique ; ils demandent encore une validation visuelle
  dans toutes les salles concernées.

`Config/lighting-sources.json` est la liste éditable. `Scripts/bake-lighting.py`
produit les 241 règles de tuiles utilisées par la bibliothèque. La lumière
prend la couleur de la palette courante. Les sprites ont un seuil de lisibilité,
sans source lumineuse artificiellement attachée au corps entier de Samus.
Les halos représentent une diffusion atmosphérique, sans ombres de géométrie.

## Inventaire et couverture

`Scripts/inventory-graphics.py` extrait les 29 entrées de décors, les 163
entrées d'ennemis nommées (154 planches non vides) et inventorie 28 fichiers
complémentaires du désassemblage, avec empreintes et pointeurs ROM.
`Scripts/catalogue-lighting.py` crée [le catalogue local](Inventory/index.html)
recherchable, comprenant les planches complètes disponibles.

Les aperçus ont permis de repérer les familles lumineuses ; **la révision
artistique exhaustive de chaque sprite, animation et décor n'est pas terminée**.
Les graphismes de Samus, effets animés et éléments chargés séparément ne sont
pas tous couverts par la table des ennemis. Les ressources sans annotation
restent `needs-review`, sans être automatiquement déclarées lumineuses.
Les captures `ceres-*-tiles.png` montrent les tuiles réellement présentes.

## Parallaxe

La présentation peut décaler BG2 de quelques pixels entiers, à partir du
mouvement de caméra. Les registres sont restaurés aussitôt ; la simulation
conserve sa caméra. Les pixels originaux du premier plan et des sprites sont
recopiés aux mêmes positions après composition. F5 active/désactive ce chemin.

Le filtre d'éligibilité est conservateur : Mode 1, salle en gameplay normal,
BG2 avec coefficient horizontal natif indépendant et sans gestion verticale
spéciale. Le mouvement supplémentaire est limité à ±8 pixels horizontaux et
±6 verticaux pour limiter l'exposition des bords de la carte chargée.
Cette détection reste expérimentale, pas une certification salle par salle.

Cérès utilise le Mode 7 : son décor inclut la géométrie jouable. Une séparation
spécifique extrait le motif à damier derrière les poutres : dix empreintes de
tuiles et des masques par pixel protègent le châssis, y compris les bordures des
panneaux. Le plan décoratif se décale à partir de la caméra, avec un motif
répétable issu de la ROM. Il est désactivé lors des transformations Mode 7 non
identitaires. Configuration : `Config/ceres-background.json`.
Les autres fonds Mode 7 restent à annoter séparément.

## Vérifications

- `Tests/check_scene.c` : fond BG2 mobile sur une scène synthétique, premier
  plan et HUD immobiles, image originale et registres conservés ; les pixels
  clairs non annotés ne deviennent pas des sources ; allumage/extinction des
  tirs actifs et lumière du fluide ; restauration exacte de la VRAM après
  composition du plan Mode 7.
- `Tests/audit_scene.py` : deux instantanés réels à Cérès, export des tuiles,
  palettes, OAM, masques d'émission et cartes de lumière ; comparaison de
  l'image originale et du panneau à damier décalé sur un même frame réel.
- `Scripts/test-pixels.sh` : captures Vulkan du même instant, référence exacte,
  grille de pixels conservée avec les effets, HUD identique, mesure des zones
  sombres et des émetteurs. Voir `../pixel-verification.json`.

La séparation à Cérès a une preuve sur le frame natif 9104 : 5 994 pixels du
plan à damier changent, aucun pixel hors du masque, aucun sprite ni pixel du
HUD. Voir `parallax-verification.json`, `ceres-background-mask.png` et les deux
captures `ceres-parallax-*.png`. Le masque a été inspecté visuellement : seuls
les intérieurs des panneaux sont blancs.

La parallaxe BG2 n'a pas encore de preuve visuelle dans une vraie salle Mode 1.
Le jeu complet et le vrai widescreen restent à valider/implémenter.
