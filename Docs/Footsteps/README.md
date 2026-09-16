# Particules aux pas

Les émissions suivent les contacts de pieds de l'animation native de course,
et leur position dans la salle. Aucun compteur de distance artificiel ne
continue d'émettre lorsque Samus est immobile, en saut ou en Morph Ball.

| Environnement lu dans le jeu | Effet de présentation |
| --- | --- |
| Pluie et bord de l'eau | Gouttelettes et petite éclaboussure basse |
| Pieds immergés | Limon en suspension et bulles qui remontent |
| Maridia sèche et sable mouvant | Poussière sableuse |
| Salle réellement chauffée | Poussière volcanique sombre |
| Vaisseau fantôme, Tourian, Ceres | Quelques fines particules grises |
| Autres sols secs | Petite poussière mate |

La classification emploie l'environnement natif et des familles de zones.
Elle ne prétend pas identifier individuellement le matériau de chaque tile.
La palette du jeu, les collisions et les données des salles restent intactes.

Les poussières sont translucides, sans halo lumineux. Les gouttes retombent et
s'effacent au niveau du sol ; les bulles remontent vers la surface. Les effets
sont effacés au changement de salle et utilisent le budget de particules Unreal
existant, distinct du générateur aléatoire de la simulation.

`-SMFootstepTest` parcourt des destinations réelles avec la sauvegarde privée sur
gaming-pc, enregistre les compteurs d'émission et des captures Unreal. Le [rapport Unreal](GamingPC/footstep-verification.json) confirme les six
familles : 4 émissions sèches, 16 humides, 15 immergées, 2 sableuses, 4 en zone
chaude et 17 métalliques. Les captures ont été revues, notamment les bulles et
les éclaboussures. La validation esthétique finale reste le retour de jeu de
l'utilisatrice.

Commande depuis la copie isolée de gaming-pc :
`bash Scripts/test-visual-gaming-pc.sh Footstep`.
Le scénario utilise la sauvegarde d'arrivée sous la pluie, puis donne les
équipements après son chargement ; une sauvegarde déjà équipée sélectionne
une autre version de Crateria. Seule la sauvegarde privée du test est écrite.
