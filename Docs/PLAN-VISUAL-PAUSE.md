# Révision : pause native et décors peints

Direction confirmée le 14 septembre 2026 : utiliser le véritable système de
carte et les assets du jeu ; peindre les prolongements **et** retoucher le décor
intérieur dans un outil externe, sans modifier les collisions.
L'[ancien plan](VisualPause/PLAN-ARCHIVE.md) est conservé comme historique.
Sa proposition de menu Canvas et le remplissage automatique ont été rejetés.

## Livré dans le code

- [x] Retirer la pause Canvas et son interception des commandes.
- [x] Élargir la présentation de la carte native à 400 pixels, avec ses tiles,
  icônes, exploration, défilement et animations existants.
- [x] Répartir les panneaux natifs de Samus et des équipements sur cette largeur.
- [x] Conserver la logique native de sélection, réserves et reprise du jeu.
- [x] Désactiver le remplissage automatique par défaut, y compris les anciennes
  préférences qui le réactivaient.
- [x] Éditeur autonome : zone, salle, état, palette du tileset d'origine,
  pinceau, gomme, pipette, miroirs, annuler/rétablir, calques avant/arrière.
- [x] Peindre dedans/dehors ; afficher les catégories de collisions en lecture
  seule ; garder la géométrie jouable et les données natives inchangées.
- [x] Enregistrer des références de tiles dans des fichiers séparés, avec
  historique ; charger ces fichiers dans la présentation du jeu (Ctrl+F8).
- [x] Laisser reprendre le dessin natif lorsqu'un bloc interactif a changé.

Les effets de combat antérieurs et les réglages graphiques sont conservés.
Aucun portrait ni décor généré ne remplace les assets originaux.

## Validation

Développement et compilation en local. Les tests autonomes utilisent uniquement
la copie isolée `~/Games/SuperMetroid-VisualValidation` sur gaming-pc et ses
sauvegardes de test. Aucun nouveau lancement local sans demande explicite.

Les [preuves de pause](NativePause/README.md) comparent la RAM, les pixels
natifs, la navigation et les captures Unreal. Les [preuves de l'éditeur](RoomEditor/README.md)
couvrent l'extraction des 310 états, la peinture, la persistance dans le navigateur
et l'indépendance de la simulation. Ces tests ne constituent pas une validation
artistique de toutes les salles du jeu.

## Suite artistique et fonctionnelle

- L'utilisatrice peint les raccords voulus, puis les examine en jeu, caméra en
  mouvement et portes ouvertes/fermées. Les exemples de test ne sont pas des
  propositions de décors finis.
- Améliorer l'aperçu des fonds spéciaux, animations et Ceres. L'éditeur indique
  ses limites actuelles ; il ne présente pas son aperçu comme le rendu Unreal.
- Ajouter au besoin sélection rectangulaire, motifs assemblés et navigation
  géographique après usage réel de l'outil.
- Reconcevoir une page Système cohérente avec les tiles et commandes natives.
  L'ancienne page a été retirée ; elle n'est pas livrée dans cette révision.
