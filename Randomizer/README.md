# Randomizer intégré au port natif

Le service `sm_randomizer_generate` est compilé dans `libsm_native.so` et appelé
par `FSMRandomizer` dans Unreal. La génération et le solver VARIA s'exécutent
**dans le processus du jeu**, dans un interpréteur Python embarqué isolé par
essai. Aucun serveur, programme externe ou ROM patchée à lancer.

Source interne : [RandomMetroidSolver](https://github.com/theonlydude/RandomMetroidSolver),
branche `production`, révision `72ec1f30b700442d0c30aad6c43801bd64f1e2a5`.
Les sources et leur licence MIT sont conservées dans `upstream/`.

## Contrat pour les menus

`FSMRandomizer::Generate` accepte une seed positive, un profil
`casual`/`regular`/`veteran`, une progression `slow`/`medium`/`fast` et des IDs
optionnels de `patches.json`. `Stage` écrit un manifeste atomique, `ReadPlan`
le recharge et le valide. Le [menu ImGui](../Docs/SYSTEM-MENU.md) appelle maintenant
la création d’un profil en attente. La génération et le déblocage de Start Game
sont maintenant déclenchés par **Generate Game dans le menu natif**. Voir
[le parcours et les données de tracker](../Docs/Randomizer/NativeMenu/README.md).
Trois seeds passent le nouveau test natif sans fenêtre sur gaming-pc ;
la validation interactive du parcours ImGui reste à effectuer.

Le noyau propose `sm_seed_stage`, puis `sm_init` avec une sauvegarde dont le
chemin contient l'empreinte complète de la seed. Les 100 mots PLM sont chargés
dans une copie mémoire de la ROM. La ROM source reste intacte. Une session
active refuse tout remplacement de seed et `sm_seed_clear` restaure vanilla
au prochain chargement. Aucun patch de code 65816 n'est exécuté par émulation.

Les adaptations natives indispensables concernent le départ au vaisseau,
les portes initiales et l'éveil de Zebes, ainsi que la présence de l'objet
à l'emplacement de la Morph Ball après cet éveil. `patches.json` distingue
ces capacités natives des options QoL/UI encore à porter. Une option marquée
`pending-native` est signalée dans le manifeste et n'est jamais appliquée
silencieusement.

## Preuves et portée

- `Scripts/test-randomizer.py` : 18 seeds, trois profils et trois vitesses,
  génération par l'ABI native, sérialisation, décodage des 100 PLM et solver
  neuf avec inventaire vide. Tous les objets, Mother Brain et l'évasion sont
  requis ; la limite VARIA « medium » est strictement inférieure à 10.
- `Scripts/test-randomizer-negative.py` : spoilers incohérents et table valide
  mais impossible rejetés.
- `Scripts/test-seed-native.py` : chargement effectif du noyau C, relecture des
  100 placements, démarrage au vaisseau et ramassages par les vrais PLM dans
  la salle de la Morph Ball. La fixture place Samus à côté de l'objet ;
  mouvement, collision, inventaire et message restent ceux du jeu.
- `-SMRandomizerTest` : génération depuis le module Unreal réel et relecture
  du manifeste préparé ; ne démarre pas une partie randomisée utilisateur.

`Docs/Randomizer/seed-*.json` contient le spoiler, le journal du générateur et
le `solverVerification.progressionLog` détaillé : inventaire avant chaque
étape, chemin, techniques, exigences et difficulté. Les fichiers
`progression-*.md` donnent une lecture rapide de ce dernier.

La validation du parcours est une preuve logique. Elle ne prétend pas être
une partie complète jouée dans le moteur natif. Les tests natifs couvrent
les adaptations et ramassages ciblés. Ils ne valident pas le nouveau parcours
interactif du menu ImGui.

Les nouvelles générations comprennent le contrat `manifest.tracker`, ses emplacements
natifs, les règles effectives et la topologie, ainsi que `trackerSha256`.
`sm_tracker_snapshot` fournit les observations du jeu séparément du spoiler.
