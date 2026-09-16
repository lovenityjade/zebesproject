> **Public source layout:** development scripts and test harnesses referenced below
> are retained in the private development repository. For current public build
> commands, see the [README](https://github.com/lovenityjade/zebesproject#build-from-source-linux-development).

# Super Metroid dans Unreal — direction et limites

Direction acceptée : 2D fidèle et pixel perfect, sprites et décors originaux,
effets d'atmosphère et de luminosité désactivables. Le shader d'affinage
Scale2x a été remplacé ; aucun lissage des contours n'est demandé.

## Architecture actuelle

- `supermetroid/` : désassemblage strager, conservé comme référence. La ROM
  reconstruite est identique à la copie personnelle Japan/USA.
- `native-core/` : base C snesrev/sm au commit
  `578f90b3cc49557bb70060ad033bb90b8cf8ac50`, non modifiée.
- `Native/` : bibliothèque C intégrée à Unreal. La logique du jeu et le
  lecteur audio natif tournent sur le thread jeu. Toute tentative d'exécuter
  une instruction CPU 65816 déclenche une erreur explicite.
- `Unreal/` : projet UE 5.8.2. Unreal gère la fenêtre, les entrées,
  la sortie audio, la texture dynamique et le matériau d'atmosphère.
- `Shaders/Present.usf` : source éditable du shader, incorporée au matériau
  `/Game/SM/M_Present` par `Scripts/create_material.py`.

Le rendu de référence des couches SNES reste calculé en logiciel par le
code PPU de la base C. Il fournit l'image pixel d'origine au matériau Unreal.
Ce premier jalon n'est donc pas encore un monde composé de sprites et de
salles Unreal éditables individuellement. Le rendu logiciel optimisé du PPU amont
n'est pas utilisé : il donnait une image noire lors des tests locaux.

## Effets

La texture source est lue au centre exact de chaque pixel, avec filtrage
nearest, puis affichée à une échelle entière et à une position entière.
Le fond conserve la résolution d'origine 256 × 224. La présentation actuelle
superpose une copie gaussienne de toute la scène à son calque net, en Lighten
ou Multiply. L'opacité est réglable ; le GUI est exclu du résultat et du flou.
Le calque net reste nearest, tandis que la copie floutée est douce à la
résolution d'affichage. Voir [le traitement actuel](GAUSSIAN-LAYER.md).

La [profondeur discrète](DEPTH.md) ajoute des plans de paysage à Crateria et
des ombres de contact sur les fonds intérieurs identifiés. Le calque Lighten
reste appliqué après cette composition.

La seconde image de présentation permet toujours la parallaxe : panneaux à
damier séparés à Cérès, ou BG2 dans les salles Mode 1 éligibles. L'ancien shader
de lumières par source et son inventaire sont conservés, mais ses lumières ne
sont pas appliquées par le matériau actuel.

Les réglages sont stockés dans `Unreal/Saved/SM/Presentation.ini`.
Les sauvegardes SRAM sont dans `Unreal/Saved/SM/sram.dat`.
Les essais automatisés utilisent exclusivement `Unreal/Saved/SMTests/`.

## Lancer et modifier

```sh
./Lancer-Super-Metroid.sh
```

F1 affiche les commandes. F2 active/désactive le calque ; F3 règle
son opacité ; F4 règle la luminosité ; F5 règle la parallaxe ; F6 choisit
Lighten/Multiply ; F11 bascule en plein écran.
P met en pause et Échap quitte. Flèches : déplacement ; Z : saut/confirmation ;
X : course ; S : tir ; C : annuler l'objet ; A/D : visée ; Maj droite :
changer d'objet ; Entrée : Start. Une manette SDL reconnue par Unreal peut
également fournir ses boutons et son stick gauche ; le confort matériel
reste à vérifier avec la manette réelle.

```sh
Scripts/build.sh
Scripts/prepare-assets.sh
```

La seconde commande régénère le matériau après modification du shader.
La compilation est limitée à deux tâches, le jeu à 60 images affichées/s.

## Travail restant pour le port complet

Le rendu élargi 400 × 224 et le paquet autonome sont implémentés et testés sur
gaming-pc. `Scripts/package.sh` construit et assemble l'exécutable, les
ressources locales et la dépendance interne du randomizer. Les adaptations
C sont générées dans `Native/build`, en gardant les deux dépôts amont intacts.

Les menus du randomizer et des patches optionnels restent à connecter.
Aucun parcours complet du jeu n'a été validé : les boss, sauvegardes et
transitions au-delà des tests ciblés restent à parcourir. Les attributions
amont restent dans `native-core/LICENSE.txt` et `Randomizer/upstream/LICENSE`.
Voir [les résultats et leurs limites](NIGHT-RESULTS.md).

## Validation

Les captures réelles et leurs mesures sont décrites dans
`runtime-verification.json` et `pixel-verification.json`. La capture sans
effet doit correspondre exactement au framebuffer natif ; les captures
Lighten et Multiply sont comparées à une référence mathématique indépendante.
La vérification à Cérès ne constitue pas une validation du jeu complet.
