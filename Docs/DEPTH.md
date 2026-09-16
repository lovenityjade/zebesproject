# Profondeur discrète, caméra de profil

Direction retenue : sprites plats, dessin original, décors répartis en plans.
Le relief est une composition de couches et de petites ombres, sans rotation
de caméra ni reconstruction des sprites en modèles 3D.

Le calque gaussien validé reste au-dessus du décor composé. Le préréglage
interactif conserve Lighten, opacité 75 %, luminosité 110 %. F4 propose
85 / 100 / 110 / 115 %. F7 active/désactive la profondeur ; F5 règle la parallaxe.
Le GUI reste hors du traitement.

F8 ajoute un [biseau éclairé sur les structures](RELIEF.md), indépendamment
de la profondeur des fonds décrite ici.

## Plans

- **Cérès, salle df45** : le damier derrière le châssis est un fond éloigné.
  Une atténuation légère et des ombres de contact le font paraître en retrait.
  Les ombres des structures et des sprites sont projetées uniquement sur ce
  plan. Les bords métalliques et les sprites source sont conservés.
- **Crateria, zone d'atterrissage 91f8** : ciel, montagnes et végétation ont
  trois réponses au mouvement de caméra. Les limites verticales 1168 / 1192
  proviennent de la table de défilement native 88:AEC1, utilisée dans
  `HdmaobjPreInstr_SkyLandBG2XscrollInner`. Le vent et les bandes HDMA d'origine
  sont conservés ; aucun décalage vertical supplémentaire ne les désaligne.
  Le vaisseau ne projette pas une ombre artificielle sur le ciel lointain.
- **Autres salles Mode 1 éligibles** : la couche BG2 indépendante peut recevoir
  le traitement intérieur. Cette couverture générique reste à contrôler
  salle par salle, en particulier autour des boss et des transitions.

Le décalage supplémentaire reste entier et borné (±8 horizontal, ±6 vertical),
avec une petite réaction à la position de Samus dans l'écran. Le plan de jeu
conserve ses positions. Les registres PPU temporaires sont restaurés après
composition et la logique native ne reçoit pas ces décalages.

`Native/sm_depth.h` compose les ombres à partir des masques de plans et de la
provenance des pixels. Il copie l'image source puis ne modifie que les texels
du fond identifié. Le matériau Unreal lit cette image, puis applique exactement
le même flou gaussien et le même mélange Lighten/Multiply qu'avant.

## Validation

`Scripts/test-depth.sh` compare deux captures du même frame à Cérès.
`Scripts/test-depth.sh -SMTestSavedRoom` fait de même sur une copie de la
sauvegarde de l'aperçu. Le test contrôle le calque de base, le GUI et la
composition gaussienne à partir d'une référence numérique indépendante.
Captures et rapports : `Depth/Ceres/` et `Depth/Crateria/`.

`Tests/check_depth.c` vérifie les ombres intérieures, l'absence d'ombre de
sprite sur le ciel et la conservation du premier plan. `Tests/check_scene.c`
vérifie que la parallaxe ne modifie ni la référence, ni les registres, ni la
VRAM. `Tests/audit_saved_room.py` vérifie aussi les pixels protégés dans la
vraie salle de Crateria.

`Scripts/test-depth-motion.sh` enregistre un déplacement réel dans Unreal,
sur une copie de sauvegarde, puis produit une vidéo de 60 images. C'est une
capture de validation, pas une mesure de performance ni un parcours complet.

## Sauvegardes

Le jeu normal utilise `Saved/SM/sram.dat`, l'aperçu `Saved/SMPreview/sram.dat`.
La sauvegarde de l'ancien aperçu a été copiée sans supprimer sa source.
Chaque validation utilise désormais un fichier unique `validation-*.sram`,
afin qu'une session de jeu ne transforme pas un test de Cérès en test du menu
pause à Crateria. Le lancement rapide s'arrête dès l'arrivée au gameplay pour
éviter d'envoyer ensuite des commandes Start dans la partie sauvegardée.

Les préférences de présentation créent leur dossier avant enregistrement.
Le vrai widescreen et la validation du jeu complet restent des travaux séparés.
