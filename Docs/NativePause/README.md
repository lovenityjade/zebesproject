# Pause native élargie

La pause Canvas dessinée précédemment a été retirée après le retour de
l'utilisatrice. Le jeu utilise de nouveau les routines natives de pause :
carte explorée, icônes, indicateur de Samus, flèches, animations, curseur
d'équipement, réserves et transitions.

Seule la présentation est élargie à 400 pixels. Le cadre et sa grille utilisent
les tiles originales. La carte montre davantage de son tilemap natif ; les
panneaux d'équipement restent à leur résolution d'origine. Aucun texte, portrait
ou panneau de remplacement n'est dessiné par Canvas. Le HUD garde ses ancrages
widescreen et les pixels du menu passent après les shaders de scène.

**Start / Entrée** ouvre et ferme la pause. **R / D** passe de la carte à
l'équipement ; **L / A** revient à la carte. Les commandes et règles de
l'équipement restent celles du jeu.

[Contrôle natif](verification.json) : 720 images comparées avec/sans widescreen,
RAM et image native identiques, carte et panneaux conservés pixel par pixel,
navigation carte → équipement → carte puis reprise. Les flèches horizontales
sont déplacées aux bords élargis, donc exclues de la comparaison centrale de carte.

Les captures et rapports Unreal de cette révision sont dans `GamingPC/`.
Le menu Système de l'ancienne interface a été retiré avec elle. Les raccourcis
d'options restent disponibles ; une future page Système devra être conçue dans
la continuité des assets et de la navigation natifs.
