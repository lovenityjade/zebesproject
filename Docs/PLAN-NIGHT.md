# Objectif de la nuit — 14 septembre 2026

## Demandes acceptées

- [x] Vrai widescreen : nouvelles portions de la salle, sans étirement du dessin.
- [x] Retirer le fond noir derrière le HUD supérieur ; minimap +2 cases à gauche,
  +1 case vers le bas, sans réduire la lisibilité.
- [x] Pluie plus dense, proche de l'originale ; brouillard lisse sans pavés.
- [x] Éclaboussures à la marche sur les surfaces mouillées ; flash du canon ;
  amplification lumineuse du charge beam et des explosions.
- [x] Nouvelle Power Bomb : onde et lumière plus impressionnantes, même logique
  de dégâts et de portée dans la simulation native.
- [x] Wall jumps assistés par défaut : tolérance aux appuis anticipés/tardifs,
  changement de direction facilité ; mode original conservé et sélectionnable.
- [x] RandomMetroidSolver : intégré au code du port (génération de seeds, placement
  des objets, options de patches), sans outil externe à lancer. Le branchement
  aux menus et l'activation d'une partie randomisée attendent la prochaine séance.
- [x] Examiner les patches VARIA (HUD/UI, confort et autres) et préparer des
  ajouts optionnels avec compatibilité vanilla/randomizer explicite ; traduire
  les modifications de code SNES nécessaires, sans activation automatique.
- [x] Déployer et tester la version sur gaming-pc, avec captures et rapports.

## Ordre de travail et critères

1. Inventaire des deux machines, état des builds et sauvegardes ; préparer une
   version autonome pour gaming-pc et des sauvegardes de validation séparées.
2. Affichage élargi/HUD/minimap : rendre les côtés depuis les vraies données de
   salle, conserver le repère de simulation et les collisions. Vérifier des
   salles des différentes zones, les transitions et les limites de salles.
3. Ambiance et combat : effets GPU fluides, signaux natifs pour les déclencher ;
   vérifier en mouvement, GUI intact, pas d'impact sur les dégâts.
4. Wall jump : assistance limitée aux murs réellement présents, pression de
   saut distincte, pas de rebond en plein air ; tester gauche/droite, répétitions,
   appuis anticipés et tardifs, et parité du mode original.
5. Randomizer : épingler la source, identifier l'interface et ses dépendances,
   produire un module intégré et testable, compilé avec le port, et un contrat
   pour les menus de demain. La source VARIA est une dépendance interne, pas
   un programme séparé présenté à l'utilisateur.
   Les patches 65816 d'une ROM randomisée ne sont pas présumés compatibles avec
   le moteur C natif ; expliciter et valider la traduction nécessaire.
6. Validation locale puis sur gaming-pc, sauvegardes conservées ; laisser les
   chemins de relance, commandes, captures et limites dans les documents.

L'objectif de la session inclut le wall jump ajouté après sa création initiale.
Les menus généraux et la connexion du randomizer restent pour la prochaine séance.

## Preuves finales

Voir [NIGHT-RESULTS.md](NIGHT-RESULTS.md) pour les commandes de relance, les
rapports et les limites. Le statut terminé porte sur cette version de test
et les scénarios documentés, sans prétendre à un parcours complet du jeu.
Les patches QoL/UI non portés restent identifiés comme options futures ;
les adaptations requises par le randomizer sont natives et testées.
