# Éditeur externe de décors

Lancement : `./Lancer-Editeur-Decors.sh` depuis la racine du projet.
L'outil ouvre une page locale dans le navigateur. Il est indépendant d'Unreal,
ne lance pas le jeu et n'est pas inclus dans son paquet.

## Peindre

1. Choisir une zone, une salle et son état natif (normal, événement, boss…).
2. Choisir une tile dans la palette. Le filtre montre d'abord les tiles déjà
   employées dans la salle ; le désactiver expose son tileset complet.
3. Peindre dans la salle ou dans la marge extérieure, sur le décor avant ou
   arrière. Les miroirs X/Y utilisent les bits de retournement natifs.
4. Enregistrer, puis utiliser **Ctrl+F8 dans le jeu** pour recharger les retouches.

Pinceau **B**, gomme **E**, pipette **I**, annuler **Ctrl+Z**, rétablir
**Ctrl+Maj+Z**, enregistrer **Ctrl+S**. Un clic droit retire une retouche.
La gomme restitue le décor original ; elle ne détruit pas un bloc du jeu.
Le mode « Original seul » permet la comparaison sans effacer le travail.

Les collisions s'affichent en lecture seule : type 8 solide en rose, type 1
pente en jaune, autres comportements en orange. Ce sont les catégories natives
des blocs, pas un dessin précis des sous-pixels des pentes.
Le contour cyan marque les dimensions de la salle. La marge peignable actuelle
fait huit blocs de chaque côté.

## Fidélité et stockage

Les tiles, les définitions de métatiles, les palettes et les calques de salle
sont décompressés directement depuis la ROM Japan/USA. La liste des salles et
le décompresseur proviennent du Randomizer déjà présent dans le dépôt ; aucun
asset n'est généré par IA. Les données compressées contiennent parfois des
rangées supplémentaires inutilisées ; elles ne sont pas prises pour des
dimensions de salle.

Les fichiers sont dans `Config/RoomDecorations/<salle>-<état>.json`. Ils stockent
des coordonnées et références aux tiles du tileset actif. Chaque enregistrement
conserve la version précédente sous `.history/`. La ROM, le SRAM, les collisions,
les BTS, les PLM et les objets ne sont pas modifiés.

Le jeu ne charge que les fichiers correspondant à l'état et au tileset actuels.
Une peinture intérieure est ignorée si son bloc natif a changé : une porte ouverte
ou un bloc détruit ne reste donc pas masqué par sa retouche initiale.
Le jeu applique les retouches dans sa copie de présentation, y compris en 4:3.

## Vérifications et limites

- [Lecture et cœur natif](verification.json) : 255 salles prises en charge,
  310 états, comparaison des définitions de tiles utilisées, 180 images de
  simulation et de rendu natif identiques pour chaque calque avec/sans peinture,
  retouches visibles en widescreen et 4:3, changement de bloc respecté et demandes
  d'édition de collisions refusées. Les différences visibles sont mesurées sur
  les canaux RGB, en excluant l'alpha.
- [Parcours réel dans le navigateur](browser-verification.json) : choix de zone,
  palette, peinture intérieure/extérieure, arrière-plan retourné, gomme,
  annuler/rétablir, enregistrement et relecture après rechargement.
- [Capture de l'outil](editor-browser.png), avec les guides de collision activés.
  Les tiles placées dans ce test servent à valider l'outil, pas à proposer un décor final.
- [Chargement Unreal](runtime-verification.json) : quatre retouches chargées depuis
  un fichier séparé, puis parcours de combat sur gaming-pc avec reprise normale
  du jeu. Ce contrôle ne valide pas la qualité artistique des tiles de test.

Les six salles de Ceres sont répertoriées mais exclues de l'éditeur de métatiles :
leurs décors Mode 7 nécessitent un autre traitement. L'aperçu utilise les calques
de salle et la palette de base ; il ne reproduit pas encore les fonds spéciaux
chargés par script, les animations de palette, les ennemis et les effets Unreal.
Les retouches de fond restent disponibles et le jeu les compose sur son fond natif.
Il faut donc examiner le résultat en jeu avant de considérer une salle comme finie.

L'éditeur ne propose pas encore de bibliothèque de motifs assemblés, de sélection
rectangulaire ni de navigation sur un atlas géographique de toute la planète.
