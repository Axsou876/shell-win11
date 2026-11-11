# Correction ciblée — alignement de l’onglet Weather

Date : 2026-11-11 · Build : Release + Debug Qt 6.9.3.
Fichier modifié : `windows/src/qml/overlay/DashboardWeather.qml` uniquement.

## 1. Cause exacte

L’onglet Weather utilisait une **`Column` classique** dont les enfants imposaient
`width: parent.width`. Contrairement à `shell-main/modules/dashboard/WeatherTab.qml` :

- l’en-tête (ville/date + lever/coucher) n’avait **aucune marge latérale 16 px** :
  la ville était collée au bord gauche et les heures au bord droit, alors que
  toutes les autres sections avaient leurs propres marges ;
- le titre « Prévisions » n’avait **ni marge gauche 12 px ni marge haute 12 px** ;
- les cartes de prévisions utilisaient une hauteur fixe de 128 px et le titre
  n’était séparé que par le spacing de la Columns ;
- la `Column` ne participait pas au layout : la répartition verticale dépendait
  uniquement des hauteurs implicites, sans les règles du `ColumnLayout` Linux.

Résultat : l’en-tête et le titre ne partageaient pas la même grille que les
cartes, et les sections n’avaient pas les mêmes retraits que la référence.

## 2. Corrections

- Racine météo passée en **`ColumnLayout`** (`anchors.fill`, spacing 12).
- En-tête en `RowLayout` avec `Layout.fillWidth`, **`Layout.leftMargin` et
  `Layout.rightMargin` = 16 px**, comme `WeatherTab.qml`.
- Grande carte météo en `Layout.fillWidth` + `Layout.preferredHeight: 150`.
- Cartes de détails en `RowLayout` `Layout.fillWidth` (largeurs égales).
- Titre « Prévisions » avec **`Layout.leftMargin` 12 px** et
  **`Layout.topMargin` 12 px**.
- Prévisions en `RowLayout` `Layout.fillWidth`, cartes à 120 px (hauteur
  suffisante pour le contenu, sans dépasser la vue de 468 px).
- Contenus internes conservés : icône/température/description centrés,
  cartes de détails centrées, textes de prévisions centrés.

Aucun composant partagé, aucun autre onglet, aucune animation globale n’a été
modifié.

## 3. Captures avant / après

| État | Avant | Après |
|---|---|---|
| Weather 100 % | `align2-weather.png` | `weather-fixed.png` |
| Weather 125 % (Qt) | — | `weather-fixed-125.png` |
| Dashboard (non-régression) | `align2-home.png` | `weather-fixed-home.png` |
| Media (non-régression) | `align2-media.png` | `weather-fixed-media.png` |
| Performance (non-régression) | `align2-performance.png` | `weather-fixed-performance.png` |

Mesures OCR après correction (100 %) :

- ville à x=33 (contenu 16 + marge 16), contre x=18 avant ;
- lever/coucher décalés de 16 px vers l’intérieur, contre le bord avant ;
- titre « Prévisions 7 jours » à x=27 (marge 12), contre x=16 avant ;
- 7 cartes de prévisions alignées sur toute la largeur, contenu centré.

Le contenu météo réel a changé entre les captures (12.2 → 12.1 °C, vent
5.4 → 7.9 km/h) sans décalage de layout.

## 4. Tests

- **1920×1080 à 100 %** : Release, capture `weather-fixed.png`, aucune erreur
  QML.
- **1920×1080 à 125 %** : test avec `QT_SCALE_FACTOR=1.25`, capture
  `weather-fixed-125.png` ; marges et alignements conservés, aucune erreur QML.
  (Le DPI système réel n’a pas été changé ; c’est une simulation Qt.)
- **Changement de contenu** : données Open-Meteo actualisées, layout stable
  (hauteurs fixes et contenus centrés).
- **États indisponibles** : les valeurs `--`/`N/D` restent dans les mêmes
  cartes ; le titre et la rangée de prévisions sont masqués si la liste est
  vide, sans déplacer les autres sections.
- **Non-régression** : Dashboard, Media et Performance recapturés
  (`weather-fixed-home/media/performance.png`) ; contenus réels inchangés.
- **Build** : Release et Debug réussissent ; `qrc:`, `TypeError`,
  `ReferenceError`, `polish` et `Cannot specify` absents des logs.
- Taskbar restaurée après fermeture propre (`state 0`, zone `0,0 1920×1032`).

## 5. Limites restantes

- La hauteur du panneau reste fixe à 560 px ; Linux anime la hauteur selon
  l’onglet. Cela laisse un petit espace sous les prévisions, mais les éléments
  sont alignés et non décalés.
- Les états chargement/erreur ne disposent pas d’un bandeau dédié ; les
  cartes conservent leur géométrie et affichent `--`/`N/D`.
