# Révision visuelle et pause — 14 septembre 2026

**Archive :** la pause décrite ci-dessous a été rejetée par l'utilisatrice,
puis remplacée par la [pause native élargie](../NativePause/README.md).
Le remplissage automatique a été désactivé au profit de
[retouches manuelles dans un éditeur externe](../RoomEditor/README.md).
Les rapports ci-dessous décrivent l'ancienne version, pas une validation
esthétique ni le comportement actuel de ces deux fonctions.

Validation autonome sur **gaming-pc**, RTX 3060, dans
`~/Games/SuperMetroid-VisualValidation`. Les tests utilisent des copies privées
de SRAM. Aucun lancement local depuis la consigne de l'utilisatrice.

## Changements implémentés

- Raccords de décor par blocs originaux de 16 × 16, choisis dans la salle.
  Les candidats de premier plan se limitent aux solides ordinaires ; les
  portes, objets et blocs destructibles ne deviennent pas des motifs décoratifs.
  Les murs peuvent se prolonger hors du cadre jouable. Les collisions restent natives.
- Power Bomb : cœur incandescent, front lumineux, rayons, flash et particules.
  L'intensité du flash est réglable dans les options.
- Grappin : arcs électriques, lumière au canon et étincelles à l'extrémité.
  Screw Attack : arcs dorés, lumière et petites particules. Morts d'ennemis :
  éclat et débris, issus des événements natifs de mort.
- Pause à trois onglets **Système / Carte / Samus**. Carte de 368 × 144 pixels
  logiques, zoom 1×/2×, déplacement et recentrage ; les cases inconnues restent
  masquées. Grille et balayage animés pendant l'arrêt de la simulation.
- Diagnostic de Samus : dessin et police de la ROM, repères d'équipement
  animés, activation des modules, exclusion Spazer/Plasma et réserves.
  Aucun dessin généré par IA. Le sprite extrait correspond exactement au rendu natif.
- Système : options, cinq succès locaux, commandes, retour au titre et sortie.
  Le retour au titre demande confirmation et conserve la sauvegarde et le profil.
  Les succès d'une sauvegarde de test restent séparés de ceux d'une partie normale.

## Commandes de la pause

Start / Entrée ouvre ou ferme la pause. L/R à la manette, A/D au clavier,
changent d'onglet. Flèches ou croix directionnelle pour naviguer ; Z / bouton A
pour confirmer ; Échap / bouton B pour revenir. Sur la carte, C / bouton X
recentre et Z / bouton A change le zoom. Ctrl+F1 permet de retrouver la pause native.

## Preuves

- [Raccords, huit destinations](Borders/verification.json) : rendu natif et RAM
  identiques avec les extensions activées ou désactivées.
- [Six salles signalées](Borders/reported-verification.json) : 121 images
  comparées par salle, déplacement dans les deux sens, état de Zebes éveillé.
- [Menu natif](pause-core-verification.json) : 28 activations, réserves,
  exclusion des rayons, extraction sans modification de RAM, reprise native.
- [Déclenchement électrique natif](electric-core-verification.json) : grappin,
  Screw Attack et morts d'ennemis déclenchés par les entrées du jeu.
- Les captures `pause-new-*.png`, `electric-*.png`, `combat-*.png` et
  `powerbomb-*.png` proviennent du rendu Vulkan sur gaming-pc.
- Les rapports de la version empaquetée sont conservés sous `GamingPC/`.
  Ils distinguent réussite fonctionnelle et appréciation visuelle.
  [Rapport consolidé et empreintes du build](GamingPC/validation.json).
- [Aperçu électrique animé](GamingPC/electric-preview.mp4) : captures
  échantillonnées, assemblées selon leurs indices de trame native. La cadence
  est approximative ; ce fichier ne mesure pas les performances.

Le parcours de pause vérifie l'arrêt de la simulation, l'équipement, les options,
la fermeture/réouverture, l'annulation du reset, sa confirmation puis le
rechargement. La sauvegarde est comparée avant et après le reset.
La persistance des succès et options est contrôlée par relecture du disque.
Le test a détecté que le cache global des INI d'Unreal ignorait la création
d'un profil absent dans le build empaqueté. Les fichiers de profil et d'options
utilisent maintenant une lecture/écriture directe via `FConfigFile`.

## Limites à conserver visibles

La continuation des murs est une première règle de proximité et de continuité,
pas une bibliothèque complète de motifs par tileset. Les grandes parois peuvent
encore paraître répétitives. Les quatorze salles contrôlées ne prouvent pas que
toutes les portes verticales, salles de boss ou transformations du décor sont
couvertes. Une terminaison particulière par salle peut encore être nécessaire.

Les captures du grappin couvrent l'extension et la rétraction ; elles ne valident
pas encore tous les angles d'accroche et de balancement. Les temps des captures
automatiques ne servent pas de mesure de performance. Les légendes actuelles
affichent surtout les touches clavier, même si les commandes manette sont branchées.

## Reproduction sur la machine autorisée

Depuis la copie isolée sur gaming-pc :

```sh
bash Scripts/test-visual-gaming-pc.sh
python3 Scripts/check-display-captures.py SMUnreal/Saved/SMTests
```

Le lanceur de tests refuse de s'exécuter sur une autre machine ou depuis
l'installation habituelle. Il ne déploie rien dans celle-ci.
