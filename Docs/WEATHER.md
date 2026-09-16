# Météo et ambiance dans Unreal

F9 active/désactive la météo Unreal. F10 permet de rejoindre les zones d'essai.
Le matériau GPU anime les effets à la résolution d'affichage, au-dessus de
la présentation pixel art. Le calque gaussien Lighten conserve sa formule et
ses réglages ; le GUI est exclu. La pause arrête le temps des effets.

- Pluie : trois plans de traînées inclinées, vitesses et opacités distinctes,
  avec une brume de pluie douce. La couche BG3 de pluie d'origine est retirée
  uniquement de l'image de présentation pour éviter de superposer deux pluies.
- Brouillard : deux champs de bruit lents et une densité plus forte dans les
  plans éloignés et vers le bas. La couche BG3 de brouillard est remplacée.
- Eau : teinte de profondeur, caustiques stylisées, rais de lumière et petites
  particules ascendantes, limités aux pixels sous le niveau d'eau réel.
- Chaleur : ondulations légères du fond et voile chaud. La silhouette de Samus
  et les structures restent stables grâce au masque des couches.

Ce sont des effets procéduraux dans le matériau Unreal, pas une simulation
physique de fluide ou un volume de brouillard 3D. L'objectif est un mouvement
plus fluide et une ambiance stylisée cohérente avec le pixel art.

## Déclenchement natif

`sm_fx_type` reprend le type FX actif : 0x0A pour la pluie, 0x0C pour le
brouillard, 0x2C pour une brume légère. `sm_water_y` reprend la hauteur animée
`fx_y_pos`, y compris les marées, pour l'eau active ; les volumes désactivés
par `fx_liquid_options & 4` sont exclus. `sm_heated_room` détecte le pré-traitement
de palette `PalPreInstr_SamusInHeat` (8D:E379), indépendamment de l'armure,
ou un volume de lave actif. Une zone entière n'est pas présumée chaude ou noyée.

Le retrait de BG3 se limite aux lignes de gameplay en Mode 1, avec les
registres restaurés après chaque ligne. Il est désactivé lorsqu'une boîte de
message utilise BG3. L'image de référence du port natif reste intacte.

## Validation

`Scripts/test-scene.sh` vérifie notamment le retrait de pluie dans la seule
présentation, la conservation du GUI et des registres, et la protection de BG3
lors d'un message.

`Scripts/test-weather.sh` capture un frame natif gelé dans Unreal : référence,
pluie à deux instants, brouillard, eau avec surface à y=100, chaleur, sélection
native et retour aux pixels d'origine hors gameplay. Les modes forcés servent
à isoler les effets ; la capture `native` suit exclusivement les indicateurs
de la vraie salle. Le test vérifie aussi la référence gaussienne et le GUI.

`Scripts/test-weather.sh -SMTestTeleport=6` effectue le même essai à Maridia Est ;
`-SMTestTeleport=3` rejoint les cavernes chaudes de Norfair. Chaque exécution
utilise une copie de sauvegarde. Captures et rapports : `Weather/<salle>/`.
La traversée de toutes les salles et des transitions reste à vérifier.

Captures Vulkan vérifiées dans les salles 91F8 (pluie), D48E (eau) et A923
(chaleur), après chargement natif. La référence gaussienne présente un écart
maximal de 2 à 4 niveaux sur 255 selon la palette ; le maximum de 4 à Norfair
concerne un canal sur plus de 1,5 million. L'erreur moyenne reste inférieure
à 0,15 niveau par canal. Le GUI et le retour hors gameplay sont comparés
sans tolérance et restent identiques.
