# Essai de relief par éclairage

F8 active/désactive un biseau sur les contours des structures BG1 qui touchent
un fond éloigné identifié. Les côtés supérieurs et gauches reçoivent un peu
plus de lumière ; les côtés inférieurs et droits sont ombrés.
La version accentuée monte jusqu’à +40 % sur le bord éclairé et −50 %
sur le bord ombré, avec une atténuation progressive vers la surface. La transition
s'étend sur six pixels source au maximum, sans agrandir les silhouettes.

Le relief est calculé dans `Native/sm_relief.h`, après la profondeur F7 et
avant le matériau gaussien. Le calque Lighten conserve sa formule et ses
réglages. F8 fonctionne indépendamment de F7 ; F2 coupe toute la présentation.
Les préférences F8 sont enregistrées avec les autres options de présentation.

Il s'agit d'un effet de volume par éclairage, avec une direction de lumière
fixe. Aucune géométrie 3D n'est créée. Les pixels de surface peuvent changer
de luminosité, mais leur position et leur échantillonnage entier sont conservés.
Les couleurs du dessin ne servent pas de carte de hauteur : pas de gaufrage
automatique de tous les détails ni de joints artificiels à chaque tuile.

Le traitement respecte les sprites OAM, le GUI, les fonds et l'alpha source.
Une silhouette de sprite ne crée pas de biseau dans le mur derrière elle.
Le flou global accepté continue naturellement de mélanger les couleurs
voisines dans son calque superposé, comme avant.

## Vérification

`Tests/check_relief.c` contrôle les faces éclairées/ombrées, les zones protégées
et l'absence de faux biseau autour d'un sprite devant un mur.

`Scripts/test-relief.sh` compare le même frame de Cérès dans Unreal avec F8
désactivé puis activé, profondeur F7 active et Lighten à 75 % / luminosité
110 %. `Scripts/test-relief.sh -SMTestSavedRoom` utilise une copie de la
sauvegarde de l'aperçu. Les captures et rapports sont dans `Docs/Relief/<salle>`.
Le test compare le résultat GPU à une référence gaussienne indépendante et
contrôle les pixels protégés avant ce calque.

La couverture dépend du masque de plans existant : une structure sans fond
éloigné identifié ne reçoit pas ce relief. La vérification du jeu entier et
des transitions reste à effectuer.
