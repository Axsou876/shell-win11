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
  qml/overlay/LauncherShell.qml          panneau launcher
  qml/overlay/DashboardShell.qml         onglets + slide horizontal
  qml/overlay/DashboardHome/Media/Performance/Weather.qml
  qml/overlay/SidebarShell.qml           Control Centre F9
  qml/bar/BarFavorites.qml               dock favoris/récents dans la barre
  qml/popouts/AudioPanel/NetworkPanel/BluetoothPanel/BatteryPanel/PowerPanel.qml
  qml/components/DashCard/Sparkline/Meter/PanelHeader.qml
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
| Média | GSMTC `GlobalSystemMediaTransportControlsSessionManager`, `TryGetMediaPropertiesAsync`, `TryPlayAsync`/`TryPauseAsync`/`TrySkipNextAsync`/`TrySkipPreviousAsync` |
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
- Dock vertical dans la barre : favoris puis applications récentes
  (dédupliqués, jusqu’à huit icônes réelles, zone défilante), clic pour lancer.
  Boutons recherche et calendrier/Dashboard.
- Dashboard 4 onglets avec données réelles ; Media GSMTC ; Performance
  CPU/RAM/GPU/réseau/disque avec historiques ; Weather Open-Meteo configurable.
- Panneaux Audio/Réseau/Bluetooth/Batterie réels ; Control Centre F9 ; Power
  avec confirmation.
- Animations de panneaux, launcher, onglets et icônes ; SDF/spring conservés.
- Rehausse périodique de l’AppBar au-dessus des fenêtres maximisées/borderless ;
  animation verticale des panneaux sans dépassement sous la barre des tâches.

Partiel / non exécuté volontairement :

- Les actions Power destructives (verrouillage, veille, redémarrage, arrêt,
  déconnexion) sont câblées et protégées par confirmation, mais n’ont pas été
  exécutées pendant les tests pour ne pas interrompre la session.
- Le toggle Bluetooth est câblé ; il n’a pas été actionné pour ne pas déconnecter
  les appareils de l’utilisateur.
- Notifications toast Windows non implémentées (le F9 est un Control Centre).
- Sélection d’un autre périphérique audio et liste/appairage Bluetooth encore
  dans les paramètres Windows.

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
- Dock : icônes récentes/favorites réelles dans la barre ; capture
  `final-bar-dock.png`.
- Barre au-dessus d’une fenêtre Notepad maximisée : logo toujours visible.
- Dashboard : 4 onglets capturés ; Media affiche la session Spotify réelle ;
  Performance affiche CPU/RAM/GPU/réseau/disque ; Weather affiche Paris.
- Météo sans ville : formulaire de configuration capturé, puis `Paris` restauré.
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
  Power ; dock de la barre ; aucune erreur QML.
- CPU idle mesuré : 0,38 s sur 5 s / 8 cœurs ≈ 0,9 % ; working set ≈ 451 Mo.

## 5. Captures

`windows/build/phase3-captures/` :

- `final-launcher-firefox.png`, `final-bar-dock.png`, `bar-dock.png`
- `final-dashboard-home.png`, `final-dashboard-media.png`,
  `final-dashboard-performance.png`, `final-dashboard-weather.png`
- `dashboard-weather-config.png`
- `final-panel-audio.png`, `final-panel-network.png`,
  `final-panel-battery.png`, `final-power.png`, `panel-bluetooth.png`
- `full-battery-fixed.png`, `bar-over-maximized.png`
- `final-sidebar.png`, `sidebar-favorite.png`

Logs : `C:\Users\axsou\AppData\Local\Temp\opencode\phase3-final-ui.err.log`,
`phase3-nonreg.err.log`, `phase3-rapid.err.log`.

## 6. Limites restantes

- Pas de centre de notifications Windows.
- Pas de sélection du périphérique audio (endpoint par défaut seulement).
- Pas de liste/appairage Bluetooth dans le shell.
- Les actions Power destructives n’ont pas été exécutées en test.
- Météo dépend d’Open-Meteo et d’une connexion réseau ; une seule ville.
- Certaines applications UWP n’exposent pas d’icône via le shell ; icône
  générique de secours.
- HDR, contraste élevé, tactile/stylet et masquage fullscreen non traités.
- Le warning QSG préexistant sur la géométrie des fenêtres AppBar subsiste.

## 7. Garanties

- `shell-main` n’a pas été modifié.
- Aucune fonctionnalité Step 1e supprimée : AppBar, focus, hover, transitions,
  D3D11, BlobRect, multi-écrans, DPI, F8/F9/F10, Échap.
- Commit final : voir `git log` sur `phase3-desktop-shell`.
