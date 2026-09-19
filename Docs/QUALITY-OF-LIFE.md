# Quality of Life et ajouts propres à notre port Super Metroid

État audité le **14 septembre 2026** dans le code local. Cet inventaire distingue les fonctions utilisables, les adaptations de présentation, les outils et les fonctions incomplètes. Les rapports liés sont des preuves déjà enregistrées ; aucun nouveau test ni lancement de jeu n'a été réalisé pour le rédiger.

Le [dossier randomizer et UI](References/SMRandomizerUI/README.md) complète cette liste avec les options VARIA d'origine. Une option seulement disponible upstream n'est pas un ajout réalisé par nous.

**Mise à jour menu :** le [menu système Dear ImGui](SYSTEM-MENU.md) est maintenant codé et compilé. Les ajouts ci-dessous qui le concernent n’ont pas encore été validés visuellement ou en interaction.

## 1. Déplacements et confort de jeu

| Ajout réalisé | Utilisation | Comportement et limites |
| --- | --- | --- |
| Wall jump assisté | **F12**, actif par défaut, préférence persistante | Détecte le mur sans exiger le timing/direction d'origine ; nouvel appui mémorisé 9 images, contact conservé 7 images, impulsion orientée à l'opposé. Temporisation et garde de distance évitent les déclenchements parasites. Ne transforme pas le maintien du bouton en sauts automatiques. Mode original conservé. |
| Space Jump assisté | **Maj+F12**, actif par défaut, indépendant du wall jump | Mémorise un nouvel appui pendant 16 images (~267 ms), le consomme au début de la descente et accepte les appuis tardifs. Space Jump équipé, vrille et restrictions des liquides restent requis. Aucun vol automatique au maintien. Mode original conservé ; Ctrl+F12 existe aussi comme alias dans le code. |
| Clavier et manette | Flèches / contrôles manette ; Z saut, X course, S tir, C annulation, A/D visée, Maj droite sélection, Entrée Start | Remapping clavier/manette et deadzone dans Settings → Input du nouveau menu système. La pause native reste inchangée. Les presets VARIA ne pilotent pas ces commandes. |
| Pause externe | **P** | Fige simulation et audio. La pause normale **Entrée/Start** conserve le fonctionnement et les animations du menu SM. |
| Aide immédiate | **F1** | Affiche les commandes ; les changements graphiques affichent également une notification. Overlay Canvas du port. |
| Plein écran et format | **F11** ; **Ctrl+F11** large/natif | Bascule de présentation sans changer la logique de la salle. Le mode natif utilise la surface d'origine 256×224, le mode large 400×224. |
| Menu système / quitter | **Échap / F4 / clic stick droit** ; **System → Exit game** | Ouvre le menu ImGui. Confirmation avant de quitter une partie ; le SRAM est vidé sur disque, sans sauvegarde instantanée. |

Sources : [wall jump](../Native/sm_walljump.c), [Space Jump](../Native/sm_spacejump.c), [adaptation des routines](../Native/prepare_overlays.py), [raccourcis et préférences](../Unreal/Source/SMUnreal/SMHUD.cpp). La [validation Space Jump existante](Movement/README.md) rapporte 1 440 images identiques au comportement précédent lorsque l'aide est coupée, et des séquences assistées plus permissives. Cela ne vaut pas validation de toutes les situations du jeu.

## 2. Lisibilité du HUD, de la carte et des menus

| Ajout / correctif | État actuel |
| --- | --- |
| Surface panoramique native | 400×224, sprites à pixels carrés, agrandissement entier ; marges possibles selon la fenêtre |
| HUD sans bande noire | Décor visible derrière les éléments du HUD ; icônes et textes séparés du traitement graphique |
| Réparation des éléments coupés | Séparation des groupes à x=80, conservation d'AUTO/réserves, des cinq icônes et des chiffres complets de munitions |
| Minimap agrandie | 7×4 cellules : deux colonnes à gauche et une ligne en bas, en conservant l'exploration et le repère de Samus |
| Pause widescreen authentique | Carte existante et assets originaux ; grille prolongée, panneaux déplacés, zone de carte élargie, titres/flèches adaptés |
| Protection du GUI | Overlay traité avant les effets ; HUD, textes et messages identifiés conservent leurs couleurs et contours |
| Stabilité de présentation | Classification des scènes, messages, prises d'objets et transitions pour éviter les changements arbitraires de surface/traitement |
| Correction du repli des sprites | Coordonnées complètes conservées pour le rendu large ; évite certaines duplications sur le bord opposé |

Sources et captures inspectées : [UI du port](References/SMRandomizerUI/04-ui-du-port.md), [pause native](NativePause/README.md), [corrections d'affichage](DisplayFixes/README.md). Le Map Overhaul complet reste non porté. Le [HUD VARIA natif](Randomizer/NativeUI/README.md), les munitions maximales et les réserves améliorées sont désormais intégrés aux slots Randomized.

## 3. Préférences persistantes et sauvegardes

Le profil normal est `Unreal/Saved/SM/Presentation.ini`. Les aperçus HUD ont leur propre profil. Les valeurs de départ ci-dessous sont celles d'un lancement normal sans réglages préexistants ; les chemins d'autotest ont des défauts spécifiques.

| Section / clé | Défaut normal et accès |
| --- | --- |
| `Controls.AssistedWallJump` | true ; F12 |
| `Controls.AssistedSpaceJump` | true ; Maj+F12 |
| `Display.Widescreen` | true ; Ctrl+F11 |
| `Atmosphere.Enabled` | true ; F2, interrupteur principal de présentation incluant météo/effets de combat |
| `Atmosphere.Intensity` | 0,75 ; F3, paliers 0,25 / 0,50 / 0,75 / 1 ; plage INI 0…1 |
| `Atmosphere.Brightness` | 1,10 ; Settings → Graphics → Scene brightness ; plage 0,75…1,25 (F4 ouvre désormais le menu) |
| `Atmosphere.BlendMode` | 0 Lighten ; F6 pour 1 Multiply |
| `Atmosphere.Parallax` | true ; F5 |
| `Atmosphere.Depth` | true ; F7 |
| `Atmosphere.Relief` | true ; F8 |
| `Atmosphere.EngineWeather` | true ; F9 ; dépend aussi du master F2 |
| `Atmosphere.FlashStrength` | 1, plage 0…1 ; Settings → Quality of Life → Combat flash strength |
| `Atmosphere.Version` | 3 ; migration des anciens réglages vers opacité 75 % / luminosité 110 % |
| `Display.BorderExtension` | Ancienne option écrite false ; remplissage automatique retiré, valeur non restaurée |

Le souhait « Lighten à 110 % » correspond aujourd'hui à **Lighten, luminosité 110 %, opacité du calque 75 %**. Les deux grandeurs sont séparées.

Le port écrit la SRAM avec fichier temporaire puis renommage, périodiquement toutes les 600 images natives et à la fermeture. **Cela sauvegarde le contenu de SRAM déjà produit par le jeu ; ce n'est pas une sauvegarde automatique de la position courante.** Les stations de sauvegarde restent pertinentes. Ce mécanisme n'est pas non plus le système de slots de secours verrouillables de VARIA.

Sources : [chargement/persistance des réglages](../Unreal/Source/SMUnreal/SMHUD.cpp), [écriture SRAM](../Native/sm_bridge.c).

## 4. Présentation et retours visuels ajoutés

Ces fonctions améliorent l'ambiance et la lecture des actions. Elles sont séparées ici du QoL mécanique pour ne pas présenter un shader comme une modification de gameplay.

| Ajout | Fonctionnement actuel / limite |
| --- | --- |
| Calque gaussien global | Copie floutée de toute la scène hors GUI, filtre 9×9, sigma 1,25, fusion Lighten ou Multiply. Base nette conservée ; pas uniquement un halo autour des lampes. |
| Profondeur discrète et parallaxe | Décalage de fonds identifiés et séparation de plans, sprites plats. Ceres et Crateria ont des références vérifiées ; d'autres BG2 restent expérimentaux. |
| Relief des structures | Bords éclairés/ombrés et biseau accentué calculés pour le décor ; réversible, sans nouvelle géométrie 3D. |
| Pluie | Quatre couches dans le shader actuel, pilotées par les effets natifs de salle. Les anciennes notes décrivant trois couches sont historiques. |
| Fog | Brume continue par bruit/voiles pour éviter les carrés de la couche SNES ; activation selon le type d'effet de la salle. |
| Eau | Caustiques, rayons et bulles selon la surface et l'immersion réelles ; applicable notamment à Maridia. |
| Chaleur | Distorsion de l'arrière-plan dans les salles identifiées chaudes/lave ; masque de protection des silhouettes et du terrain. Pas activée indistinctement sur tout Norfair. |
| Pas contextuels | Poussière, éclaboussures, limon/bulles sous l'eau, sable de Maridia, cendres des salles chaudes, débris gris des zones métalliques. Déclenchés par les pas natifs au sol, pas à chaque frame ; pas de pas en Morph ou dans les airs. |
| Tir du canon | Flash de bouche et éclairage local associé aux projectiles |
| Charge Beam | Halo croissant suivant la charge réelle |
| Explosions et mort d'ennemis | Éclat lumineux et particules depuis les événements du cœur natif |
| Grapple Beam | Arcs électriques bleu/blanc le long du rayon, éclat au canon et particules à la pointe |
| Screw Attack | Arcs dorés, luminosité et petites particules autour de Samus |
| Power Bomb | Détonation claire, front turbulent, rayons, lueur résiduelle et distorsion du fond ; remplace visuellement l'ellipse native lorsque les effets sont actifs. L'effet actuel est chaud/blanc-jaune ; l'ancienne description bleue n'est plus la référence. |

Les pas utilisent une classification d'environnement en six styles, **pas une annotation de matériau pour chaque tile**. Les particules du port sont gérées par un système C++ limité puis dessinées par shader ; ce n'est pas une simulation de fluides ou une scène Niagara complète. Leur RNG est séparé de celui du gameplay.

`FlashStrength` atténue le terme de flash de détonation de la Power Bomb. Il ne supprime pas tous les flashes du jeu ou tous les autres termes lumineux ; ne pas le présenter comme l'équivalent du patch d'accessibilité `noflashing`.

Sources : [shader](../Shaders/Present.usf), [effets C++](../Unreal/Source/SMUnreal/SMVisualEffects.cpp), [pas](Footsteps/README.md), [météo](WEATHER.md), [profondeur](DEPTH.md), [relief](RELIEF.md), [calque](GAUSSIAN-LAYER.md). Le vieil audit des sources lumineuses reste une base d'assets ; son ancien pipeline de lumière n'est pas la description du rendu final actuel.

## 5. Outils et confort de développement

| Outil ajouté | Accès / rôle | Séparation et limite |
| --- | --- | --- |
| Téléportation | F10, Haut/Bas ou D-pad, Entrée/Z ou bouton de confirmation ; huit lieux de Crateria, Brinstar, Norfair, Maridia et Wrecked Ship | Outil de visite/test, conserve inventaire/événements ; pas d'invincibilité ni de sauvegarde automatique de position. Ouverture en gameplay hors message. |
| Sauvegarde tous équipements | `Lancer-Super-Metroid-Test-HUD.sh` | Fixture avec tous les items, 1499 énergie, 400 réserves et munitions ; copie sous `SMTests/HUDPreview`, séparée de la partie normale. Plasma/Spazer respectent leur exclusion d'équipement. |
| Aperçu d'ambiance | `Scripts/preview-lighting.sh` | SRAM dans `SMPreview`, séparée de la partie normale |
| Éditeur externe de décors | `Lancer-Editeur-Decors.sh` ; navigateur et serveur local dédiés | Non inclus dans le jeu ; utilise les tiles/palettes du jeu, aucune génération d'assets |
| Peinture fidèle par salle | Sélection salle/état/tileset, deux couches, retournements X/Y, pinceau/gomme/pipette | Retouches intérieures et huit blocs de prolongement autour de la salle ; ne modifie pas collisions, BTS ou PLM |
| Édition réversible | Annuler/rétablir, sauvegarde, historique, vue original et overlay des collisions | JSON sous `Config/RoomDecorations`, garde sur le bloc original pour les portes et blocs modifiés en jeu |
| Rechargement de décor | Ctrl+F8 dans le jeu | Charge les retouches sans imposer un redémarrage ; ne réactive pas l'ancien remplissage automatique |
| Validation isolée du randomizer | Génération dans le processus natif, progression, vérification indépendante des placements, staging et SRAM par seed | Réglages dans ImGui → nouvelle partie → Generate Game dans le menu natif ; Start Game verrouillé jusqu’au succès. Trois seeds vérifiées sans fenêtre sur gaming-pc. |

L'éditeur recense 255 salles / 310 états ; six salles de Ceres sont listées mais exclues du dessin à cause du Mode 7. Cela ne signifie pas que les prolongements de 255 salles ont déjà été peints. Guides : [éditeur](RoomEditor/README.md), [téléportation](TELEPORTATION.md), [intégration randomizer](References/SMRandomizerUI/03-integration-native.md).

## 6. Présent en code, retiré ou encore à faire

| Élément | État exact |
| --- | --- |
| Achievements locaux | Suivi encore appelé en gameplay et enregistré dans `Achievements.ini` : équipement obtenu, 10 ennemis, Power Bomb, Grapple, Screw Attack. Pas de panneau actuel, pas d'intégration Steam. |
| Reset vers titre | System → Reset to title la déclenche avec confirmation ; conserve désormais aussi le Space Jump assisté. |
| Menu Système / Options | Le popup Canvas rejeté est remplacé par une nouvelle interface système ImGui inspirée de CoC : graphismes, audio, commandes, QoL, profils et randomizer. Compilation validée, essai interactif restant à faire. |
| Nouvelle Samus de pause | Aucun remplacement généré accepté ; silhouette et assets d'origine dans la pause actuelle. |
| Complétion procédurale des trous | Retirée du rendu actif. La peinture externe est la direction actuelle, et le contenu reste à produire. |
| UI native du randomizer et Customizer | Réglages classés dans ImGui, génération déclenchée par le menu natif, slots A/B/C Vanilla/Randomized indépendants, sauvegarde initiale après génération et données préparées pour les trackers. Les options HTML non portées sont indiquées indisponibles ; le Customizer reste non intégré. |
| Patches VARIA de confort | HUD VARIA, max ammo, réserves améliorées et marqueurs de collection : natifs, optionnels, actifs par défaut en Randomized. 10 entrées `pending-native` restantes : spin restart, refill, portes/ascenseurs rapides, visée libre, secousses/flashes/couleurs, sons courts et MSU-1. |
| Infinite Space Jump upstream | Non porté comme tel ; notre Space Jump tolérant conserve des règles plus proches de l'original. |
| Map Overhaul et sauvegardes de secours VARIA | Étudiés, non intégrés au port natif. |

La liste des patches et leur portée vanilla/randomizer se trouve dans [l'intégration native](References/SMRandomizerUI/03-integration-native.md). Les comportements natifs requis pour les seeds (Zebes, Morph, portes, identité des emplacements) y sont séparés des préférences utilisateur.

## État de validation

Cet audit a relu le code, les documents et des captures existantes. Les vérifications historiques couvrent notamment HUD/messages, pause large, Space Jump, pas et génération/solveur multi-seed. **Ni une partie complète du jeu, ni toutes les salles, ni toutes les options VARIA ne sont déclarées validées par cette documentation.** Les références ci-dessus permettent de retrouver la portée réelle de chaque preuve.

La gestion des [slots indépendants](Randomizer/SaveSlots/README.md) ajoute le choix de mode natif, le badge original, la sauvegarde après génération et la conservation de la seed lors des copies.

Pour les parties Randomized, les [cartes originales complètes](Randomizer/FullMaps/README.md), passages secrets compris, sont maintenant visibles dès le départ. L’exploration réelle reste distincte pour les sauvegardes et trackers.

Les [trackers intégrés](Tracker/README.md) ajoutent les carrés de checks à la carte native et à la minimap, ainsi que les icônes et capacités acquises au bandeau de la carte. Actifs par défaut en Randomized, ils peuvent être activés en Vanilla dans Settings → Interface. La logique suit chaque seed et les flags du slot ; aucune installation de PopTracker ni aucun pack externe n’est nécessaire.

Les [patches UI VARIA natifs](Randomizer/NativeUI/README.md) sont disponibles sur les anciennes et nouvelles seeds ; préférences dans Settings → Interface, sans effet sur Vanilla.

Le [générique The Zebes Project](Credits/README.md) ajoute le suivi des statistiques par slot, le décor original de Zebes avec effets Unreal et Debug → Launch Credit Roll. La prévisualisation revient à la session intacte.


## Refill at save stations — 2026-09-15

`Settings → Quality of Life → Refill energy and ammo when saving` is optional and disabled by default. Accepting the native save-station prompt refills energy, reserve energy, Missiles, Super Missiles and Power Bombs to existing capacities before saving. Cancelling does not refill. Ordinary SRAM flushes, switching profiles and publishing generated seeds do not heal Samus. It works in Vanilla and Randomized games.

The complete seed editor also carries `refill_before_save` per randomized slot. Global comfort enablement remains available independently. See the [separate release](../Releases/2026-09-15-save-refill/README.md) and [configurator checkpoint](Randomizer/FullOptions/MenuEditor/README.md).

## Future optional QoL — requested September 19, 2026

See [Planned optional gameplay QoL](PLANNED-OPTIONAL-QOL.md). These requested
additions apply to Vanilla, Vanilla New Game+ and Randomizer, are excluded from
speedrun mode, and are not presented as implemented features of this release.
