# Révision de l'affichage — 14 septembre 2026

Cette révision concerne la version de développement **locale**. Le paquet
autonome livré précédemment sur gaming-pc reste une version antérieure.

## HUD et sauvegarde de test

La coupure entre énergie et armes était placée au pixel 96, au milieu de
l'icône des missiles et de son compteur. Elle est maintenant au pixel 80,
après les réserves AUTO. Les cinq groupes d'armes restent entiers, ainsi
que les quatorze réservoirs d'énergie. La minimap conserve ses 7 × 4 cases.
Les mêmes ancrages sont utilisés dans la carte et l'écran d'équipement.
La superposition passe désormais par le même chemin de couleur que l'image
native : l'ancien passage Canvas éclaircissait une seconde fois les couleurs
du HUD. Les textes et icônes gardent donc aussi leurs teintes d'origine.

Pour tester avec tous les équipements :

```sh
./Lancer-Super-Metroid-Test-HUD.sh
```

La sauvegarde contient 1499 d'énergie, 400 de réserve, 230 missiles,
50 super missiles et 50 Power Bombs. Tous les rayons sont collectés ;
Plasma est équipé et Spazer est disponible dans l'inventaire, car le jeu
ne permet pas de les équiper simultanément. **Entrée** ouvre la pause,
**D** correspond à R pour afficher l'équipement. F10 ouvre les téléportations.

La fixture `Unreal/Saved/SMTests/HUD-all-items.sram` possède les sommes de
contrôle produites par le jeu. Le lanceur en crée une copie indépendante dans
`Unreal/Saved/SMTests/HUDPreview/sram.dat`, avec ses propres réglages.
La sauvegarde normale et ses réglages ne sont pas remplacés.

## Messages, cinématiques et transitions

Le canevas reste à 400 × 224 pendant les chargements, les cinématiques et
les messages. Il ne change plus d'échelle quand Samus prend la Morph Ball.
Les illustrations des cinématiques reçoivent le calque gaussien ; leurs
textes, les messages d'objets et le compte à rebours sont composés après
les effets à partir des pixels natifs. Les menus restent nets.

Les cinématiques et les pages de pause conservent leur composition centrale
de 256 pixels, sans étirement. Leurs côtés ne sont pas encore redessinés :
un canevas stable ne constitue pas un nouvel arrière-plan panoramique.

Le temps de la météo continue d'avancer pendant les transitions et les
messages. La simulation conserve ses pauses natives : le message d'objet
attend la fanfare, et certaines portes attendent une musique en cours.
Le passage de porte mesuré dans le test prend 105 images avec ou sans
widescreen, soit environ 1,75 seconde. Cela ne mesure pas toutes les portes.

Pendant le milieu d'un chargement de salle, la reconstruction des côtés
n'utilise plus des données de salle et de VRAM provenant de moments
différents. Les coordonnées complètes des sprites sont conservées avant
leur réduction matérielle à neuf bits, pour empêcher un sprite éloigné
de réapparaître sur le bord opposé. Ces informations restent hors de la
mémoire de simulation.

## Vérifications

Résultat local : **36 configurations du HUD** et **15 captures Vulkan**
validées. Sur ces captures, l'écart entre les pixels natifs de l'interface
et leur affichage Unreal est nul. L'introduction a également été capturée :
texte exact, calque gaussien comparé à une référence indépendante (écart
maximal de 2 niveaux sur 255). Les captures GPU ont été reprises après la
compilation du nouveau matériau ; le premier essai avait capturé une image
noire pendant sa préparation asynchrone.

- [HUD : 36 configurations, messages, carte, équipement et compte à rebours](verification.json).
- [Histoire d'introduction, textes et arrivée à Cérès](story-verification.json).
- [Captures Vulkan et contrôle des pixels de l'interface](Unreal/pixel-verification.json).
- [Calque gaussien des cinématiques et textes protégés dans Unreal](Unreal/story-pixel-verification.json).
- [Coordonnées de sprites et rejet des entrées périmées](sprite-coordinates.txt).
- [Parité du rendu rapide et du décodeur de référence dans huit salles](../Widescreen/renderer-parity.json).

Exemples : [HUD complet](Unreal/display-00.png),
[message Morph Ball](Unreal/display-01.png),
[inventaire](Unreal/display-10.png),
[compte à rebours](Unreal/display-14.png),
[introduction](Unreal/story.png).

Pour reproduire les contrôles natifs, après compilation du pont :

```sh
python Scripts/test-display-regressions.py
python Scripts/test-story-presentation.py
python Scripts/test-renderer-parity.py
```

Pour reproduire les captures dans Unreal (matériau compilé au préalable) :

```sh
Scripts/play.sh -SMDisplayTest -SMTestSavedRoom \
  -SMTestSave="$PWD/Unreal/Saved/SMTests/HUD-all-items.sram" \
  -SMWarmup=8500 -RenderOffscreen -unattended
python Scripts/check-display-captures.py
Scripts/play.sh -SMWideTest -SMWarmup=8500 -SMTestHoldState=30 \
  -SMTestFrames=2400 -RenderOffscreen -unattended
python Scripts/check-story-capture.py
```

## Proposition pour les espaces vides

Prévoir des prolongements décoratifs par type de salle : roche sombre dans
les cavernes, structures techniques dans les installations, ciel ou brume
dans les extérieurs. Les zones ajoutées seraient explicitement hors du
terrain jouable. Les portes, objets et plateformes ne seraient pas répétés.

Les arrière-plans particuliers et les compositions des cinématiques
nécessitent ensuite une reprise ciblée. Cette révision ne prétend pas
avoir éliminé chaque artefact de chaque salle du jeu.
