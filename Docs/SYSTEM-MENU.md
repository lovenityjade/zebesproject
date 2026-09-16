# Menu système Project SM — Dear ImGui

Implémentation du 14 septembre 2026, inspirée du menu de Canon of Convergence.
Le menu est **en anglais**. Depuis la refonte du 16 septembre, il reste fermé
au lancement ; **Escape**, **F4** ou le **clic du stick droit** l’ouvrent. Start
reste la pause native de SM. Le [parcours actuel](MENU-START-FLOW.md) utilise le
menu natif pour choisir Vanilla/Randomized et démarrer ou générer la partie.
Les modes automatiques et les aperçus HUD/éclairage ne sont pas interceptés.

## Direction visuelle et navigation

Même organisation que CoC : surface pleine fenêtre, fond bleu noir, panneaux
sobres, accents dorés, Montserrat, navigation et catégories latérales.
Aucun sprite ni illustration généré. Les assets de la pause native ne sont pas
remplacés. La colonne de catégories devient un sélecteur dans les petites fenêtres,
et les lignes de réglage s'empilent quand l'espace manque. Chaque page défile.

Rubriques : **Overview**, **Settings**, **Randomizer**, **Save library**,
**Achievements**, **Session**, **Debug tools**.
Recherche transversale par nom/description et profils. Navigation souris, clavier
(Tab, flèches, Entrée) et manette (croix/stick, A). Escape/Back annule une capture
ou une confirmation, efface la recherche, retourne à Overview, puis reprend le jeu.
Le bouton Return to game reprend directement. L'ouverture suspend les frames natives et
vide la file audio ; la fermeture conserve l'état de pause manuelle précédent.

## Réglages effectivement branchés

| Page | Réglages |
| --- | --- |
| Display | Widescreen, fenêtre/borderless, VSync, échelle de l’image |
| Atmosphere & effects | Gaussian Lighten/Multiply, opacité, luminosité, parallaxe, profondeur, relief, météo |
| Audio | Volume général, mute, trame Original/Remastered |
| Controls | Réaffectation des 12 commandes SNES et Previous/Next weapon, échange des doublons, deadzone, restauration |
| Gameplay & comfort | Assists Wall Jump/Space Jump, recharge en sauvegardant, force des flashes |
| Interface | Échelle du menu, raccourcis, options d’interface VARIA |
| Map & trackers | Trackers, activation en Vanilla, techniques Vanilla et légende |

Les préférences vivent dans `Saved/SM/Presentation.ini`. Les paramètres de fenêtre
et VSync utilisent `UGameUserSettings`. Les réglages de présentation et mouvement
s'appliquent aux deux modes. Le menu ne remplace pas les contrôles avancés d'un
pilote audio ou d'une manette ; le remapping ne change pas les boutons de navigation
du menu lui-même. Escape, touches F, P et clic du stick droit restent réservés.

## Debug : commandes F1–F12

L’onglet **Debug** rassemble les outils de présentation existants, avec leurs
valeurs actuelles. Les réglages partagent les mêmes variables et préférences que
Settings et les raccourcis de jeu ; ils sont également trouvables par la recherche.

| Commande d’origine | Contrôle Debug |
| --- | --- |
| F1 | Aide des raccourcis |
| F2 | Atmosphère et effets associés |
| F3 | Opacité : cycle 25 / 50 / 75 / 100 % |
| F4 | Luminosité : cycle 85 / 100 / 110 / 115 % ; la touche F4 ouvre maintenant le menu système |
| F5 | Parallaxe |
| F6 | Lighten / Multiply |
| F7 | Profondeur du décor |
| F8 | Relief |
| Ctrl+F8 | Rechargement des décors peints dans l’éditeur externe |
| F9 | Météo Unreal |
| F10 | Ouverture du sélecteur de téléportation existant |
| F11 | Bascule plein écran |
| Ctrl+F11 | Vue large / largeur native |
| F12 | Wall jump assisté / original |
| Maj+F12, alias Ctrl+F12 | Space Jump assisté / original |

La téléportation reste désactivée hors gameplay et pendant les messages ou
transitions. Son bouton ferme le menu système et ouvre le sélecteur existant,
simulation et audio suspendus jusqu’au choix ou à l’annulation.

## Randomizer

Catégories : **Seed & sharing**, **Logic & difficulty**, **Progression**,
**Items & ammo**, **World & escape**, **Goals & victory**, **Gameplay patches**,
puis **Techniques** et **Combat & heat** sous Advanced.

Le catalogue intégré comprend les presets, techniques, tolérances, pools,
progression, connexions area/boss, portes, points de départ, objectifs, variantes
de Tourian, règles d’évasion et patches pris en charge. Chozo Tablet Hunt ajoute
ses quotas et sa durée d’évasion. Mirror et Race restent exclus du périmètre.
Une vérification automatique indique les conflits et leurs catégories ; la
configuration complète est partageable par settings string ou fichier JSON.

L’UI prépare les réglages, sans créer obligatoirement une banque. Le choix
**Mode Randomized** dans un nouveau slot copie ces réglages. **Randomizer Options**
permet de les ajuster pour ce slot. La génération se déclenche uniquement dans
le menu natif avec **Generate Game**. **Start Game** reste verrouillé jusqu’au
succès, puis Generate Game devient indisponible. Le travail VARIA est asynchrone,
dans le processus du jeu ; ImGui ne lance pas lui-même la génération.

Voir le [parcours natif, les captures et le contrat tracker](Randomizer/NativeMenu/README.md).

## Profils, sauvegardes et flags

Depuis le 15 septembre, chaque banque possède **trois slots indépendants A/B/C**.
Le menu natif permet de choisir Vanilla ou Randomized pour chaque nouvelle partie.
Les réglages sont modifiables avant génération ; une partie existante conserve
son mode et sa seed. L'étoile des assets du menu original marque les slots Randomized.

Le format de banque est `profile.json` schema 3, avec trois références vers des
manifestes enfants schema 1/2. `bank.sram.dat` est le SRAM natif partagé. Chaque
slot conserve sa propre seed et ses données de tracker. Génération, Copy et Clear
publient ensemble leur identité et le SRAM via un journal récupérable.
Les profils historiques restent sur disque et sont importés dans de nouvelles
banques. Les emplacements historiques vides restent configurables.

La dernière banque activée est mémorisée dans Presentation.ini et rechargée au
lancement normal. Le changement de session conserve les confirmations existantes
pour la progression non sauvegardée. Une génération crée immédiatement une vraie
sauvegarde native au point de départ choisi, avant de débloquer Start Game.

Voir [les slots indépendants, les captures et la validation](Randomizer/SaveSlots/README.md).

**Le menu n'ajoute pas de sauvegarde instantanée.** Les stations de sauvegarde
restent nécessaires. Reset to title et Exit game vident le SRAM sur disque mais
ne sauvegardent pas la position actuelle. Achievements désigne les cinq jalons
locaux existants, isolés par profil, sans intégration à une plateforme externe.

## Implémentation

- `SMSystemMenu.cpp/.h` : pages ImGui, préférences, génération async et activation.
- `SMProfiles.cpp/.h` : métadonnées, validation et publication des profils.
- `SMImGuiWidget.cpp/.h` : widget Slate, font atlas, triangles texturés, clipping,
  souris, texte UTF-8, clavier, manette, focus et presse-papiers.
- `SMHUD.cpp` : attachement, suspension des frames, commandes et cycle de vie.
- `SMPause.cpp` : reset conserve désormais aussi l'assistance Space Jump.
- `Unreal/ThirdParty/ImGui` : sources officielles v1.91.9b, MIT, sommes SHA-256.
- `Unreal/Content/UI` : Montserrat existant de CoC et licence OFL, staged NonUFS.

La pause du jeu reste celle déjà implémentée dans le noyau et son rendu Unreal.
Le nouveau menu système est une couche distincte, pas l'ancien popup Canvas rejeté.

## Validation et limites

La refonte et ses vérifications du 16 septembre sont documentées dans
[MENU-START-FLOW.md](MENU-START-FLOW.md). Les paragraphes suivants conservent
l’historique des premiers audits du 14–15 septembre ; leurs restrictions de
couverture ne décrivent pas à elles seules l’état actuel.

### Audits initiaux

Compilation `SMUnrealEditor Linux Development` avec Unreal 5.8.2 : réussie après
correction des adaptations Slate. Les [tests natifs sans fenêtre sur gaming-pc](Randomizer/NativeMenu/verification.json)
valident maintenant trois générations et le verrouillage des options. Aucun jeu avec
fenêtre lancé. La compilation ne prouve pas la qualité du rendu ou des contrôles ImGui.
Les résultats de seeds des audits antérieurs ne prouvent pas ce parcours menu.

Mise à jour du 15 septembre : les [tests des slots A/B/C](Randomizer/SaveSlots/README.md)
valident les modes mixtes, deux seeds indépendantes, sauvegarde initiale/rechargement,
Copy/Clear et erreur d'écriture simulée. Le binaire Unreal exécuté avec NullRHI valide
séparément les requêtes, manifestes, migrations historiques et récupération du journal.
Le parcours interactif complet ImGui reste à vérifier.

À vérifier lors du prochain test autorisé (gaming-pc pour les essais autonomes) :

1. Home à 1280×720, petite fenêtre et grand écran, échelle 0.8–1.4 ; défilement,
   recherche, clavier, manette, souris, focus après Alt-Tab.
2. Ouvrir/fermer en jeu et dans la pause native ; aucun input ni audio retenu.
3. Modifier chaque réglage ; fermer/relancer et vérifier sa persistance.
4. Dans une banque, mixer A Vanilla et B/C Randomized, vérifier les badges et
   empreintes, sauvegarder en station, alterner les slots puis relancer.
5. Comparer les SRAM et manifests avant/après chaque session ; les autres profils
   doivent rester intacts. Relire le spoiler et la progression de chaque seed.
6. Exercer les erreurs (dossier non inscriptible, manifeste modifié, SRAM tronqué,
   génération refusée), les confirmations Reset/Exit et la restauration précédente.

La version packagée existante n'est pas republiée par la compilation Editor.
Le lanceur local normal utilise le module Editor reconstruit. `Scripts/package.sh`
reconstruira le package avec la police et les licences lors de sa prochaine exécution.
