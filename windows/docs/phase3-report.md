# Phase 3 — Desktop Shell : rapport d’implémentation

Branche : `phase3-desktop-shell` (créée depuis `79eaffb`, sans réécriture de l’historique).
Projet Linux : `shell-main` lu uniquement, jamais modifié.

## 1. Architecture finale

```
windows/src/
  main.cpp                              services Qt + modes diagnostics
  platform/platform.*                   AppBar, overlay input, focus, écrans
  platform/services/applauncherservice.* découverte, recherche, favoris, lancement
  platform/services/systemservice.*      CPU, RAM, GPU, réseau, disque, historiques
  platform/services/mediaservice.*       GSMTC C++/WinRT, artwork, contrôles
  platform/services/weatherservice.*     Open-Meteo, configuration ville
  platform/services/audioservice.*       Core Audio, volume, mute
  platform/services/networkservice.*     WLAN API / IPHelper
  platform/services/bluetoothservice.*   Windows.Devices.Radios + set-state
  platform/services/batteryservice.*     GetSystemPowerStatus + événements
  platform/services/powerservice.*       verrouillage/veille/session
  platform/services/userservice.*        nom, photo de compte, version Windows
  platform/services/wallpaperservice.*   fonds d’écran réels + changement Windows
  platform/services/notificationservice.* boîte de notifications du shell
  platform/services/windowsservice.*     fenêtres du bureau actuel (Workspaces)
  qml/overlay/LauncherShell.qml          panneau launcher (applications / >wal)
  qml/overlay/NotificationsShell.qml     panneau notifications + actions
  qml/overlay/DashboardShell.qml         onglets + slide horizontal
  qml/overlay/DashboardHome/Media/Performance/Workspaces.qml
  qml/overlay/PowerMenu.qml              menu vertical droit + confirmation
  qml/overlay/SidebarShell.qml           Control Centre F9
  qml/bar/BarFavorites.qml               dock de favoris dans la barre
  qml/popouts/AudioPanel/NetworkPanel/BluetoothPanel/BatteryPanel.qml
  qml/components/DashCard/Sparkline/Meter/RingGauge/DashboardCalendar/PanelHeader.qml
  src/shaders/blob.*                     panneau launcher ajouté au SDF
```

Le launcher est un panneau `launcher` du `PanelController`, rendu par le même
SDF/spring que les autres surfaces. Son ouverture est explicite (focus clavier),
sa fermeture passe par Échap, le clic extérieur ou le lancement réussi.

## 2. APIs Windows utilisées

| Domaine | API |
|---|---|
| Applications | `IShellLinkW`/`IPersistFile`, `FOLDERID_Programs`, `FOLDERID_CommonPrograms`, `FOLDERID_AppsFolder`, `IShellItem2`/`PKEY_AppUserModel_ID`, `IShellItemImageFactory`, `ShellExecuteExW`, `SHGetFileInfoW` |
| Audio | `IMMDeviceEnumerator`, `IAudioEndpointVolume`, `IMMNotificationClient` |
| Réseau | `WlanOpenHandle`/`WlanEnumInterfaces`/`WlanQueryInterface`, `NotifyIpInterfaceChange`, `GetIfTable2` |
| Bluetooth | `Windows.Devices.Radios` (C++/WinRT), `Radio.SetStateAsync` |
| Batterie | `GetSystemPowerStatus`, `WM_POWERBROADCAST` |
| Performance | `GetSystemTimes`, `GlobalMemoryStatusEx`, `GetIfTable2`, `GetDiskFreeSpaceExW`, DXGI `IDXGIAdapter3::QueryVideoMemoryInfo`, PDH `GPU Engine` |
| Média | GSMTC `GlobalSystemMediaTransportControlsSessionManager`, `TryGetMediaPropertiesAsync`, timeline position/duration, `TryPlayAsync`/`TryPauseAsync`/`TrySkipNextAsync`/`TrySkipPreviousAsync` |
| Profil | `GetUserNameW`, `NetUserGetInfo`, `FOLDERID_AccountPictures`, `QSysInfo` |
| Fond d’écran | `SystemParametersInfoW(SPI_SETDESKWALLPAPER)`, `QStandardPaths::PicturesLocation`, dossiers Themes/Web de Windows |
| Notifications | boîte interne au shell (`NotificationService`) : titre, application, corps, heure, actions |
| Workspaces | `EnumWindows`, `IVirtualDesktopManager`, `DwmGetWindowAttribute`, `SetForegroundWindow` |
| Météo | Open-Meteo Geocoding + Forecast (HTTPS, sans clé API) |
| Session | `LockWorkStation`, `SetSuspendState`, `ExitWindowsEx`, privilège `SeShutdownPrivilege` |

Aucun texte saisi n’est exécuté : le launcher ne lance que des entrées découvertes
et appelle `ShellExecuteExW` sur leur `.lnk` ou `shell:AppsFolder\<AUMID>`.

## 3. État des fonctionnalités

Terminé et testé :

- Launcher par logo, bouton recherche ou F1, recherche fuzzy insensible à la
  casse, navigation clavier, Échap, clic extérieur, favoris, icônes shell,
  lancement réel.
- 213 applications découvertes : Firefox, Discord, Brave, Steam, Bloc-notes,
  Calculatrice, Paramètres, etc.
- Dock vertical dans la barre : **favoris uniquement** (dédupliqués, jusqu’à
  huit icônes réelles, zone défilante), clic pour lancer. Le dock est **masqué
  quand il est vide** (plus d’icône grille), les applications récentes ne sont
  plus ajoutées automatiquement et le bouton calendrier a été retiré.
- Dashboard 4 onglets **Dashboard / Media / Performance / Workspaces** avec
  données réelles : météo, profil Windows (photo de compte, version, uptime),
  calendrier, média, anneaux CPU/GPU/mémoire et fenêtres du bureau actuel.
  Media GSMTC avec progression réelle ; Performance en anneaux ; Workspaces
  liste et active les fenêtres réelles.
- Panneaux Audio/Réseau/Bluetooth/Batterie réels ; Control Centre F9 ; menu
  d’alimentation vertical à droite avec photo de compte et confirmation.
- Launcher `>wal` : 94 fonds d’écran réels trouvés dans Pictures/Themes/Web,
  vignettes, filtre par nom ou dossier, fond actif coché, application réelle au
  bureau Windows par `SystemParametersInfoW`.
- Panneau Notifications : cartes avec application/heure/titre/corps, bouton
  d’action, croix, clic du milieu et balayage horizontal pour fermer ; état vide
  avec bouton de notification de test. Les notifications restent internes au
  shell (pas de capture des toasts d’autres applications).
- Animations de panneaux, launcher, onglets et icônes ; SDF/spring conservés.
- Thème monochrome strict : surfaces noires pures en mode sombre, blanches pures
  en mode clair, accents et conteneurs en gris neutres, aucune teinte colorée.
  Les icônes d’applications, y compris l’icône de la fenêtre active dans la
  barre, les pochettes média et la photo de compte gardent leurs couleurs
  d’origine.
- Rehausse périodique de l’AppBar au-dessus des fenêtres maximisées/borderless ;
  animation verticale des panneaux sans dépassement sous la barre des tâches.

Partiel / non exécuté volontairement :

- Les actions Power destructives (verrouillage, veille, redémarrage, arrêt,
  déconnexion) sont câblées et protégées par confirmation, mais n’ont pas été
  exécutées pendant les tests pour ne pas interrompre la session.
- Le toggle Bluetooth est câblé ; il n’a pas été actionné pour ne pas déconnecter
  les appareils de l’utilisateur.
- Capture des toasts Windows d’autres applications non implémentée : un build
  Win32 non empaqueté n’obtient pas de façon fiable l’accès au listener de
  toasts. Le panneau Notifications est une boîte interne au shell.
- Sélection d’un autre périphérique audio, liste/appairage Bluetooth et profil
  d’alimentation Windows encore dans les paramètres Windows.

## 4. Tests réellement exécutés

### Debug

- `--list-apps` : `TOTAL 213`, Firefox/Discord/Bloc-notes/Calculatrice présents.
- `--launch-app "Calculatrice"` : `CalculatorApp` démarré.
- UI : clic logo → launcher ; saisie `calc` ; Entrée → `CalculatorApp` PID
  observé ; clic direct sur le premier résultat (900,632) → `CalculatorApp`
  PID 11832 ; Échap ferme ; styles `transparent=yes noactivate=yes` après
  fermeture.
- Favori : clic étoile → `launcher/favorites` contient l’ID de Calculatrice ;
  F9 affiche le favori ; état restauré après test.
- Dock : favoris uniquement, masqué quand vide (plus d’icône grille), icônes
  réelles, clic de lancement vérifié ; capture `clean-bar3.png`. Le bouton
  calendrier et les récents automatiques ont été retirés.
- Barre au-dessus d’une fenêtre Notepad maximisée : logo toujours visible.
- Profil : `User.displayName`, `Windows 11 Version 26H2`, hôte `DESKTOP-ALEOSMB`,
  photo de compte générique Windows (`user.bmp`, aucune photo personnelle dans
  AccountPictures). Fenêtres : 6 fenêtres réelles listées via `--dump-windows`
  (Discord, Firefox, Spotify, Terminal, Explorer).
- Dashboard : 4 onglets capturés (Dashboard/Media/Performance/Workspaces) ;
  Media affiche la session Spotify réelle avec progression ; Performance affiche
  les anneaux CPU/GPU/mémoire ; Workspaces liste les fenêtres réelles.
- Menu Power : ouvert à droite, carte de confirmation affichée après clic sur
  Arrêter ; annulation par Échap ; aucune action destructive exécutée.
- Météo : carte Paris sur le Dashboard (Open-Meteo), configuration conservée.
- Media : session Spotify (`Kalash`, Booba) et Media Player
  (`caelestia-test-tone`) détectées ; pause/reprise vérifiées ; artwork en cache.
- Statut : audio `Haut-parleurs (SkillKorp SKP_H21)`, volume 100, mute testé et
  restauré ; Wi-Fi `Livebox-6190` signal 81/66 ; Bluetooth présent/activé ;
  batterie 100 % sur secteur (`minutes=-1`, Windows ne fournit pas d’autonomie).
- Dashboard 100 % ↔ 125 % : ouvertures directes, sans `PANEL_CLOSE`
  intermédiaire ; sortie ferme.
- Transitions directes audio → réseau → Bluetooth : aucune fermeture
  intermédiaire. Balayage rapide : seule la cible finale (Battery) s’ouvre.
- Focus : Discord reste au premier plan pendant l’ouverture hover ; styles
  hover `transparent=no noactivate=yes`.
- Panneaux Audio/Réseau/Bluetooth/Batterie/Power ouverts et capturés, sans
  erreur QML.
- Step 1e conservé : AppBar, clic à travers, `WS_EX_NOACTIVATE`,
  `WS_EX_TRANSPARENT`, D3D11, F8/F9/F10, multi-écrans/DPI.

### Release

- Build `windows/build-release` en `Release` : succès.
- `--dump-status`, `--dump-system`, `--weather Paris`, `--list-apps`,
  `--dump-media`, `--launch-app` : mêmes services, mêmes valeurs réelles.
- UI Release : launcher Firefox capturé ; `calc` + Entrée a lancé
  `CalculatorApp` ; Dashboard 4 onglets ; Audio/Réseau/Batterie ; F9 ;
  menu Power ; dock de la barre ; aucune erreur QML.
- L’onglet Workspaces liste les fenêtres du **bureau virtuel actuel** et permet
  de les activer ; il n’affiche pas encore de miniatures de tous les bureaux.
- Launcher `>wal` Release : `94 fonds d’écran`, vignettes et filtre vérifiés ;
  `Entrée` a appliqué une image réelle au bureau Windows (valeur WallPaper du
  registre changée et capture du bureau), puis le fond d’origine a été restauré.
- Notifications Release : état vide, création (`Notifications · 1`), carte
  complète, bouton `Another action` (fermeture), clic du milieu et balayage
  horizontal (`Notifications · 0`) vérifiés ; aucune erreur QML.
- Thème monochrome Release : launcher sombre `#000000`, launcher clair
  `#fefefe/#ffffff`, notifications `#0a0a0a`, barre `#000000`, texte
  `#ffffff/#000000` ; aucune couleur d’accent dans le thème (pixels neutres).
- Icônes d’applications Release : Firefox reste coloré dans l’icône de fenêtre
  active en mode blanc et sombre (recoloration au thème supprimée) ; captures
  `firefox-icon-light.png`, `firefox-icon-dark.png`.
- Nouveautés Release testées à 100 % (`GetDpiForWindow` = 96) ; à 125 %, les
  captures de l’interface précédente restent valides, mais les deux nouveaux
  panneaux n’ont pas été rejoués après un changement d’échelle système.
- Non-régression Release à 100 % : AppBar 60 px réservé, fenêtre Notepad
  maximisée à `(52,-8) 1876×1048` (débord 8 px normal), rail visible au-dessus ;
  overlays fermés `noactivate=yes transparent=yes` ; aucune erreur QML.
- CPU idle mesuré : 0,38 s sur 5 s / 8 cœurs ≈ 0,9 % ; working set ≈ 451 Mo.

## 5. Captures

`windows/build/phase3-captures/` :

- `redesign-dashboard.png`, `redesign-media.png`,
  `redesign-performance.png`, `redesign-workspaces.png`,
  `redesign-power.png`, `redesign-power-confirm.png`
- `final-launcher-firefox.png`, `final-bar-dock.png`, `bar-dock.png`
- `final-dashboard-home.png`, `final-dashboard-media.png`,
  `final-dashboard-performance.png`, `final-dashboard-weather.png`
- `dashboard-weather-config.png`
- `final-panel-audio.png`, `final-panel-network.png`,
  `final-panel-battery.png`, `final-power.png`, `panel-bluetooth.png`
- `full-battery-fixed.png`, `bar-over-maximized.png`
- `final-sidebar.png`, `sidebar-favorite.png`
- `wallpaper-launcher.png`, `wallpaper-applied.png`, `notifications-panel2.png`,
  `notifications-added.png`, `notifications-added2.png`
- `windows/build-release/smoke/` : `release-wallpaper.png`,
  `release-notif-empty.png`, `release-notif-created.png`,
  `release-notif-swiped.png`, `release-notif-middle.png`,
  `release-notif-action.png`, `mono-dashboard.png`, `mono-launcher.png`,
  `mono-launcher-light2.png`, `mono-notifications.png`, `mono-bar.png`,
  `firefox-icon-light.png`, `firefox-icon-dark.png`

Logs : `C:\Users\axsou\AppData\Local\Temp\opencode\phase3-final-ui.err.log`,
`phase3-nonreg.err.log`, `phase3-rapid.err.log`.

## 6. Limites restantes

- Pas de capture des notifications toast d’autres applications ; la boîte de
  notifications est interne au shell (avec notification de test).
- Pas de sélection du périphérique audio (endpoint par défaut seulement).
- Pas de liste/appairage Bluetooth ni de choix du profil d’alimentation dans le
  shell (les paramètres Windows restent la référence).
- Les actions Power destructives n’ont pas été exécutées en test.
- Météo dépend d’Open-Meteo et d’une connexion réseau ; une seule ville.
- Certaines applications UWP n’exposent pas d’icône via le shell ; icône
  générique de secours.
- Les températures CPU/GPU ne sont pas exposées par une API Windows fiable ici ;
  les anneaux affichent l’utilisation et la mémoire réelles, pas des degrés
  inventés.
- La photo de profil affichée est celle de Windows si elle existe, sinon la
  photo générique du système (`user.bmp`).
- HDR, contraste élevé, tactile/stylet et masquage fullscreen non traités.
- Le warning QSG préexistant sur la géométrie des fenêtres AppBar subsiste.

## 7. Garanties

- `shell-main` n’a pas été modifié.
- Aucune fonctionnalité Step 1e supprimée : AppBar, focus, hover, transitions,
  D3D11, BlobRect, multi-écrans, DPI, F8/F9/F10, Échap.
- Commit final : voir `git log` sur `phase3-desktop-shell`.
