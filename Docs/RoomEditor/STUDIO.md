# The Zebes Project — atelier de décors

## Retouches intégrées le 18 septembre 2026

Les sept salles corrigées par l'utilisateur sont disponibles dans le jeu local :
Landing Site, Gauntlet Entrance, Parlor and Alcatraz, Crateria Power Bomb Room,
West Ocean, East Ocean et Gauntlet Energy Tank Room. Les 2 217 cellules peintes
et l'atlas personnalisé sont conservés sans modification.

Six variantes ont été ajoutées pour les autres états de Landing Site,
Gauntlet Entrance et Parlor and Alcatraz. Le tileset et les blocs d'origine aux
coordonnées peintes sont identiques entre ces états : les mêmes corrections
restent donc disponibles après les événements natifs, y compris l'évasion.
Il y a désormais 13 fichiers et 5 012 cellules en comptant ces variantes.
Ce sont des copies explicites, pas un héritage automatique : une nouvelle
retouche de l'état normal doit aussi être reportée aux états concernés.

Le [rapport d'intégration](corrected-maps-verification.json) consigne les tests
sur gaming-pc : 68 vues natives avant/après, mémoire de simulation identique,
ROM et sauvegarde de référence intactes, et chargement des 13 fichiers par
Unreal sans rejet. Des captures des sept salles ont été examinées. Cela ne
remplace pas un parcours manuel complet des salles et de leurs transitions.
Le lancement local et la publication d'une nouvelle release restent distincts.

Implémentation du 18 septembre 2026. Ces changements sont dans le projet de
travail; la distribution ALPHA-0.25 reste inchangée.

## Bibliothèque et pixel art

- **Source → Tous les tilesets** : les 29 tilesets natifs, avec leurs palettes.
  Une tile d'un autre tileset est copiée dans la palette personnalisée lors de
  sa première utilisation, pour garder ses couleurs dans la salle choisie.
- **Nouvelle tile**, **Tile noire**, **Éditer les pixels** : édition 16 × 16,
  couleur, gomme transparente, annulation des coups de pinceau.
- **Pivoter la tile ↻** : copie de la sélection tournée de 90 degrés.
- **Pivoter le tileset ↻** : rotation de toute l'image de palette, positions
  comprises. Deux clics donnent 180 degrés, trois donnent 270 degrés.
  La confirmation prévient que les peintures personnalisées existantes gardent
  leurs indices et utilisent donc la nouvelle palette. Exporter la palette
  avant de la remplacer pour en conserver une copie.
- Import/export PNG : dimensions multiples de 16 pour les tilesets, jusqu'à
  1024 tiles. Les PNG sont convertis automatiquement pour le rendu natif.

## Caches-secrets

Sur le décor avant, cocher **Cache-secret**, choisir le rayon et l'opacité à
proximité, puis peindre. Le décor reste opaque loin de Samus et devient
progressivement translucide lorsqu'elle approche. Cliquer dans l'aperçu déplace
son repère cyan pour examiner l'effet. Cela ne crée pas de collision.

## Parallaxe

L'outil expose les plans de présentation existants : ciel, montagnes et
végétation au Landing Site, fond BG2 dans les salles ordinaires compatibles.
Choisir un plan ou **Fond complet**, importer un PNG, régler les vitesses X/Y,
les décalages et les répétitions. Le fond complet remplace les réglages par plan.
**Rétablir le plan original** retire la personnalisation de la cible.

Pour un PNG, 100 % suit la caméra et 0 % reste fixe à l'écran. Pour un fond
natif, le pourcentage règle l'amplitude du parallaxe supplémentaire existant;
les animations et déplacements natifs restent sous le contrôle du jeu.
Les décalages et répétitions s'appliquent aux PNG seulement.

L'aperçu anime une caméra et utilise une reconstruction statique des fonds ROM.
Il ne simule pas les scripts spéciaux, le vent/HDMA, les animations de palette,
les ennemis ni les effets Unreal. Ce n'est pas une reproduction exacte du rendu
final. Les salles Ceres et les fonds utilisés pour les corps de Kraid/Mother
Brain ne proposent pas cette substitution de fond.

## Zones d'événement

**Zone d'événement** permet de tracer un rectangle et d'y écrire une consigne,
par exemple « lumière bleue pulsante » ou « zone d'obscurité ». Cliquer à nouveau
sur la zone permet de modifier ou supprimer la note. Ces commentaires sont
conservés avec la salle pour une implémentation ultérieure : ils ne sont pas
interprétés comme du code et n'activent aucun effet par eux-mêmes.

## Enregistrement et jeu

Enregistrer avec **Ctrl+S**; **Ctrl+F8 dans le jeu** recharge les décors.
Les JSON de version 2 restent dans `Config/RoomDecorations`, les images PNG et
leur copie BGRA dans `images/`. Les JSON de version 1 restent compatibles.
Les fichiers précédents sont conservés dans `.history/`.

Les données ROM, sauvegardes, collisions, BTS et objets ne sont pas modifiés.
Les retouches intérieures sont ignorées lorsque leur bloc natif change, pour ne
pas masquer une porte ouverte ou un bloc détruit. Les images sont chargées
pour la salle courante seulement. Les notes ne sont pas dessinées dans le jeu.

## Vérifications

Tests exécutés sur gaming-pc dans un répertoire isolé :

- [Validation et composition native](studio-verification.json) : 29 tilesets,
  imports, refus des données invalides, opacité près/loin, priorité des sprites,
  préservation de la mémoire de simulation lors de la composition et des
  fichiers ROM/sauvegarde de référence.
- [Parcours navigateur](studio-browser-verification.json) : pixel art, tile
  noire, rotations, PNG, cache-secret, annuler/rétablir les peintures, zones de
  commentaire, enregistrement et relecture.
- [Chargement et rendu Unreal](studio-runtime-verification.json) : patch v2,
  atlas personnalisé, cache translucide et fond PNG visibles dans la salle
  de test, avec le HUD, Samus et les sprites natifs conservés. Le scénario de
  démarrage automatique a aussi été adapté à la rangée Start Game du menu actuel.

Les bibliothèques Linux compilées sur gaming-pc ont été copiées dans le projet
local pour le prochain lancement. Les anciennes versions sont sauvegardées dans
`.tmp/editor-studio/before`. Aucun jeu n'a été lancé ou compilé localement.

Les tiles et couleurs de test sont volontairement artificielles. Ces essais ne
valident pas la direction artistique d'une salle retouchée par l'utilisateur.
