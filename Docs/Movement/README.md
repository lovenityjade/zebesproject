# Space Jump permissif

Le mode assisté est activé par défaut. **Maj+F12** alterne entre assisté et
original, indépendamment de **F12** qui règle le wall jump. Le choix persiste
dans la section `Controls`, clé `AssistedSpaceJump`, du profil d'affichage.

- Un nouvel appui en fin de montée reste disponible pendant 16 images natives
  (environ 267 ms), jusqu'au début de la descente.
- Toute la descente accepte un nouvel appui : la limite de vitesse qui faisait
  rater les pressions tardives est retirée en mode assisté.
- Un appui déclenche un seul saut ; garder le bouton enfoncé ne produit pas une
  montée automatique. L'appui initial du décollage n'est pas mis en attente.
- L'équipement Space Jump, le saut tournoyant et les restrictions natives des
  liquides restent requis. Aucun double saut sans l'objet, ni rebond de Morph Ball.
- Le saut conserve son impulsion native. L'assistance est remise à zéro au sol,
  pendant les transitions, au changement de salle et au changement d'option.

L'adaptation est générée dans `Native/prepare_overlays.py`. Le chemin original
reste intact dans la routine native ; la file d'appui est stockée hors RAM SNES.
`Scripts/test-spacejump.py` se lance uniquement sur gaming-pc avec une copie de
la sauvegarde de test, et compare le mode original à la bibliothèque précédente.
[Rapport de validation](spacejump-verification.json) : 1 440 images comparées
avec la version précédente en mode original, RAM et pixels identiques. Les
séquences rapides/espacées/tardives donnent respectivement **10/5/1** relances
assistées contre **5/3/0** originales. Aucun saut supplémentaire avec maintien
seul, sans l'objet ou sous l'eau sans Gravity Suit. Le contrôle aquatique
vérifie explicitement l'immersion et la désactivation de la physique Gravity.
Les [trajectoires enregistrées](spacejump-traces.json) accompagnent le rapport.
