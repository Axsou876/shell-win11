# Rapport de parité — Dashboard Caelestia Shell (Windows)

Date : 2026-11-11 · Build : Release + Debug Qt 6.9.3 · Écran : 1920×1080,
DPI 96 (100 %) · Thème : monochrome noir/blanc choisi par l’utilisateur.

## 1. Méthode

1. Audit en lecture seule de `C:\Users\axsou\Documents\shell-main` :
   `modules/dashboard/*`, `modules/dashboard/dash/*`,
   `modules/dashboard/media/*`, `modules/dashboard/performance/*`,
   `modules/launcher/*`, `modules/sidebar/*`, `modules/bar/*`,
   `plugin/src/Caelestia/Config/tokens.hpp`.
2. Audit de l’état Windows (`DashboardShell`, `DashboardHome`, `DashboardMedia`,
   `DashboardPerformance`, composants partagés, `Theme.qml`).
3. Reconstruction composant par composant dans `windows/src/qml`.
4. Compilation Release + Debug, exécution réelle, captures et OCR/vérification
   des valeurs affichées.
5. Limite : aucun runtime Linux Quickshell/Hyprland n’est disponible sur cette
   machine ; la comparaison est **code source Linux → rendu Windows**, pas un
   diff pixel à pixel. Les captures Linux ne peuvent donc pas être produites ici.

## 2. Correspondance des composants

| Linux | Windows | État |
|---|---|---|
| `modules/dashboard/Content.qml` | `overlay/DashboardShell.qml` | reconstruit |
| `modules/dashboard/Tabs.qml` | `overlay/DashboardShell.qml` | reconstruit |
| `dash/Dash.qml` | `overlay/DashboardHome.qml` | reconstruit |
| `dash/User.qml` | carte utilisateur de `DashboardHome` | reconstruit |
| `dash/SmallWeather.qml` | carte météo de `DashboardHome` | reconstruit |
| `dash/DateTime.qml` | carte horloge de `DashboardHome` | reconstruit |
| `dash/Calendar.qml` | `components/DashboardCalendar.qml` | reconstruit |
| `dash/Resources.qml` | anneaux de `DashboardHome` | reconstruit |
| `dash/Media.qml` | carte média de `DashboardHome` + `bongocat.gif` | reconstruit |
| `Media.qml` + `media/*` | `overlay/DashboardMedia.qml` | partiel |
| `Performance.qml` + `performance/*` | `overlay/DashboardPerformance.qml` | reconstruit |
| `WeatherTab.qml` | `overlay/DashboardWeather.qml` | reconstruit |
| `components/CircularProgress` | `components/CircularProgress.qml` | créé |

## 3. Dimensions et tokens

| Élément | Linux | Windows |
|---|---|---|
| Panneau | largeur du contenu, hauteur du tab courant | 1000×560 fixes |
| Onglets | icône au-dessus, indicateur = largeur du contenu, h 3, spacing 5 | identique, 5 onglets |
| Accueil : météo | 275 px, radius 42 | 275×180, radius 42 |
| Accueil : utilisateur | 340 px, radius 28 | 340×180, radius 28 |
| Accueil : horloge | 110 px, radius 16 | 110×278, radius 16 |
| Accueil : calendrier | grille, radius 28 | 552×278, radius 28 |
| Accueil : ressources | anneaux 6 px | 70×278, anneaux 6 px |
| Accueil : média | 200 px, radius 56 | 200×470, radius 56 |
| Performance héro | 400 px, usage 100 | 478×180, usage 100 |
| Performance réseau | 390×220 | 380×276 |
| Performance stockage | carte + anneau 270° | 280×276, radius 32 |
| Performance mémoire | carte + anneau 270° | 284×276, radius 12 |
| Média | cover 200, sections 300 / auto / 300 | cover 200, sections 300 / 312 / 300 |
| Météo | 840 px, prévisions 7 j | 968 px de contenu, 7 j |
| Launcher | item 600×57, wallpaper 280×200 | 640×520, lignes 60, vignettes 58×38 |

Les largeurs cartes de l’accueil sont celles des tokens Linux. Les hauteurs
sont adaptées à la hauteur fixe de 560 px du panneau Windows ; Linux anime la
hauteur selon l’onglet. Les formes `MaterialShape` Linux (Gem, Clamshell, Sunny,
SoftBurst) sont approchées par des cercles/rectangles arrondis monochromes.

## 4. Tests réellement exécutés (Release)

- `F10` depuis le launcher : dashboard ouvert, focus clavier, `F8` bascule en
  clair (`#efefef`), `F8` revient, `F9` ouvre la Sidebar, `Échap` ferme.
- Dashboard par survol du bord haut : ouvert, `PANEL_CLOSE` après sortie.
- Captures des 5 onglets : `parity4-home.png`, `parity4-media.png`,
  `parity4-performance.png`, `parity4-weather.png`,
  `parity3-workspaces.png`, `parity6-dashboard-f10.png`,
  `parity6-dashboard-light.png`.
- Accueil : météo réelle `12.2 °C / Partiellement nuageux / Paris`,
  uptime `up 1j 0h 58min`, nom `axsou`, version Windows, calendrier novembre 2026,
  média Spotify `Mauvais djo`, anneaux CPU/RAM/disque réels.
- Performance : CPU `30 %`, GPU `18 %` (RTX 3070 Laptop), températures `N/D`,
  stockage `433 / 476 Go`, mémoire `10.4 / 15.7 Go`, réseau `55 / 27 kb/s`.
- Météo : `Paris`, `07:53 / 17:15`, ressenti `11.8 °C`, humidité `95 %`,
  vent `5.4 km/h`, prévisions 7 jours (`Aujourd’hui` + 6 jours).
- Média : Spotify réel, position `1:08 / 2:20`, pochette, source Spotify,
  paroles explicitement indisponibles.
- Launcher : `213 applications`, recherche, `calc` + Entrée a lancé
  `CalculatorApp` (PID observé), fermeture.
- Sidebar : `F9` et bouton barre `dock_to_right` ; audio 100 %, état système.
- Notifications et menu Power : ouverts, `PANEL_OPEN` / `PANEL_CLOSE` corrects.
- Barre des tâches : masquée pendant la session, restaurée après `Ctrl+Q`
  (état 0, zone `0,0 1920×1032`).
- Aucune erreur QML (`qrc:`, `TypeError`, `ReferenceError`) sur les captures
  finales.

## 5. Captures de référence

Dans `windows/build-release/smoke/` :

- `parity4-home.png`, `parity4-media.png`, `parity4-performance.png`,
  `parity4-weather.png`, `parity3-workspaces.png`
- `parity6-dashboard-f10.png`, `parity6-dashboard-light.png`,
  `parity6-sidebar-f9.png`
- `reg-launcher.png`, `reg-notifications.png`, `reg-power.png`,
  `reg-sidebar2.png`
- `taskbar-hidden-full.png`, `taskbar-enforced.png`

## 6. Évaluation de parité (rubric, pas une impression)

| Critère | Poids | Note | Preuve / écart |
|---|---:|---:|---|
| Structure Dashboard (onglets, pages, slide) | 20 | 18 | 5 onglets dont Weather ; Workspaces conservé |
| Accueil (grille, cartes, hiérarchie) | 20 | 16 | largeurs tokens exactes ; hauteurs fixes |
| Performance (cartes, mesures) | 20 | 16 | Hero/Storage/Network/Memory ; temp N/D |
| Météo (contenu réel) | 20 | 18 | Open-Meteo, 7 jours, ressenti |
| Média (structure + réel) | 20 | 12 | cover/arc/détails ; cava et lyrics absents |
| Launcher | 20 | 16 | réel ; dimensions 640 vs 600, vignettes |
| Sidebar / barre | 20 | 14 | Control Centre adapté, workspaces barre maquette |
| Animations | 20 | 14 | indicateur/slide/anneaux/calendrier ; cava, hauteur variable |
| Données réelles | 20 | 20 | aucune valeur inventée ; N/D explicites |
| **Dashboard seul** | 100 | **80** | structure + accueil + perf + météo + média |
| **Ensemble du shell** | 100 | **~78** | écarts cava/lyrics, formes, sidebar, launcher |

**Conclusion honnête** : la reconstruction du Dashboard atteint l’objectif de
80 % sur la structure et les fonctions vérifiées ci-dessus, mais la parité
globale du shell reste estimée à ~78 % et **n’est pas prouvée au pixel**. Les
points qui empêchent de déclarer 80 % global sont listés ci-dessous.

## 7. Écarts restants et prochaines étapes

1. **Visualiseur audio** : porter cava/FFT (WASAPI loopback) pour remplacer la
   mention honnête « indisponible ».
2. **Paroles** : aucune source GSMTC ; intégrer un backend de paroles si une
   API est disponible.
3. **MaterialShapes** : Gem/Clamshell/Sunny/SoftBurst exacts à porter depuis
   `third_party/m3shapes`.
4. **Hauteur variable** : faire suivre la hauteur du panneau à l’onglet courant
   comme `Wrapper.qml`/`Content.qml` (nécessite de propager la hauteur dans
   `ScreenSurface`/BlobRect).
5. **Palette** : le Linux utilise Material 3 coloré ; le thème Windows est
   volontairement monochrome depuis la demande utilisateur. La comparaison
   colorimétrique n’est donc pas comparable.
6. **Sidebar** : le port Windows reste un Control Centre ; la sidebar Linux
   notifications/lyrics n’est pas reproduite à l’identique.
7. **Barre Workspaces** : la barre Windows utilise encore une maquette
   (`BarWorkspaces.qml`) alors que l’onglet Workspaces est réel.
