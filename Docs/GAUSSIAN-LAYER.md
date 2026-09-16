# Calque gaussien sur toute la scène

Direction demandée : dupliquer l'image comme dans Photoshop, appliquer un léger
flou gaussien à la copie, puis la superposer en Lighten ou Multiply. Le GUI est
exclu. Cette présentation remplace le shader de lumières ponctuelles v2.

L'image de base garde son échantillonnage nearest et ses contours nets. La copie
floutée contient toute la surface de jeu, personnages et décors compris, sans
seuil de luminosité ni masque d'émetteurs. Le noyau gaussien 9 × 9 a un sigma
de 1,25 pixel source. Ses poids suivent les coordonnées continues de l'écran :
le calque diffus est doux à la résolution de la fenêtre, pas découpé en blocs.

Le mélange utilise les valeurs RGB de présentation :

- **Lighten / Éclaircir** : `max(original, copie_floutée)` par canal.
- **Multiply / Produit** : `original × copie_floutée`.
- Résultat : interpolation entre l'image nette et ce mélange selon l'opacité.

Le préréglage interactif retenu est Lighten, opacité 75 %, luminosité 110 %.
La validation historique du calque seul utilise aussi une opacité de 35 %. F2 active/désactive le calque ;
F3 règle son opacité ; F6 bascule Lighten/Multiply. F4 reste la luminosité,
F5 la parallaxe, F7 la [profondeur discrète](DEPTH.md) avant le calque gaussien. Les préférences sont conservées dans Presentation.ini.

Les 32 lignes du HUD sont recopiées sans traitement et exclues des échantillons
utilisés par le flou. Les menus/pause natifs sont exclus par leur état de jeu.
Le calque est suspendu pendant les boîtes de message natives, dont l'index
seul peut rester mémorisé après fermeture : l'état du BG3 et de la coroutine
sert à détecter leur présence. L'aide Unreal est dessinée après le matériau. Les fondus restent synchronisés
avec le jeu.

« Pixels nets » décrit ici le calque de base. Le calque flouté ajoute volontairement
des variations à l'intérieur des blocs affichés. Avec F2 désactivé, l'image
originale et ses couleurs sont conservées exactement.

La détection des lampes et la séparation des fonds restent dans le projet pour
les travaux futurs. Leurs cartes de lumière ne participent pas à ce matériau.
L'ancien shader est conservé dans `Shaders/Archive/SourceLighting-v2.usf`.

## Vérification

`Scripts/test-pixels.sh` capture le même frame réel dans Unreal : original,
Lighten et Multiply. Le contrôle compare les deux effets à une convolution
et aux formules de mélange calculées indépendamment en Python ; il vérifie
également le HUD, les marges et la référence sans effet. Résultats dans
`pixel-verification.json`. Les captures sont dans `Captures/`.
