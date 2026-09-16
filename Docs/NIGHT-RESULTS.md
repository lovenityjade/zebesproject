# Version de test — 14 septembre 2026

## Lancer

Sur gaming-pc :

```sh
~/Games/SuperMetroid-Unreal/Lancer-Super-Metroid.sh
```

C'est un exécutable Unreal autonome. L'installation de l'éditeur n'est pas
nécessaire sur gaming-pc. Dans le projet de développement local,
`./Lancer-Super-Metroid.sh` continue à lancer la version de travail.

Les tests se ferment automatiquement. Les sauvegardes ordinaires et les
réglages acceptés restent séparés des fixtures dans `SMTests`.
Le réglage conservé est Lighten, 75 % d'opacité, luminosité 110 %, profondeur
et relief actifs ; la préférence existante de parallaxe désactivée est conservée.

## Changements

- Affichage natif **400 × 224** : les côtés proviennent des vraies données de
  la salle. Les sprites ne sont pas étirés. Le rendu gère aussi le Mode 7
  de Cérès ; les salles étroites conservent leurs limites physiques.
- Le décor continue derrière le HUD supérieur. Son fond noir et son ancien
  séparateur sont retirés. La minimap passe de **5 × 3 à 7 × 4 cases**, en
  ajoutant deux colonnes à gauche et une ligne en bas.
- Pluie plus dense en quatre plans, bruit continu pour la brume, effets
  aquatiques et chaleur calculés par le matériau Unreal.
- Éclaboussures à la marche sur sol mouillé, flash du canon, halo de charge,
  explosions éclairées. La Power Bomb produit une onde bleue lumineuse,
  un cœur chaud, des rayons et une rémanence ; ses dégâts restent natifs.
- Wall jump assisté par défaut : appui mémorisé, tolérance au contact et
  impulsion vers l'extérieur du mur. Il faut réappuyer pour rebondir.
  Le chemin original est conservé et sélectionnable.
- Randomizer intégré au processus du jeu : génération, manifeste, spoiler,
  journal de progression, validation et adaptations natives des objets.
  Les menus et le lancement utilisateur d'une partie randomisée restent
  à brancher lors de la prochaine séance, comme demandé.

## Commandes utiles

| Touche | Action |
|---|---|
| F10 | Téléportation vers huit lieux |
| F12 | Wall jump assisté / original |
| Ctrl + F11 | Vue large / vue native étroite |
| F11 | Plein écran |
| F2 | Activer ou désactiver la présentation |
| F4 | Luminosité, dont le réglage 110 % |
| F5 / F7 / F8 | Parallaxe / profondeur / relief |
| F9 | Météo Unreal |
| F1 | Toutes les commandes |

## Randomizer : résultat des tests de seeds

Les **18 seeds 14092026 à 14092043** couvrent trois profils et trois vitesses
progressives. Chacune possède 100 placements relus, un parcours complet du
solver, Mother Brain et le retour au vaisseau. Le journal détaille l'inventaire
avant chaque étape, les techniques, le chemin et la difficulté. La limite
VARIA « medium » n'est pas augmentée pour faire passer une seed.

Les six premières seeds sont aussi générées **dans l'exécutable Unreal sur
gaming-pc**, écrites sur disque puis relues ; leurs placements et empreintes
correspondent aux références locales. Huit seeds démarrent dans le noyau C
et ramassent le vrai objet placé dans la salle de la Morph Ball : Missile,
Morph, Hi-Jump ou Power Bomb selon le spoiler. Les états endormi et éveillé
de Zebes sont couverts. La fixture place Samus près de l'objet ; le ramassage,
l'inventaire et le message restent ceux du jeu.

Les incohérences d'objet, d'adresse ou de visibilité et une table d'objets
impossible à terminer sont rejetées. Les sauvegardes sont isolées par
empreinte de seed ; un retour à vanilla recharge la table originale.

- [Rapport des 18 seeds](Randomizer/verification.json)
- [Exemple de progression détaillée](Randomizer/progression-14092040.md)
- [Démarrages et ramassages natifs](Randomizer/native-verification.json)
- [Génération depuis Unreal sur gaming-pc](GamingPC/Unreal/randomizer-ue-verification.json)
- [Interface et options de patches](../Randomizer/README.md)

Le catalogue distingue les options natives et les patches QoL/UI encore à
porter. Ces derniers sont signalés comme `pending-native`, avec leur portée
vanilla/randomizer ; ils ne sont pas appliqués silencieusement.

## Rendu et simulation

Huit destinations couvrent Crateria, Brinstar, Norfair, Maridia et le Vaisseau
fantôme ; Cérès dispose d'un test Mode 7 supplémentaire. Le centre du rendu
élargi correspond exactement à la scène native. Le décodage optimisé a été
comparé octet par octet à son chemin de référence dans les huit salles,
avec quatre configurations de parallaxe et la mémoire native incluse.

Une séquence de 850 images compare les 128 Kio de mémoire native avec les
effets désactivés puis activés. La Power Bomb et l'explosion d'une bombe sont
présentes dans cette séquence. Les tests ne demandent aucun opcode 65816
émulé. Les captures Vulkan de gaming-pc vérifient séparément le calque
Gaussian, le GUI, la pluie animée, la brume, l'eau et la chaleur.

- [Parité du rendu optimisé](Widescreen/renderer-parity.json)
- [Parité de la simulation pendant les effets](Combat/native-parity.json)
- [Wall jumps, maintien du bouton et absence de rebond en l'air](WallJump/verification.json)
- [Combat réel dans Unreal](GamingPC/Unreal/combat-verification.json)
- [Mesures de performance](GamingPC/Performance.json)

## Ce qui reste à tester

La progression des seeds est validée par le solver ; cela ne remplace pas une
partie entière jouée dans le moteur natif. Les boss, les transitions et les
salles au-delà des scénarios documentés restent à parcourir. Le ressenti du
wall jump et l'intensité artistique des effets attendent ton essai.
