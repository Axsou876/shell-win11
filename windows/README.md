# Caelestia Shell — Windows 11 (Phase 2.5)

Barre Caelestia intégrée au bureau Windows 11 comme **AppBar réelle sur chaque
écran**, avec fenêtre active réelle, status système réel et **icône System Tray
Caelestia réelle**. Le DPI par écran est géré en Per-Monitor V2.

Windows Explorer et la barre des tâches sont conservés. Le design de la barre
reste celui de la Phase 1.5.

## Prérequis

| Outil | Version testée |
|---|---|
| Windows | 11, build 26300 |
| Visual Studio Build Tools 2022 | 17.14 (MSVC 19.44) |
| Qt | 6.9.3 MSVC 2022 x64 + Qt Shader Tools + Qt SVG |
| CMake | 3.31.6 |
| Ninja | 1.12.1 |

## Compilation

```powershell
powershell -ExecutionPolicy Bypass -File "C:\Users\axsou\Documents\shell-win11\windows\scripts\build.ps1" -Clean -Deploy
```

Lancement :

```powershell
.\build\bin\caelestia-shell.exe
```

Fermeture propre : menu tray → « Quitter Caelestia », `Ctrl+Q`, `Alt+F4` ou
fermeture de la fenêtre.

Un second lancement ne crée ni AppBar ni icône tray supplémentaire : il se
termine immédiatement avec le code 0.

## Architecture

### AppBar multi-écrans
- `Platform::screens` retourne un `QVariantList` de `QScreen*` trié par position
  (X, puis Y, puis nom) ; il ne dépend pas de l'ordre de
  `QGuiApplication::screens()`.
- `Main.qml` crée une `Window` et une AppBar par écran via `Instantiator`.
- Chaque AppBar utilise `ABM_NEW`, `ABM_QUERYPOS`, `ABM_SETPOS` (`ABE_LEFT`),
  `ABM_REMOVE` et `ABN_POSCHANGED` sur son propre HWND.
- Le `HMONITOR` est obtenu directement depuis `QScreen` via
  `QWindowsScreen::handle()`, sans dépendre de `MonitorFromWindow` avant que la
  fenêtre soit positionnée.
- Largeur logique fixe de 60 px, convertie avec `GetDpiForMonitor` /
  `GetDpiForWindow` ; la zone de travail Windows réserve la largeur physique
  correspondante.

### Changements d'écran et DPI
- `QGuiApplication::screenAdded` / `screenRemoved` déclenchent la reconstruction
  du modèle QML au tick suivant, après que Qt a terminé sa gestion interne des
  moniteurs.
- À la déconnexion, l'AppBar de l'écran supprimé est désenregistrée et masquée
  avant la destruction de sa fenêtre.
- À la reconnexion, une nouvelle AppBar est créée et la zone de travail est
  réservée à nouveau.
- `WM_DISPLAYCHANGE`, `WM_DPICHANGED`, `WM_SETTINGCHANGE` et les changements de
  géométrie d'écran rafraîchissent les réservations.
- Per-Monitor DPI Awareness V2 est actif (vérifié sur les HWND Caelestia :
  `awareness=2`, contexte V2).
- Les changements de résolution, de topologie et d'écran principal sont pris en
  charge sans redémarrage.

### Instance unique
Mutex Win32 nommé `Local\CaelestiaShellWindows`, libéré par l'OS même en cas
d'arrêt brutal. Aucune seconde AppBar ni icône tray n'est créée.

### Fenêtre active
`SetWinEventHook` foreground/name/minimize/destroy, `GetWindowTextW`, icône
`WM_GETICON` / classe / `SHGetFileInfoW`.

### Status
`BatteryService` (`GetSystemPowerStatus` + `WM_POWERBROADCAST`),
`NetworkService` (WLAN + IP Helper), `BluetoothService` (WinRT Radio),
`AudioService` (WASAPI + callbacks).

### System Tray
- `TrayService` utilise `Shell_NotifyIconW` :
  - `NIM_ADD`, `NIM_SETVERSION` (`NOTIFYICON_VERSION_4`), `NIM_DELETE` ;
  - `NIM_SETFOCUS` ;
  - `NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP`, GUID stable.
- Icône générée depuis `assets/logo.svg` original, recoloré avec les couleurs
  du thème ; aucun nouveau logo.
- Menu natif `CreatePopupMenu` / `TrackPopupMenuEx` :
  - « Ouvrir la zone de notification » ;
  - « Quitter Caelestia ».
- Callbacks `WM_CONTEXTMENU`, `NIN_SELECT`, `NIN_KEYSELECT`,
  `NIN_BALLOONUSERCLICK`.
- La zone tray de la barre affiche une **seule icône Caelestia réelle** (24 px),
  cliquable pour ouvrir le même menu.

### Limite documentaire du tray
Aucune API Win32 documentée ne permet d'énumérer ou de récupérer les icônes des
autres applications. `Shell_NotifyIcon` gère uniquement l'icône de
l'application appelante ; `Shell_NotifyIconGetRect` exige l'identité de l'icône.
Les interfaces `ITrayNotify` / `ITrayDeskBand` sont non documentées et ne sont
pas utilisées. L'UI Automation ne fournit pas les images des icônes et dépend
d'une arborescence XAML instable ; elle n'est pas utilisée pour simuler le tray.

Les icônes Discord, Steam, Firefox et autres restent donc dans la **zone de
notification Windows native**. La barre Caelestia n'en affiche pas de copies.

## Tests effectués (9 novembre 2026)

| Test | Résultat |
|---|---|
| Build + déploiement + smoke test | OK, exit 0 |
| qmllint (cible CMake `caelestia-shell_qmllint`) | 0 erreur ; 3 warnings d'accès non qualifié sur `Caelestia.Platform` (singletons C++ absents du qmltypes) |
| Démarrage avec deux écrans, DPI différents | 2 AppBars : 75 px physiques (60 logiques, 125 %) et 60 px (100 %) ; zones de travail réservées 75 px / 60 px |
| Démarrage avec un seul écran | 1 AppBar 60×1032 ; zone de travail 60→1920 |
| Déconnexion de DISPLAY1 (`ChangeDisplaySettingsEx`) | application stable, AppBar détruite, zone de travail libérée, aucune exception |
| Reconnexion de DISPLAY1 (`SetDisplayConfig` extend) | AppBar recréée 75×1020 à −1920,0 ; zone de travail re-réservée |
| Changement de résolution DISPLAY5 (1280×720) | AppBar 60×672, zone de travail correcte ; retour 1920×1080 validé |
| Changement d'écran principal (DISPLAY1 en 0,0) | AppBars 75 px / 60 px aux bons bords, zones de travail correctes ; disposition d'origine restaurée |
| Deuxième lancement | exit 0 en ~1 s, un seul processus, aucune AppBar ni icône supplémentaire |
| Fermeture normale (WM_CLOSE) | application terminée, réservations AppBar retirées, zones de travail restaurées |
| Absence de doublons | une seule AppBar visible par écran après chaque événement testé |
| Phases 2.1–2.4 | icône tray enregistrée, status, fenêtre active et rendu QML toujours fonctionnels |

## Tests non effectués

- **Scaling 150 / 175 / 200 %** : Windows ne fournit pas d'API publique pour
  modifier le facteur par moniteur à chaud ; le test nécessite l'interface
  Paramètres et une reconnexion de session. Le code utilise le DPI réel du
  moniteur, mais ces valeurs n'ont pas été validées physiquement.
- **Débranchement physique d'un câble** : la déconnexion/reconnexion a été
  simulée par les API de configuration d'affichage, pas par un débranchement.
- **Veille / reprise et verrouillage de session** : non testés.
- **Arrêt forcé** (`taskkill` / `TerminateProcess`) : non validé ; la
  restauration des zones de travail n'est garantie que pour une fermeture
  normale.
- **Menu tray et commandes** : non rejoués après les changements Phase 2.5 ; le
  code du tray est inchangé et l'icône est enregistrée au démarrage.
- **Icônes tierces dans la barre** : impossible avec les API documentées.

## Bugs / limites connus

- Qt 6.9.3 émet des avertissements `QSGThreadedRenderLoop ... invalid geometry`
  lors des changements d'écran. Ils sont cosmétiques ; aucun crash ni artefact
  visuel observé après la correction.
- Détruire une `QQuickWindow` pendant le traitement interne de suppression
  d'écran de Qt provoquait un crash (`Qt6Gui`). La reconstruction QML est
  maintenant différée d'un tick et l'AppBar supprimée est masquée avant
  destruction ; le scénario déconnexion/reconnexion est validé.
- La déconnexion d'un écran recrée aussi les barres des écrans restants
  (changement de modèle `Instantiator`) ; aucun doublon ni clignotement durable
  observé.
- Les workspaces et le bouton power restent décoratifs (état Phase 2.4).
- Les modules Caelestia majeurs (notifications, launcher, dashboard, OSD, lock,
  session, wallpaper, météo, média, luminosité, clavier, VPN) ne sont pas
  implémentés.

## Captures

- `caelestia-phase25-screen1-125pct.png` : barre sur DISPLAY1 à 125 %.
- `caelestia-phase25-screen2-100pct.png` : barre sur ZOWIE XL LCD à 100 %.
- `caelestia-phase25-secondary-corner.png` : bord gauche de l'écran secondaire.
- `caelestia-tray-real.png`, `caelestia-tray-native.png`,
  `caelestia-tray-overflow.png`, `caelestia-tray-menu-native.png`,
  `caelestia-tray-menu.png`, `caelestia-tray-cleanup.png` : tray (Phase 2.4).
- `caelestia-status-*.png`, `caelestia-activewindow-*.png`,
  `caelestia-appbar*.png` : phases précédentes.

## Phase suivante

Phase 2.6 non commencée. En attente de validation de la Phase 2.5.
