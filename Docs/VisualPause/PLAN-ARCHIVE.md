# Plan de révision : raccords procéduraux, combat et pause

Statut : implémentation en cours le 14 septembre 2026 ; validation finale sur gaming-pc.
Ce document reprend le retour de test et la proposition de réutiliser les
tiles du même tileset pour prolonger les décors. Il ne marque aucun des
correctifs ci-dessous comme livré avant leur vérification.

Les raccords par métatiles, les quatre effets de combat et les trois pages
de pause sont codés. Les captures et rapports intermédiaires se trouvent dans
`Docs/VisualPause`. Ils ne couvrent pas encore tous les scénarios ci-dessous.
Pour préserver la fidélité, la page Samus réutilise le dessin de diagnostic
et la police extraits de la ROM, avec leurs variantes natives ; aucune
illustration générée ne remplace Samus.

## Direction conservée

Pixels et silhouettes fidèles, atmosphère sombre, calque gaussien Lighten
à 110 % de luminosité. Les nouveaux effets doivent rendre les actions plus
puissantes et lisibles. Les interfaces restent nettes. Les tests utilisent
la sauvegarde séparée avec tous les équipements.

## 1. Raccords procéduraux des décors et encadrements de portes

**Premier chantier.** Reproduire les deux captures et enregistrer les salles,
positions de caméra et états de portes correspondants. Leurs identifiants
restent à déterminer ; une ressemblance de tileset ne suffit pas.

Le code actuel coupe explicitement le rendu hors des dimensions de la salle
dans `Native/sm_wide.inc`. Il reconstruit aussi les côtés depuis les blocs de
salle. Ces deux chemins doivent être distingués lors du diagnostic.

- Identifier les données réellement manquantes : bord de salle, région
  découverte au-dessus de l'ancien HUD, tile non chargée, mauvais raccord
  de scroll, ou élément de porte partiellement affiché. Corriger d'abord
  les erreurs de rendu lorsque le contenu original existe.
- Construire un masque explicite des régions à prolonger. Un pixel noir
  n'est pas une preuve de trou : préserver les cavités, couloirs, ciel sombre,
  transparences et ouvertures dessinés dans la salle.
- Former une bibliothèque de motifs à partir du tileset actif, avec priorité
  aux motifs effectivement utilisés dans la salle. Travailler sur les blocs
  assemblés de 16 × 16 et leurs voisinages, pour préserver les structures
  composées de plusieurs tiles de 8 × 8.
- Classer les motifs de fond, remplissages de murs, sols, plafonds, angles et
  raccords. Exclure de la sélection décorative les portes, objets, passages
  secrets, plateformes, blocs destructibles et autres éléments interactifs.
- Prolonger les structures par des règles de voisinage : continuité des
  contours, matériau, palette, direction des tuyaux et poutres. Une petite
  variation déterministe peut éviter les répétitions trop visibles. Prévoir
  des règles par famille de tilesets et des exceptions par salle lorsque
  les seuls bords de pixels ne suffisent pas à identifier le bon motif.
- Ancrer la génération aux coordonnées du monde et à l'état de la salle,
  avec un aléatoire visuel indépendant de celui du jeu. Calculer et mettre
  en cache les raccords, puis les invalider sur les changements pertinents :
  arrivée dans une salle, transformation d'un bloc ou changement de décor.
  Conserver les références aux tiles/palettes pour suivre leurs animations.
- Pour une porte, traiter le mur et l'encadrement comme une structure
  complète : coins, montants, linteau, support. Suivre son orientation et
  son état ouvert/fermé ; conserver son ouverture, son animation et son
  emplacement natifs. Prolonger la paroi extérieure et son fond sans
  révéler artificiellement la salle suivante.
- Composer les extensions dans les couches visuelles appropriées afin que
  relief, lumière, brouillard et parallaxe restent cohérents. Garder les
  collisions, dégâts et limites de déplacement natifs. La jonction avec
  la limite jouable doit rester visuellement compréhensible.
- Lors d'un chargement, basculer entre les compositions cohérentes des deux
  salles. Ne pas générer depuis un mélange de données anciennes et nouvelles.
- Si aucun raccord valide n'est disponible, utiliser une terminaison de
  mur ou de fond définie pour ce tileset et signaler le cas au diagnostic.
  Aucun assemblage incohérent ne doit être accepté silencieusement.

**Validation :** captures avant/après des deux problèmes, caméra en mouvement
dans les deux sens, portes horizontales et verticales ouvertes/fermées,
retour dans une salle, blocs détruits, eau et animations. Vérifier l'absence
de scintillement, répétition de props interactifs, coutures et changement
de simulation. Un indicateur de diagnostic distinguera contenu natif et
extension procédurale pour examiner les raccords.

## 2. Power Bomb, grappin, Screw Attack et morts d'ennemis

Les effets existants se trouvent dans `SMVisualEffects.cpp` et
`Shaders/Present.usf`. `Native/sm_effects.c` expose déjà les projectiles,
événements de sprites, charge du canon et état de Power Bomb ; il manque
des signaux explicites pour le grappin et l'activité du Screw Attack.
Quelques événements d'explosion produisent déjà des particules : identifier
les morts d'ennemis qui échappent à ce chemin avant d'ajouter des doublons.

| Action | Rendu prévu | Déclenchement et contrôles |
| --- | --- | --- |
| Power Bomb | Concentration brève, flash initial nettement plus fort sur la scène, cœur incandescent, large front turbulent, rayons et particules expulsées, puis lumière résiduelle décroissante. | Phases calées sur la bombe native ; vérifier centre et bords d'écran, eau, obscurité, bombes successives et transition de salle. |
| Grappling Beam | Filament électrique continu avec petits arcs irréguliers, impulsions lumineuses, flash au départ du canon et étincelles au point d'accroche. | Position réelle du canon, extrémité et état du grappin ; suivre visée, extension, attache, balancement et rétraction. Masquer uniquement le dessin natif remplacé lorsque le nouveau rendu est prêt. |
| Screw Attack | Arcs autour de la silhouette, illumination pulsée et traînée courte de petites particules électriques. | Seulement pendant le Screw Attack réellement actif ; distinguer saut tournoyant, Space Jump et atterrissage. |
| Mort d'ennemi | Flash bref, particules radiales et débris lumineux qui retombent et s'éteignent ; intensité adaptée à l'explosion. | Événement de mort/explosion identifié, émis une seule fois ; une disparition hors champ ou un changement de salle ne doit pas déclencher une mort. |

Employer les coordonnées et événements natifs, conserver la logique de dégâts
et la trajectoire réelle des armes. Les particules et l'éclairage vivent dans
la présentation Unreal. Auditer le plafond actuel de 63 particules et la
texture de 64 entrées : dimensionner les lots et priorités selon les mesures,
pour qu'une Power Bomb ne supprime pas tous les autres effets importants.

Prévoir dans les options l'intensité des effets, le niveau du flash et une
réduction des flashes ; le HUD et les menus ne reçoivent pas le flash global.
Le réglage normal de la Power Bomb doit être sensiblement plus puissant que
la version actuellement montrée.

**Validation :** vidéos comparatives des quatre actions, simultanéité des
effets, absence de particules conservées dans la salle suivante, lisibilité
de Samus et mesures de temps de trame dans Unreal. La réussite demande un
rendu visible, pas seulement un compteur d'événements.

## 3. Pause widescreen et carte façon visière

Construire une interface Unreal adaptée à toute la largeur utile, alimentée
par les données natives de carte et d'inventaire. La page actuelle est une
image centrale de 256 pixels ; l'agrandir par étirement ne répondrait pas au
besoin.

- Trois onglets en boucle avec L/R : **Système — Carte — Samus**. L'écran
  Carte reste l'entrée par défaut ; L y ouvre directement Système et R Samus.
- Carte réellement plus grande, davantage de cases visibles, déplacement
  fluide, zoom par paliers nets et recentrage sur Samus. Préserver la
  distinction entre exploré, révélé par une station et inconnu.
- Habillage de visière : cadres techniques, léger balayage lumineux,
  indicateur de position pulsé, transitions discrètes. Les tracés de carte,
  icônes et légendes conservent un contraste stable et restent nets.
- Horloge propre à l'interface pour animer la pause pendant que la simulation
  du monde reste arrêtée. Gérer clairement les entrées, l'audio et la reprise
  afin d'éviter un tir ou un saut involontaire à la fermeture.
- Prévoir clavier et manette, légendes cohérentes et mise en page pour les
  tailles de fenêtre prises en charge, y compris le retour à la vue native.

**Validation :** carte peu explorée et remplie, limites de chaque zone,
navigation L/R, zoom, déplacement, fermeture/réouverture et vérification que
le monde n'avance pas pendant la pause. Aucune information de seed inconnue
ne doit être révélée par l'interface.

## 4. Page Samus : ordinateur de diagnostic animé

- Composition widescreen : Samus au centre, catégories d'équipement sur
  les côtés, description de l'élément sélectionné et ressources lisibles.
- Fond de diagnostic animé avec une grille et un balayage retenus ; points
  lumineux reliés au canon, torse, jambes et modules concernés. Le survol ou
  la sélection anime le point associé et sa liaison.
- Préparer une nouvelle représentation de Samus pour le menu : silhouette,
  proportions du casque, épaulières, canon et couleurs reconnaissables de
  Super Metroid ; contours, palette et résolution délibérément maîtrisés.
  Une aide IA peut servir à explorer une base, suivie d'une reprise pixel
  par pixel. Les détails incohérents ou le rendu lisse générique ne sont
  pas des critères acceptables pour l'asset final.
- Garder une source éditable avec calques pour le corps, les repères et les
  animations. Représenter correctement les variantes de combinaison.
- Brancher activation/désactivation et transfert des réserves aux règles
  natives, notamment l'exclusion Spazer/Plasma. Le dessin de diagnostic doit
  refléter l'équipement effectivement actif.

**Validation :** comparer les variantes visuellement à la référence native,
essayer inventaire vide/partiel/complet, chaque sélection, les combinaisons
de rayons et les réserves AUTO/MANUAL. Vérifier le résultat en jeu à la sortie.

## 5. Page Système

- **Options** : affichage, Lighten/Multiply, luminosité, profondeur, relief,
  météo, nouveaux effets/flash, commandes et wall jump assisté/original.
  Décrire les réglages et offrir un retour aux valeurs par défaut.
- **Reset game** : retour au titre sans effacer la sauvegarde, avec
  confirmation de la perte éventuelle de progression non sauvegardée.
  Réinitialiser proprement cœur natif, audio et effets ; conserver le profil
  et l'identité de la seed d'une partie randomisée.
- **Achievements** : succès locaux avec description et progression,
  alimentés par des événements vérifiables. Enregistrer dans le profil
  associé ; isoler la sauvegarde cheatée des succès de la partie normale.
  Préserver les secrets tant qu'ils ne sont pas découverts.
- **Exit game** : fermeture propre avec confirmation si une progression
  non sauvegardée serait perdue. Ne pas présenter l'écriture du SRAM comme
  une sauvegarde instantanée de la position actuelle.
- Centraliser la persistance des options et profils en préparant la
  compatibilité avec le randomizer déjà intégré et ses patches optionnels.

**Validation :** redémarrage et persistance, annulation et confirmation,
retour au titre puis chargement, succès déclenchés une seule fois, séparation
des profils vanilla/randomizer/test et sortie propre du processus.

## Ordre de livraison et preuves

1. Reproduction des coupures et prototype de raccords sur les deux salles.
2. Règles de raccord et traitement complet des portes ; tests de mouvement.
3. Renforcement des quatre effets de combat, captures et vidéos comparatives.
4. Infrastructure de pause, navigation des trois onglets et nouvelle carte.
5. Page Samus et illustration de diagnostic finalisée.
6. Options, reset, succès locaux et sortie dans la page Système.
7. Parcours de validation avec la sauvegarde dédiée et une sauvegarde normale
   séparée : jeu, pause, modification d'équipement, reprise, porte, objet,
   mort et rechargement. Rejouer les contrôles HUD existants et mesurer les
   scènes chargées. Conserver captures, vidéos, rapports et liste des salles
   encore à traiter.

Développement et compilation en local. À la demande de l'utilisatrice,
aucun nouveau test ni lancement de jeu local sans demande explicite.
Les vérifications autonomes utilisent gaming-pc (qui remplace print-server),
dans `~/Games/SuperMetroid-VisualValidation`, avec des sauvegardes de test
isolées. Cette copie se distingue de l'installation jouable habituelle.
