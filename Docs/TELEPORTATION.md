# Téléportation pour les essais

F10 ouvre le menu et suspend la simulation. Haut/Bas choisissent la destination,
Entrée ou Z confirment, Échap ou F10 ferment le menu. La croix directionnelle
et le bouton inférieur de la manette fonctionnent aussi dans le menu.

Destinations : vaisseau et refuge de Crateria, puits vert de Brinstar,
cavernes chaudes et entrée des profondeurs de Norfair, ouest et est de Maridia,
hall du vaisseau fantôme.

Chaque destination réutilise une entrée existante de la table native des points
de chargement, avec sa salle, sa porte et ses coordonnées. Le chargement normal
reconstruit les tuiles, ennemis, collisions, musique et effets du lieu.
L'équipement et les événements de la partie sont conservés. Les dangers du jeu
restent actifs : cette fonction ne donne pas d'invincibilité.

Le menu est disponible pendant le gameplay, hors message. Une demande pendant
un chargement ou une coroutine de message est refusée. Le déplacement ne
déclenche pas de sauvegarde ; sauvegarder ensuite à une station suit le
fonctionnement normal du jeu.

`Tests/check_teleport.py` parcourt les huit destinations et revient au vaisseau
sur une copie de SRAM. Il contrôle la salle réelle, le retour au gameplay,
l'absence de CPU 65816 émulé, le refus des indices invalides et des demandes
pendant le chargement, ainsi que la conservation des fichiers de sauvegarde.
Résultat et captures : `Teleport/verification.json` et `Teleport/destination-*.png`.

`Scripts/test-teleport-ui.sh` injecte les touches dans le contrôleur Unreal et
vérifie ouverture F10, navigation, arrêt de la simulation, annulation Échap,
confirmation Entrée et arrivée native à Brinstar. La capture du menu est
`Teleport/menu.png`, avec le rapport `Teleport/ui-verification.json`.
