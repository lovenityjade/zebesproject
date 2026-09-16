# Generate Game dans le menu natif

Génération initiale du 14 septembre 2026. **Mise à jour du 15 septembre :**
[chaque slot A/B/C possède maintenant son mode et sa seed](../SaveSlots/README.md).
Les captures et tests historiques en fin de page décrivent l'étape précédente.

## Parcours utilisateur

1. Dans le menu système anglais, régler **Randomizer**.
2. Choisir une banque de sauvegardes ou créer **New randomized game**, sans générer de seed.
3. Dans le jeu, choisir A/B/C puis **MODE RANDOMIZED**. **RANDOMIZER OPTIONS** ajuste les réglages de ce slot.
4. Sur **OPTION MODE**, sélectionner **GENERATE GAME**, qui remplace **SPECIAL SETTING MODE**.
5. Attendre la génération et la vérification logique. **START GAME** reste indisponible pendant ce temps.
6. Après succès, **GENERATE GAME** devient indisponible et **START GAME** devient disponible. Le curseur revient sur Start Game ; il faut confirmer pour jouer.

La génération n'est plus déclenchable depuis ImGui. Le menu système conserve les
réglages et la création/gestion des profils. Les paramètres sont copiés dans la
nouvelle partie : modifier les réglages de la prochaine partie ne change pas celle-ci.
Controller Setting Mode reste natif. Les anciennes lignes de langue deviennent
Mode et Randomizer Options. En Vanilla, Start Game est disponible et Generate Game grisé.

| État natif | Generate Game | Start Game | Message |
| --- | --- | --- | --- |
| En attente | Disponible | Grisé, confirmation refusée | GENERATE TO START |
| Génération | Grisé | Grisé | IN PROGRESS |
| Échec | Réessayer | Grisé | GENERATION ERROR ou DATA ERROR |
| Prêt | Grisé, confirmation refusée | Disponible | GAME GENERATED |

Pendant la génération, les animations du menu continuent. Les commandes natives
qui quitteraient l'écran sont bloquées. Le menu système peut s'ouvrir mais refuse
les changements de session pendant le travail. La génération n'est pas annulable.
Les erreurs détaillées restent dans le log et le statut du menu système.

Les lettres à deux tiles de haut proviennent des labels originaux décompressés
par le jeu. Aucun asset généré ni texte ImGui n'est dessiné dans OPTION MODE.

![En attente : Start Game grisé](pending.png)
![Prêt : Generate Game grisé](ready.png)

## Sauvegarde et activation

La banque schema 3 utilise un SRAM natif et trois manifestes de slot indépendants.
Les enfants conservent les schémas validés 1/2 et les contrats tracker décrits ici.
La génération enregistre maintenant une sauvegarde initiale réelle avant Start.
Un journal lie la publication du manifeste, du flag et du SRAM ; Copy et Clear
suivent la même identité de slot. Voir [la documentation du stockage actuel](../SaveSlots/README.md).

L'API native de génération valide les 100 placements et leur visibilité avant
activation. Le mode géré conserve le chemin de la banque ; la seed sélectionnée
est appliquée seulement aux frontières du menu de fichier. Le mode Vanilla remet
les 100 PLMs originaux. Les API historiques hors banque restent compatibles.
Les écritures natives passent par `sm_save`, et non par `saves/sm.srm`.

## Données prêtes pour les futurs trackers

Chaque nouvelle génération inclut `manifest.tracker` (schema 1), relié au numéro
de seed, à son empreinte et à la révision du solver. `trackerSha256` identifie ce
bloc ; le digest de manifeste du profil protège également son contenu stocké.

- **100 emplacements stables** : index identique à l'ABI native, adresse PLM, objet,
  visibilité, bit de collecte, salle, zone, coordonnées PLM, coordonnées de carte
  natives, identifiant AP et chemin PopTracker de référence.
- **Inventaire** : catalogue des types, catégories, mots PLM, bits d'items et de
  beams ; unités et conversions des capacités. Les bits acquis et équipés sont distincts.
- **Logique effective** : preset source, techniques développées avec difficulté,
  règles de dégâts/hell runs/bosses, départ, inventaire initial, objectifs, split,
  progression, patches logiques et adaptations natives nécessaires.
- **Graphe** : accès internes, paires de transitions vanilla, boss et escape,
  géométrie d'entrée/sortie des accès, portes avec couleur et bit d'ouverture.
- **Reproductibilité** : révision et hashes des fichiers de logique. Le spoiler,
  `generatorProgression` et `solverVerification.progressionLog` restent dans le manifeste.

`Randomizer/native_locations.json` est extrait de la ROM originale et des
correspondances auditées du pack. Les 100 bits sont uniques et correspondent aux
arguments PLM réels ; chaque emplacement est contenu dans sa salle. Les coordonnées
sont calculées depuis les headers ROM et les positions PLM, pas depuis le PNG PopTracker.
Le script reproductible est `Scripts/export-native-tracker-catalog.py`.

### État vivant : `sm_tracker_snapshot`

L'ABI définie dans `Native/sm_tracker.h` copie un instantané versionné sur le thread
du jeu, après `sm_step`. Pour la version 1 Linux actuelle, `sizeof` vaut 2688 octets.
Le consommateur doit passer sa capacité, vérifier `version` et `size`, puis rejeter
les résultats de logique dont `session`/`revision` sont devenus anciens.

L'instantané fournit : session, révision, frame, slot, empreinte, état du jeu,
salle/zone/repère de Samus, équipement acquis et actif, capacités, collecte des
100 lieux, bits d'items, boss, événements, portes, stations de carte et exploration.
La session change lors d'un init, d'une activation de seed, d'un changement de slot
ou d'un chargement SRAM, y compris dans le même slot.

`world_valid` n'est vrai qu'en gameplay stable ou pause native ; ne pas utiliser
l'inventaire temporaire des écrans de choix de fichier. Les 256 octets de la carte
courante et les 2048 octets des huit zones sauvegardées gardent leur disposition SNES.
Pour une cellule `(x,y)`, l'octet est `((x & 31) >> 3) + 4 * ((x & 32) + y)` et
le masque `0x80 >> (x & 7)`. La carte courante remplace le bloc de la zone active
jusqu'à sa sauvegarde. Les capacités sont en unités de jeu, pas en nombre de pickups.

La topologie/les placements effectifs ne sont **pas** des découvertes du joueur.
Les futurs trackers devront garder à part l'exploration, les objets observés et
les annotations, sous la clé profil/empreinte/slot. Cette étape prépare les données
et l'ABI ; elle ne dessine pas encore le tracker et n'ajoute pas de service de requête
d'accessibilité instantanée.

## Vérifications

`verification.json` : test sans fenêtre sur **gaming-pc**, dans un dossier isolé,
avec trois seeds (14092026 / Casual / Medium, 14092027 / Regular / Slow,
14092028 / Veteran / Fast).

Le test parcourt les vrais menus avec les entrées de manette, vérifie le blocage
avant génération, l'écran Controllers, les états occupé/erreur/réessai, les 100
mots PLM, le blocage de Generate après succès et le démarrage au Landing Site.
Il vérifie les données du tracker et leur cohérence avec la seed, ainsi qu'un
ramassage réel dans la salle Morph via une fixture de positionnement. Les trois
seeds ont un parcours logique complet validé par le solver, sans opcode 65816 émulé.
La ROM reste inchangée et le menu Vanilla garde Special Setting Mode.

Compilations noyau C et `SMUnrealEditor Linux Development` réussies. Les captures
ici montrent le framebuffer natif. **Ce test ne couvre pas le rendu Unreal final
ni la persistance des profils par le menu ImGui** : ces points restent à vérifier
lors de l'essai interactif. Aucun jeu avec fenêtre n'a été lancé.
