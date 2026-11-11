# Audit de parité — Dashboard Caelestia Linux → Windows

Référence Linux : `C:\Users\axsou\Documents\shell-main` (lecture seule).
Port Windows : `C:\Users\axsou\Documents\shell-win11\windows`.

## 1. Composition réelle du Dashboard Linux

| Élément | Fichier source | Dimensions / tokens |
|---|---|---|
| Panneau | `modules/dashboard/Wrapper.qml` | largeur de contenu, hauteur du tab courant, `offsetScale` animé, décalage `-height-5` à la fermeture |
| Onglets | `modules/dashboard/Tabs.qml` | icône au-dessus du label, indicateur largeur du contenu actif, hauteur 3, espacement 5, séparateur 1 px |
| Contenu | `modules/dashboard/Content.qml` | marges 16, onglets Dashboard / Media / Performance / Weather, slide horizontal + drag |
| Accueil | `modules/dashboard/Dash.qml` | GridLayout 6 colonnes, espacement 12 |
| Utilisateur | `dash/User.qml` | 340×168, PFP pilule, logo gem, chip WM, chip uptime, bulles |
| Météo | `dash/SmallWeather.qml` | 275 px, icône + température + description centrés |
| Heure | `dash/DateTime.qml` | 110 px, HH ••• MM vertical, 28 pt |
| Calendrier | `dash/Calendar.qml` | grille mois, navigation chevrons, aujourd'hui forme Sunny, animations de mois |
| Ressources | `dash/Resources.qml` | 3 anneaux CPU / RAM / Disque, trait 6 px |
| Média accueil | `dash/Media.qml` | 200 px, pochette + arc de progression 180°, titre/album/artiste, 3 boutons |
| Média onglet | `Media.qml` + `media/*` | 1000×320, CoverVisualiser 300, détails, paroles 300, fond décoratif |
| Performance | `Performance.qml` + `performance/*` | HeroCard CPU/GPU 400 px, Storage, Network 390×220, Memory, BatteryTank 150×160 |
| Météo onglet | `WeatherTab.qml` | 840 px, ville/date, sunrise/sunset, grande carte, 3 détails, prévisions |

Tokens utiles (`plugin/src/Caelestia/Config/tokens.hpp`) :
`tabIndicatorHeight 3`, `tabIndicatorSpacing 5`, `userWidth 340`, `weatherWidth 275`,
`dateTimeWidth 110`, `mediaWidth 200`, `mediaCoverArtSize 200`, `mediaTabWidth 1000`,
`mediaTabHeight 320`, `mediaSectionWidth 300`, `perfHeroCardWidth 400`,
`perfNetworkCardWidth 390`, `perfNetworkCardHeight 220`, `perfBattWidth 150`.

Palette Linux : `Colours.palette` (Material 3, couleurs du thème) + `Colours.tPalette`
pour les surfaces transparentes. Rayons : small 8, medium 12, large 16,
largeIncreased 20, extraLarge 28; `extraLarge*1.5` météo accueil et `extraLarge*2`
média accueil.

## 2. État Windows avant reconstruction

| Élément | Fichier | Écart principal |
|---|---|---|
| Onglets | `DashboardShell.qml` | icône et label sur une ligne, indicateur largeur de colonne — Linux les empile et adapte l'indicateur au contenu |
| Accueil | `DashboardHome.qml` | cartes 220/358/220 puis calendrier/système, pas la grille Linux ; pas de chip WM ni d'anneaux ressources |
| Calendrier | `DashboardCalendar.qml` | pas de navigation mensuelle ni de marqueur Sunny |
| Performance | `DashboardPerformance.qml` | 3 anneaux au lieu des cartes Hero/Storage/Network/Memory |
| Média | `DashboardMedia.qml` | visualiseur circulaire + colonne unique ; pas la structure CoverVisualiser / détails / paroles |
| Météo | `DashboardHome` carte seule + `DashboardWeather.qml` orphelin | pas d'onglet Météo actif |
| Dimensions | `Theme.qml` | 854×560 fixes, alors que le contenu Linux demande ~1000 px |

## 3. Décisions de reconstruction Windows

- Largeur du panneau : **1000 px** (largeur de l'onglet Média Linux), contenu utile 968 px.
- Hauteur : 560 px conservée pour ne pas casser le SDF/BlobRect et le hover existant.
- Onglets : Dashboard, Media, Performance, **Weather**, Workspaces (5e onglet conservé
  pour ne pas supprimer la fonctionnalité Windows déjà validée).
- Accueil : grille reconstruite avec les proportions Linux :
  météo 275×180, utilisateur 340×180, média 200×470, heure 110×278,
  calendrier 552×278, ressources 70×278, espacements 12.
- Performance : HeroCard CPU + GPU (si disponible), Storage, Network, Memory ;
  BatteryTank seulement si une batterie est réellement présente.
- Média : CoverVisualiser avec pochette + arc réel, détails réels GSMTC, panneau
  paroles honnêtement indisponible (GSMTC n'expose pas les paroles).
- Météo : onglet réel Open-Meteo (ville, température, ressenti, humidité, vent,
  lever/coucher, prévisions 5 jours) ; configuration de ville si absente.
- Données : uniquement les services Windows existants ; aucune valeur inventée.
  Températures CPU/GPU : état `N/D` faute d'API fiable.
- Animations : indicateur d'onglet et slide conservés, animations de mois,
  anneaux animés, transitions d'opacité ; SDF/spring/BlobRect intacts.

## 4. Tableau de suivi

| Composant | Original analysé | Windows implémenté | Visuel validé | Fonctionnel validé |
|---|---|---|---|---|
| Dashboard (structure) | Wrapper/Content/Tabs | oui : onglets empilés, indicateur contenu, slide | oui (parity4/6) | oui (F10, clics, Échap) |
| Accueil | Dash + dash/* | oui : grille 6 zones, PFP, chips, anneaux, média | oui (parity4-home) | oui (services réels) |
| Launcher | modules/launcher | existant, barre + `>wal` | oui (reg-launcher) | oui (213 apps, `calc` lancé) |
| Sidebar | modules/sidebar | Control Centre adapté, bouton barre + F9 | oui (reg-sidebar) | oui (audio/état) |
| Media | Media + media/* | pochette + arc + détails + paroles N/D | oui (parity4-media) | partiel (pas de cava/lyrics) |
| Performance | Performance + performance/* | Hero CPU/GPU + Stockage/Réseau/Mémoire | oui (parity4-performance) | oui (mesures réelles, temp N/D) |
| Weather | WeatherTab | onglet réel Open-Meteo, 7 jours | oui (parity4-weather) | oui (ville, ressenti, lever/coucher) |
| Audio | popouts/AudioPanel | existant | validé phase 3 | validé phase 3 |
| Réseau | popouts/NetworkPanel | existant | validé phase 3 | validé phase 3 |
| Bluetooth | popouts/BluetoothPanel | existant | validé phase 3 | validé phase 3 |
| Batterie | popouts/BatteryPanel | existant | validé phase 3 | validé phase 3 |
| Animations | Tabs/Dash/Media/Performance | indicateur, slide, anneaux, fondu calendrier | oui | partiel (pas de cava) |

## 5. Limites honnêtes prévues

- Les `MaterialShape` Linux (Gem, Clamshell, Sunny, SoftBurst, etc.) n'ont pas
  d'équivalent QML direct dans le port Windows : les formes sont approchées par
  des rectangles arrondis/cercles monochromes, sans introduire de fausses données.
- Le visualiseur audio Linux dépend de cava (FFT temps réel). Windows n'a pas
  encore de capture FFT : la pochette garde son arc de progression réel, sans
  barres inventées.
- Les paroles ne sont pas exposées par GSMTC : panneau « Paroles indisponibles ».
- Les températures CPU/GPU restent `N/D` (aucune API Windows fiable).
- La hauteur variable par onglet de Linux n'est pas encore portée ; le panneau
  reste à 560 px pour préserver le SDF et le hover.
