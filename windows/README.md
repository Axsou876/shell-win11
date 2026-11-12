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
`NetworkService` (WLAN + IP Helper), `BluetoothService` (WinRT Radio pour
l'adaptateur, `bthprops` pour les appareils), `AudioService` (WASAPI +
callbacks, sortie et micro), `PowerProfileService` (overlay d'alimentation,
repli sur les power schemes), `IdleInhibitService` (demandes d'éveil système et
écran).

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

## Tiling Windows (Phase 3)

Le shell inclut un gestionnaire de fenêtres réel inspiré de Hyprland :
layout Dwindle par défaut, Master, Split horizontal/vertical, tiling strict,
floating, focus directionnel, déplacement entre bureaux et coupe-circuit.
Les raccourcis `SUPER+…` de Caelestia sont remplacés par des alternatives
`Ctrl+Alt+…` lorsque Windows les réserve ; `Ctrl+Alt+Shift+T` active/désactive
le tiling et `Ctrl+Alt+Shift+Escape` restaure les fenêtres. Détails :
`docs/hyprland-window-management-report.md`.

## OSD volume et luminosité

Le pilule OSD de Caelestia (volume, luminosité, message transitoire) est
réimplémentée avec les services Windows natifs.

- **Luminosité** — `BrightnessService` pilote le rétroéclairage du moniteur
  principal en DDC/CI (`GetVCPFeatureAndVCPFeatureReply` / `SetVCPFeature`,
  code VCP `0x10`), comme `brightnessctl` sous Linux. Les appels VCP sont des
  transactions I2C lentes (~100 ms) : lecture et écriture tournent sur un
  worker `QtConcurrent`, la valeur QML est publiée de façon optimiste puis
  réconciliée avec la relecture du moniteur (certains panneaux arrondissent).
  Un seul worker à la fois : les demandes suivantes sont coalescées et seule la
  dernière valeur en attente est écrite. Une relecture automatique toutes les
  20 s garde l'état aligné si le rétroéclairage change par un autre moyen.
- **Volume** — l'OSD n'est affiché que pour les actions déclenchées par le
  shell (raccourcis ci-dessous, appel QML explicite). Il n'écoute volontairement
  pas `AudioService::changed` : les touches média matérielles font déjà
  apparaître l'OSD natif de Windows, et le refléter produirait deux indicateurs
  pour un seul geste.
- **Rendu** — `OsdSurface.qml`, en haut à droite de l'écran principal, dans
  l'overlay click-through (il ne prend jamais l'input). Il se décale de la
  largeur de la sidebar quand celle-ci est ouverte.

Raccourcis globaux (`RegisterHotKey`, `Ctrl+Alt+…` afin de ne pas heurter les
`Ctrl+Alt+Flèches/HJKL` du tiling) :

| Raccourci | Action |
|---|---|
| `Ctrl+Alt+F12` | Luminosité +5 % |
| `Ctrl+Alt+F11` | Luminosité −5 % |
| `Ctrl+Alt+F10` | Volume +5 % |
| `Ctrl+Alt+F9` | Volume −5 % |

Les raccourcis non disponibles (déjà pris par un pilote ou un autre logiciel)
sont ignorés silencieusement ; `CAELESTIA_DEBUG_OSD=1` les journalise.
`CAELESTIA_DEBUG_BRIGHTNESS=1` journalise chaque état DDC/CI.

## Sélection des périphériques audio

`AudioService` énumère les endpoints actifs (`IMMDeviceEnumerator::
EnumAudioEndpoints`, `DEVICE_STATE_ACTIVE`) et expose deux listes `{ id, name }`
— `outputs` et `inputs` — plus les ids par défaut `defaultOutputId` /
`defaultInputId`. Le popout Audio affiche chaque liste en `M3ListItem`
sélectionnable, comme les `Audio.sinks` / `Audio.sources` du popout Linux.

Le changement de périphérique par défaut n'a pas d'API documentée. Le service
utilise `IPolicyConfig` (CLSID `870af99c-…`, interface
`f8679f50-850a-41cf-9c72-430f290290c8`), l'interface employée par Explorer et
par SoundSwitch/nircmd. Elle est déclarée avec le vtable complet dans
`audioservice.cpp` et appliquée aux trois rôles Windows (`eConsole`,
`eMultimedia`, `eCommunications`) : ne changer que `eConsole` laisserait les
lecteurs multimédias sur l'ancienne sortie.

Les notifications `IMMNotificationClient` (ajout, retrait, changement d'état,
renommage, changement de défaut) rafraîchissent listes et défauts ; les
callbacks COM arrivent sur un thread worker et repassent par
`QMetaObject::invokeMethod` avant de toucher l'état visible en QML.

`CAELESTIA_DEBUG_STATUS=1` journalise le nombre de sorties/entrées détectées.

## Appairage Bluetooth

`BluetoothService` combine deux API, chacune pour ce qu'elle fait le mieux :

- **Radio** — WinRT `Windows.Devices.Radios` fournit l'état de l'adaptateur et
  son activation/désactivation (`SetStateAsync`), plus une notification
  `StateChanged` quand un autre logiciel ou le mode avion le bascule. WinRT
  exige son propre appartement COM, d'où un thread worker dédié.
- **Appareils** — l'énumération WinRT des appareils Bluetooth ne retourne rien
  pour les périphériques classiques tant que l'application n'est pas empaquetée
  (capacités `bluetooth` absentes du manifeste). Le service utilise donc
  `bluetoothapis.h` (`bthprops`), qui fonctionne pour un exécutable non
  empaqueté : `BluetoothFindFirstDevice` (`fReturnRemembered`,
  `fReturnAuthenticated`, `fReturnConnected`), `BluetoothAuthenticateDeviceEx`
  pour l'appairage et `BluetoothRemoveDevice` pour la dissociation.

Toutes les commandes passent par une file traitée par le worker
(`SetRadio`, `Refresh`, `Scan`, `Pair`, `Unpair`) : une recherche classique
bloque plusieurs secondes et ne peut jamais tourner sur le thread GUI. Chaque
résultat est republié via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`.

Le popout Bluetooth affiche `Bluetooth.devices` (`{ address, name, connected,
paired }`), un bouton **Rechercher** (`scanDevices`, enquête d'environ 5 s) et
un bouton d'action par ligne — **Appairer** ou **Dissocier**, la ligne entière
étant cliquable. Les échecs (appairage refusé, adaptateur verrouillé par une
stratégie) remontent dans `toggleFailed` et s'affichent en rouge dans le popout
plutôt que d'échouer silencieusement.

Limites assumées :

- L'enquête Win32 est l'enquête Bluetooth **classique** ; les appareils
  exclusivement BLE non encore appairés n'apparaissent pas. Ils restent
  appairables depuis les Paramètres Windows, et le popout y renvoie.
- `BluetoothAuthenticateDeviceEx` déclenche l'UI d'appairage sécurisé de
  Windows quand un code doit être confirmé. Les appareils à PIN fixe échouent
  avec le message Win32 d'origine.

`CAELESTIA_DEBUG_STATUS=1` journalise l'état de la radio et le nombre
d'appareils à chaque rafraîchissement.

## Mode d'alimentation

`PowerProfileService` expose le sélecteur à trois positions
Économie d'énergie / Équilibré / Performances du popout Batterie — le
`PowerProfiles.profile` du shell Linux.

Windows superpose deux mécanismes, et le service suit délibérément le premier :

- **Overlay** (mode d'alimentation, ce que montre l'application Paramètres de
  Windows 11). C'est le vrai équivalent des trois profils Linux. Il est appliqué
  par `PowerSetActiveOverlayScheme` (`powrprof.dll`), résolu dynamiquement par
  `GetProcAddress` : la fonction n'est pas exportée par toutes les versions et
  n'est pas déclarée dans les en-têtes du SDK.
- **Power schemes** (modes de gestion de l'alimentation du panneau classique).
  Énumérés par `PowerEnumerate` / `ACCESS_SCHEME`, nommés par
  `PowerReadFriendlyName` et activés par `PowerSetActiveScheme`. Ce chemin sert
  de repli quand l'overlay n'est pas disponible.

L'état courant de l'overlay se lit dans
`HKLM\SYSTEM\CurrentControlSet\Control\Power\User\PowerSchemes`
(`ActiveOverlayAcPowerScheme`, ou `ActiveOverlayDcPowerScheme` sur batterie).
`PowerGetActiveOverlayScheme` n'existe qu'à partir de Windows 11 22621 et
`PowerGetEffectiveOverlayScheme` décrit ce que produit le *plan* actif, pas ce
que l'utilisateur a choisi : la valeur de registre est la seule lecture
portable. Elle est relue toutes les 5 s pour suivre un changement fait dans
Windows.

Limite assumée : le plan et l'overlay restent deux réglages distincts. Le shell
affiche l'overlay, comme le fait l'application Paramètres ; un plan
« Haute performance » choisi dans le panneau classique n'apparaît donc pas dans
le popout.

`CAELESTIA_DEBUG_STATUS=1` journalise le mode retenu et le profil actif.

## Utilitaires (garder éveillé, micro, ne pas déranger)

La sidebar reprend la carte « Keep awake » et la rangée de bascules rapides du
module `utilities` de Caelestia.

- **Garder éveillé** — `IdleInhibitService` crée une demande d'éveil
  (`PowerCreateRequest` + `PowerSetRequest` en `PowerRequestSystemRequired` et
  `PowerRequestDisplayRequired`), l'équivalent Windows d'`Inhibit` sur
  `org.freedesktop.login1`. La demande apparaît dans `powercfg /requests`, et
  l'OS la libère de lui-même si le processus disparaît. Le service la retire
  explicitement à la fermeture, et les deux requests sont annulées ensemble si
  Windows n'en accepte qu'une. Un refus est remonté par `actionFailed` et
  affiché sous la ligne : la bascule ne reste jamais allumée à tort.
- **Micro** — `AudioService` expose `micMuted` et `toggleMicMute()` sur
  l'endpoint de capture par défaut (`IAudioEndpointVolume::SetMute`). Un
  `IAudioEndpointVolumeCallback` dédié garde la bascule alignée sur le
  raccourci Windows et sur les autres applications. `refreshDefaultInput()`
  réactive l'endpoint uniquement quand le défaut change, pour ne pas
  réenregistrer le callback à chaque notification de périphérique.
- **Silencieux** — `NotificationService::doNotDisturb` coupe les toasts du
  shell sans vider la boîte de réception, comme
  `PersistentState.doNotDisturb` sous Linux. L'état est persisté dans
  `QSettings` (`notifications/doNotDisturb`).
- **Bluetooth** — la bascule rapide appelle `BluetoothService::setEnabled`,
  déjà utilisé par le popout.

La sidebar défile (`Flickable`) quand la somme des modules dépasse la hauteur de
l'écran ; la colonne de cartes est scrollable à la molette.

`CAELESTIA_DEBUG_STATUS=1` journalise `Idle inhibit: on|off` avec l'heure, le
mutisme du micro et son endpoint.

## Fenêtre active (window info)

`WindowInfoPanel.qml` reprend `modules/windowinfo` : aperçu, détails et actions
sur la fenêtre active. Il s'ouvre en cliquant le titre de la fenêtre active
dans la barre.

- **Suivi** — `WindowService` (singleton `AppWindows`) décrit la dernière
  fenêtre externe au premier plan. Ouvrir un panneau déplace le focus vers le
  shell : `Platform.activeWindowId` conserve le HWND de la dernière fenêtre
  externe et `trackFocused(true, id)` l'ensemence. La sonde (700 ms) et la
  capture ne tournent que panneau ouvert, jamais pendant toute la session.
- **Détails** — `GetWindowThreadProcessId`, `QueryFullProcessImageNameW`,
  `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` (le cadre perçu, sans la
  bordure de redimensionnement invisible), `GetWindowPlacement` pour une
  fenêtre réduite, `MonitorFromWindow` + `MONITORINFOEXW.szDevice`,
  `GetDpiForWindow`, `IsZoomed` / `IsIconic`, `WS_EX_TOPMOST` ; l'état « Plein
  écran » est déduit d'un cadre qui couvre le moniteur.
- **Aperçu** — `PrintWindow(PW_RENDERFULLCONTENT)` dans un DIB 32 bits
  top-down, exécuté sur un worker `QtConcurrent` : une capture de fenêtre
  agrandie prend des dizaines de millisecondes et figerait l'animation du
  panneau sur le thread GUI. `WindowPreviewProvider` sert la dernière image
  sous `image://windowpreview/<révision>` ; la révision change à chaque capture
  et `cache: false` empêche `QQuickPixmapCache` de grossir d'une frame toutes
  les 700 ms.
- **Actions** — **Réduire** (`ShowWindow(SW_MINIMIZE)`, panneau épinglé 1,5 s
  pour qu'il continue de décrire la fenêtre qui vient d'être réduite au lieu de
  suivre celle que Windows active ensuite) ; **Fermer**
  (`PostMessage(WM_CLOSE)`, l'application garde la main pour demander une
  confirmation).
- **Flotter / Ancrer** — le gestionnaire de fenêtres de la Phase 3 possède la
  géométrie : une fenêtre pavée est re-disposée au rescan suivant, donc un
  `ShowWindow(SW_MAXIMIZE)` brut serait annulé en moins d'une seconde. Le
  bouton appelle `WindowManager.setFloating(id, …)`, qui détache la fenêtre du
  layout — ou la lui rend — sur le thread du gestionnaire.
  `WindowManager.floating` publie la liste des HWND flottants, ce qui garde le
  libellé du bouton et la ligne « État » exacts, y compris quand le raccourci
  `Win+Alt+Space` fait la même opération. C'est l'équivalent du bouton
  Float/Tile de `windowinfo/Buttons.qml` ; Linux n'a pas d'action « agrandir »
  puisque Hyprland pave toutes les fenêtres.

Limites assumées :

- Une fenêtre réduite n'a pas d'aperçu (Windows ne compose plus son contenu) ;
  l'icône de repli s'affiche.
- `PrintWindow` ne restitue pas le contenu protégé (DRM, certaines surfaces
  UWP en composition exclusive) : l'icône de repli s'affiche.
- Pas de « déplacer vers un bureau » dans le panneau (le gestionnaire le fait
  par raccourci) ni de « pin » : `WS_EX_TOPMOST` n'épingle pas une fenêtre sur
  tous les bureaux virtuels comme Hyprland.

`CAELESTIA_DEBUG_STATUS=1` journalise chaque changement de fenêtre suivie
(titre, adresse, taille) et chaque action (`Window action: …`).

## Sélecteur de zone (capture d'écran)

`overlay/AreaPicker.qml` reprend `modules/areapicker`. `Ctrl+Alt+S` ouvre le
sélecteur et enregistre la zone ; `Ctrl+Alt+X` la copie dans le
presse-papiers. `Win+Shift+S` reste à l'outil Windows : le shell enregistre des
raccourcis globaux `RegisterHotKey`, donc pas de conflit à gérer.

- **Gel de l'écran** — `ScreenCaptureService` capture chaque moniteur avec GDI
  (`BitBlt` depuis le DC de l'écran, `SRCCOPY | CAPTUREBLT`) *avant* qu'aucune
  fenêtre du sélecteur n'existe. C'est ce qui empêche l'overlay de se
  photographier lui-même et ce qui rend la sélection exacte : la zone
  enregistrée vient de la même image que celle affichée. La version Linux
  propose aussi un mode « live » (capture au relâchement) ; sous Windows le gel
  est systématique, car capturer après avoir masqué l'overlay rouvre une course
  avec le compositeur et perd l'état affiché.
- **Une fenêtre par écran** — l'`Instantiator` crée un `Window` transparent par
  moniteur capturé, alimenté par `Platform.fitWindowToScreen` :
  `QWindow::setScreen` d'abord, puis la géométrie `QScreen::geometry()`.
  L'ordre compte — une fenêtre `Qt::Tool` naît sur l'écran principal et Qt
  résout son DPI à la création ; sans `setScreen`, un overlay de l'écran à
  125 % est mis en page avec l'échelle de l'écran à 100 %.
- **Image** — `Screenshots.captureScreen(screen)` renvoie un jeton ;
  `image://screencapture/<jeton>` sert les octets via un
  `QQuickImageProvider` (le jeton est la partie avant un éventuel `-`, réservé
  à casser le cache). `cache: false` évite que `QQuickPixmapCache` conserve une
  copie de la capture après la fermeture.
- **Coordonnées** — les rectangles X11 de Windows sont en pixels physiques,
  alors qu'un `Window` Qt raisonne en DIP. Le service reçoit donc la taille de
  la vue QML et convertit par `largeurImage / largeurVue` : le rapport (1,25
  sur un écran à 125 %) tombe juste sans jamais consulter `devicePixelRatio`.
  C'est aussi ce qui rend un test externe trompeur : `GetWindowRect` appelé
  depuis un processus *DPI-unaware* renvoie une fenêtre pleine largeur comme
  1536×864 alors qu'elle fait réellement 1920×1080.
- **Survol** — `Screenshots.windowAt` fait un `EnumWindows` du haut vers le
  bas et retient la première fenêtre visible non cloaked
  (`DWMWA_CLOAKED`), hors processus du shell, hors bureau, qui contient le
  point. `WindowFromPoint` est inutilisable ici : le point tombe toujours sur
  l'overlay. Le rectangle est celui de `DWMWA_EXTENDED_FRAME_BOUNDS`, sans la
  bordure de redimensionnement invisible, donc la sélection correspond à ce que
  l'utilisateur voit. Un clic sans glissement enregistre ainsi la fenêtre
  cliquée, comme `checkClientRects` sous Hyprland.
- **Enregistrement** — `Pictures\Screenshots\Capture yyyy-MM-dd HH-mm-ss.png`
  (`QStandardPaths::PicturesLocation`). La version Linux remet la capture à
  `swappy` pour l'annotation ; Windows n'a pas d'équivalent intégré, le fichier
  est écrit directement. Une notification donne le chemin, ou signale l'échec.
- **Annulation** — `Échap` (touche locale *et* raccourci global enregistré par
  le premier overlay, pour couvrir un sélecteur ouvert alors qu'une autre
  fenêtre garde le focus), clic droit, ou une sélection vide. Fermer rend le
  focus à la dernière fenêtre externe (`Platform.restoreLastExternalWindow`),
  sinon Windows active la fenêtre suivante dans l'ordre Z — souvent la console
  de lancement.

Validé sur le bureau réel, double écran 125 % / 100 % : `Ctrl+Alt+S` puis
glissement enregistre un PNG (400×400 px pour 400 px glissés sur l'écran à
125 %, la résolution physique est donc conservée) ; `Ctrl+Alt+X` place la même
zone dans le presse-papiers ; `Échap`, clic droit et sélection vide ferment le
sélecteur et rendent le focus ; `--smoke-test` sort en 0 sans alerte QML.

Limites assumées :

- Pas de mode « live » ni d'éditeur d'annotation (voir ci-dessus).
- La capture GDI ne lit pas les surfaces protégées par DRM : ces zones
  apparaissent noires, comme dans l'aperçu du panneau « Fenêtre active ».
- La sélection par survol ignore les fenêtres UWP cloaked (autre bureau
  virtuel, application suspendue) et les fenêtres réduites.

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
- Les workspaces de la barre sont réels (Phase 3, VirtualDesktopService) ; le bouton power reste décoratif (état Phase 2.4).
- Modules Caelestia encore absents : écran de verrouillage, session/login,
  clavier, VPN, panneau de réglages (Nexus), enregistreur d'écran.
  Notifications, launcher, dashboard, météo, média, wallpaper, OSD, volume,
  luminosité, sélection audio, appairage Bluetooth, mode d'alimentation,
  utilitaires, info fenêtre et sélecteur de zone sont implémentés.
- Luminosité DDC/CI : dépend du moniteur. Les dalles d'ordinateur portable
  interne et certains écrans en DisplayPort/USB-C n'exposent pas le code VCP
  `0x10` ; l'OSD affiche alors « Luminosité indisponible ».

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

Le tiling et les modules documentés ci-dessus sont en place. Restent, dans
l'ordre prévu : complétion du launcher (calculatrice, actions), dock de
notifications de la sidebar, écran de verrouillage, panneau de réglages
(Nexus).

L'enregistreur d'écran Linux (`utilities/cards/Record.qml`,
`RecordingList.qml`) n'est volontairement pas porté : aucune API Win32
documentée ne fournit l'équivalent de `wf-recorder`.
