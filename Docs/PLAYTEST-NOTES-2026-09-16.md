# Notes de test — 16 septembre 2026

État : collecte terminée au « stop » de la personne. Liste en attente de ses prochaines instructions : ne pas commencer les investigations, modifications ou tests liés à cette liste avant une nouvelle instruction. Aucune modification du jeu engagée pendant la collecte. Les bugs ci-dessous sont des observations rapportées, sans diagnostic confirmé.

Instruction ultérieure : **Game Over terminé**. Le reste de la liste est maintenant autorisé ; voir [PLAYTEST-IMPLEMENTATION.md](PLAYTEST-IMPLEMENTATION.md) pour les décisions et le suivi.

## Bugs rapportés

1. **Lumière des yeux** : l'effet illumine incorrectement le haut de l'écran. Remplacer cet effet par un véritable effet de lumière, dans l'esprit du travail réalisé sur les Power Bombs.
2. **HUD pendant les transitions** : les transitions changent momentanément les items affichés en haut de l'écran.
3. **Artefacts / sprites dédoublés** : Samus, ou d'autres sprites, peuvent apparaître dédoublés pendant une fraction de seconde. Peut survenir n'importe où. **Précision rapportée : certains sprites qui sortent de l'écran par le bas réapparaissent partiellement en haut de l'écran.** Cause technique non diagnostiquée à ce stade.
4. **Portes des sauvegardes pendant la fuite Chozo** : après la collecte des tablettes et le déclenchement de la destruction, les portes vers les stations de sauvegarde se verrouillent. **Corrigé et vérifié sur gaming-pc** : conserver les états habituels des salles durant la fuite Chozo et permettre les stations malgré la restriction VARIA de l'évasion classique. Détails : [CHOZO-ESCAPE-STRESS.md](CHOZO-ESCAPE-STRESS.md).

## Ajouts et idées

1. **Speedrun Vanilla** : mode dédié, leaderboard en ligne et connexion avec Discord. Le rôle exact de Discord reste à préciser ; aucune intégration engagée.
2. **Récapitulatif de run randomizer** : à la fin du run, retracer sur la carte le chemin complet du joueur, dans l'ordre chronologique, y compris ses retours et revisites ; pas seulement une liste des checks collectés.
3. **New Game+ Vanilla** : recommencer avec tous les items et une difficulté des monstres augmentée. Prévoir également un mode speedrun pour ce New Game+.
4. **Évasion après collecte des Chozo Tablets** :
   - Déclencheur exprimé : « quand on obtient tous les Chozo Tablets ». À préciser après la collecte : le quota requis pour terminer ou toutes les tablettes placées, puisque ces valeurs sont configurables séparément.
   - Appliquer un effet rouge et déclencher des explosions pendant le gameplay.
   - Passer la musique à **Escape**.
   - Afficher le **timer natif du jeu**, avec une durée sélectionnable parmi **3, 5, 6, 7 ou 10 minutes**, pour la course vers le vaisseau.
   - Une fois le vaisseau atteint et Samus entrée à bord, arrêter le timer.
   - Lancer la séquence de vol du vaisseau, puis l'ending, puis le credit roll.

5. **Achievements** : afficher des notifications toast lors du déblocage, accompagnées d'un bref son approprié provenant du jeu. Prévoir un écran Achievements propre et soigné. Le choix du son et les détails de présentation restent à définir.

6. **Sélection des items avec les gâchettes** : **LT = item précédent** et **RT = item suivant**, clavier Q/E, bindings configurables. Implémenté et vérifié dans le cœur natif sur gaming-pc : disponibilité, maintien, pressions simultanées, Select/Cancel et pause. Les bindings personnalisés existants priment en cas de conflit avec les nouveaux défauts.

7. **Priorité fuite Chozo** : après le message de la dernière tablette requise,
   afficher « Something's wrong, get to the ship ! » puis « Stress gives determination ! »
   dans une boîte native, accorder/équiper Space Jump, et seulement après sa fermeture
   démarrer le chrono, la musique et les explosions. Dégâts reçus ×2 durant la fuite.
   Implémenté et vérifié ; sauvegarde originale conservée à 4/5 tablettes.

8. **Refonte du menu UI — différée à la demande de la personne** : catégories plus
   claires, démarrage direct sans ouverture automatique de l'UI, choix Vanilla /
   Randomized dans le menu natif Start Game pour le slot choisi. À faire **après**
   les correctifs en cours ; voir le suivi d'implémentation.

## Règles demandées pour les speedruns

- Les glitches sont autorisés.
- Deux catégories : **No QoL** et **QoL**.
- Dans la catégorie QoL, seules les assistances **Space Jump** et **Wall Jump** sont autorisées.
- Les **Time savers** ne sont pas autorisés dans la version Vanilla.
- Ne pas assimiler « No QoL » à « glitchless » : l'autorisation des glitches s'applique aux deux catégories.

La portée exacte de « Time savers » et les modalités du leaderboard seront à détailler après la collecte, sans modifier les règles ci-dessus par supposition.

## Polish

1. **Map screen** : nettoyer et peaufiner l'écran de carte. Les éléments précis à reprendre restent à détailler ; aucune modification engagée pendant la collecte.
2. **Stations de sauvegarde** : ajouter des effets de lumière et d'électricité pendant la sauvegarde.
3. **Game Over** : travail autorisé ensuite avec les images `gameover-*.png` et `gameover.mp3` fournies. Remplacer le fond et les éléments précédents, centrer **Continue / End** sous le titre, faire clignoter le projecteur, ajouter de la poussière et conserver le son du Metroid sans l’ancienne ambiance. Suivi technique : [GAME-OVER.md](GAME-OVER.md).
