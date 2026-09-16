# Retouches visuelles des salles

Les fichiers JSON enregistrés par l'éditeur externe se trouvent ici.
Chaque fichier cible une salle, un état natif et son tileset. Il contient des
références de tiles, jamais de ROM modifiée ni de données de collision.

Dans le jeu, **Ctrl+F8** recharge ces fichiers. Les tiles restent celles du
tileset actif, avec ses palettes et animations. Un bloc interactif qui change
depuis l'état original retrouve son rendu natif, plutôt que de conserver une
peinture par-dessus une porte ouverte ou un bloc détruit.

Les versions précédentes des fichiers sont conservées dans `.history/`.
Ce dossier est exclu de la lecture par le jeu et du paquet de distribution.
