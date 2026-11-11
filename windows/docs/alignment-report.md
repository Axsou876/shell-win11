# Rapport d’alignement — Phase 3.x

Date : 2026-11-11 · Build : Release + Debug Qt 6.9.3 · Thème monochrome.
Référence : `C:\Users\axsou\Documents\shell-main` (lecture seule).

## 1. Méthode

Audit de chaque composant QML des surfaces Dashboard, Launcher, Sidebar,
média, performance, météo, panneaux Audio/Réseau/Bluetooth/Batterie, barre,
cartes, icônes, textes, boutons, champs et indicateurs. Les corrections
remplacent les positions absolues ou les spacers numériques par les layouts
Qt Quick (GridLayout, RowLayout, ColumnLayout) ou par des règles dérivées des
tokens du thème, en conservant le design existant et les règles Linux.

## 2. Corrections appliquées

### Dashboard

- `DashboardHome.qml` : passage d’une grille en positions absolues à un
  **GridLayout 6 colonnes** identique à `Dash.qml` (météo colspan 2, utilisateur
  colspan 3, média rowspan 2, horloge colonne 0, calendrier colspan 3,
  ressources colonne 4). Toutes les cartes partagent désormais les mêmes bords
  de colonnes ; plus de `x`/`y` arbitraires au niveau des cartes.
- Largeurs Linux conservées : météo 275, utilisateur 340, horloge 110,
  ressources 70, média 200 ; rayons 42 / 28 / 16 / 16 / 56, hauteur totale 468
  (180 + 12 + 276) cohérente avec la vue du shell.
- `DashboardShell.qml` : onglets icône au-dessus du label, indicateur centré sur
  la largeur réelle du contenu actif, séparateur 1 px, slide horizontal.
- `DashboardCalendar.qml` : chevrons gauche/droite aux bords, mois réellement
  centré, semaine et grille alignées sur une colonne unique, marqueur du jour
  centré dans sa cellule.

### Météo

- En-tête en `RowLayout` avec spacer `Layout.fillWidth` : suppression du
  `parent.width - 520` magique ; la ville et les stats lever/coucher restent
  alignés à gauche/droite.
- Cartes de détails et prévisions en `RowLayout` avec `Layout.fillWidth` :
  largeurs égales sans formule dépendante du nombre de prévisions.

### Performance

- Colonne de label des cartes Hero contrainte à l’espace réellement disponible
  avant la forme d’usage (`usageShape.x - x - 8`) : plus de chevauchement entre
  le nom GPU/CPU et le libellé « Usage ».
- Libellé « Usage » centré au-dessus de la forme d’usage.
- Rayon du stockage harmonisé sur `roundingExtraLargeIncreased` (32), comme la
  carte Linux.
- Cartes Hero, stockage, réseau et mémoire alignées sur les mêmes marges et
  espacements de 12 px ; jauges et barres centrées dans leurs cartes.

### Launcher

- En-tête passé de 38 à 40 px : le bouton rafraîchir (40 px) ne dépasse plus.
- Pied de page : textes gauche/droite centrés verticalement dans leur bande.
- Recherche, icônes de résultats, noms, descriptions et étoiles conservent le
  centrage vertical existant ; marges internes unifiées.

### Sidebar

- En-tête passé en `RowLayout` : horloge centrée verticalement, bouton fermer
  à droite via spacer `Layout.fillWidth`.
- Ligne Audio passée en `RowLayout` : le slider prend la largeur restante
  (`Layout.fillWidth`) au lieu de `parent.width - 120` ; icône, valeur et bouton
  muet alignés sur le même axe.

### Panneaux système

- `AudioPanel` : ligne principale et ligne 0/100 converties en `RowLayout` avec
  spacers `Layout.fillWidth` (suppression de `-210` et `-60`).
- `BluetoothPanel` : ligne principale convertie en `RowLayout` (suppression de
  `-250`) ; icône, textes et switch alignés.
- `NetworkPanel` et `BatteryPanel` vérifiés : en-têtes, jauges, débits et
  boutons alignés sur les marges 16 et les espacements 8/12.

### Barre

- Bouton Sidebar ajouté à côté des boutons recherche/notifications.
- Horloge, statut, alimentation et titre de fenêtre active vérifiés : icônes
  centrées dans la pilule, marges identiques ; les emboîtements négatifs de
  l’horloge proviennent du design Linux (`BarClock.qml`) et sont conservés.
- Raccourcis F8/F9/F10/Ctrl+Q déplacés uniquement sur l’overlay pour supprimer
  les raccourcis ambigus ; F9 ouvre bien la Sidebar, F8 bascule le thème, F10
  ouvre le Dashboard depuis une surface focusée.

## 3. Captures avant / après

Avant (référence `parity4-*`, `reg-*`) et après (référence `align*`) dans
`windows/build-release/smoke/` :

| Surface | Avant | Après |
|---|---|---|
| Dashboard accueil | `parity4-home.png` | `align2-home.png` |
| Media | `parity4-media.png` | `align2-media.png` |
| Performance | `parity4-performance.png` | `align2-performance.png` |
| Weather | `parity4-weather.png` | `align2-weather.png` |
| Launcher | `reg-launcher.png` | `align1-launcher.png` |
| Sidebar | `reg-sidebar2.png` | `align1-sidebar.png` |
| Audio | `reg-power.png` (contexte) | `align1-popout-audio2.png` |
| Réseau | — | `align1-popout-network2.png` |
| Bluetooth | — | `align1-popout-bluetooth2.png` |
| Batterie | — | `align1-popout-battery.png` |
| DPI 125 % simulé | — | `align1-dpi125-home.png` |

## 4. Validation

- Release et Debug compilent sans erreur.
- Aucune erreur QML (`qrc:`, `TypeError`, `ReferenceError`, `polish`,
  `Cannot specify`, `Unable to assign`) sur les captures finales.
- Valeurs toujours réelles : CPU/GPU/RAM/disque, Spotify, Open-Meteo,
  fenêtres du bureau ; températures `N/D` quand indisponibles.
- 1920×1080 à 100 % (DPI système 96) : captures ci-dessus.
- 125 % : test avec `QT_SCALE_FACTOR=1.25` ; la mise en page du Dashboard reste
  alignée et aucune erreur QML n’apparaît. Le DPI système réel n’a pas été
  modifié (un changement réel demanderait une déconnexion Windows) : c’est une
  simulation Qt, pas une preuve DPI système complète.
- Multi-écrans et AppBar conservés ; barre des tâches restaurée après
  fermeture propre (état 0, zone `0,0 1920×1032`).
- Hover, Échap, clics, animations d’onglets et interactions inchangés.

## 5. Problèmes restants

- Hauteur du panneau fixe (560 px) alors que Linux anime la hauteur par onglet.
- Formes `MaterialShape` Linux approchées par des formes monochromes.
- Visualiseur cava et paroles indisponibles sous Windows.
- Sidebar Windows = Control Centre adapté, pas la sidebar Linux complète.
- La barre Workspaces reste une maquette graphique.
- DPI 125 % seulement simulé au niveau Qt ; le DPI système réel reste à tester
  avec une session Windows à 125 %.
