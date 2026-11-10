# Phase 3 â€” Desktop Shell : rapport dâ€™implÃ©mentation

Branche : `phase3-desktop-shell` (crÃ©Ã©e depuis `79eaffb`, sans rÃ©Ã©criture de lâ€™historique).
Projet Linux : `shell-main` lu uniquement, jamais modifiÃ©.

## 1. Architecture finale

```
windows/src/
  main.cpp                              services Qt + modes diagnostics
  platform/platform.*                   AppBar, overlay input, focus, Ã©crans
  platform/services/applauncherservice.* dÃ©couverte, recherche, favoris, lancement
  platform/services/systemservice.*      CPU, RAM, GPU, rÃ©seau, disque, historiques
  platform/services/mediaservice.*       GSMTC C++/WinRT, artwork, contrÃ´les
  platform/services/weatherservice.*     Open-Meteo, configuration ville
  platform/services/audioservice.*       Core Audio, volume, mute
  platform/services/networkservice.*     WLAN API / IPHelper
  platform/services/bluetoothservice.*   Windows.Devices.Radios + set-state
  platform/services/batteryservice.*     GetSystemPowerStatus + Ã©vÃ©nements
  platform/services/powerservice.*       verrouillage/veille/session
  qml/overlay/LauncherShell.qml          panneau launcher
  qml/overlay/DashboardShell.qml         onglets + slide horizontal
  qml/overlay/DashboardHome/Media/Performance/Weather.qml
  qml/overlay/SidebarShell.qml           Control Centre F9
  qml/popouts/AudioPanel/NetworkPanel/BluetoothPanel/BatteryPanel/PowerPanel.qml
  qml/components/DashCard/Sparkline/Meter/PanelHeader.qml
  src/shaders/blob.*                     panneau launcher ajoutÃ© au SDF
```

Le launcher est un panneau `launcher` du `PanelController`, rendu par le mÃªme
SDF/spring que les autres surfaces. Son ouverture est explicite (focus clavier),
sa fermeture passe par Ã‰chap, le clic extÃ©rieur ou le lancement rÃ©ussi.

## 2. APIs Windows utilisÃ©es

| Domaine | API |
|---|---|
| Applications | `IShellLinkW`/`IPersistFile`, `FOLDERID_Programs`, `FOLDERID_CommonPrograms`, `FOLDERID_AppsFolder`, `IShellItem2`/`PKEY_AppUserModel_ID`, `IShellItemImageFactory`, `ShellExecuteExW`, `SHGetFileInfoW` |
| Audio | `IMMDeviceEnumerator`, `IAudioEndpointVolume`, `IMMNotificationClient` |
| RÃ©seau | `WlanOpenHandle`/`WlanEnumInterfaces`/`WlanQueryInterface`, `NotifyIpInterfaceChange`, `GetIfTable2` |
| Bluetooth | `Windows.Devices.Radios` (C++/WinRT), `Radio.SetStateAsync` |
| Batterie | `GetSystemPowerStatus`, `WM_POWERBROADCAST` |
| Performance | `GetSystemTimes`, `GlobalMemoryStatusEx`, `GetIfTable2`, `GetDiskFreeSpaceExW`, DXGI `IDXGIAdapter3::QueryVideoMemoryInfo`, PDH `GPU Engine` |
| MÃ©dia | GSMTC `GlobalSystemMediaTransportControlsSessionManager`, `TryGetMediaPropertiesAsync`, `TryPlayAsync`/`TryPauseAsync`/`TrySkipNextAsync`/`TrySkipPreviousAsync` |
| MÃ©tÃ©o | Open-Meteo Geocoding + Forecast (HTTPS, sans clÃ© API) |
| Session | `LockWorkStation`, `SetSuspendState`, `ExitWindowsEx`, privilÃ¨ge `SeShutdownPrivilege` |

Aucun texte saisi nâ€™est exÃ©cutÃ© : le launcher ne lance que des entrÃ©es dÃ©couvertes
et lance `ShellExecuteExW` sur leur `.lnk` ou `shell:AppsFolder\<AUMID>`.

## 3. Ã‰tat des fonctionnalitÃ©s

TerminÃ© et testÃ© :

- Launcher par logo/F1, recherche fuzzy insensible Ã  la casse, navigation
  clavier, Ã‰chap, clic extÃ©rieur, favoris, icÃ´nes shell, lancement rÃ©el.
- 213 applications dÃ©couvertes : Firefox, Discord, Brave, Steam, Bloc-notes,
  Calculatrice, ParamÃ¨tres, etc.
- Dashboard 4 onglets avec donnÃ©es rÃ©elles ; Media GSMTC ; Performance
  CPU/RAM/GPU/rÃ©seau/disque avec historiques ; Weather Open-Meteo configurable.
- Panneaux Audio/RÃ©seau/Bluetooth/Batterie rÃ©els ; Control Centre F9 ; Power
  avec confirmation.
- Animations de panneaux, launcher, onglets et icÃ´nes ; SDF/spring conservÃ©s.

Partiel / non exÃ©cutÃ© volontairement :

- Les actions Power destructives (verrouillage, veille, redÃ©marrage, arrÃªt,
  dÃ©connexion) sont cÃ¢blÃ©es et protÃ©gÃ©es par confirmation, mais nâ€™ont pas Ã©tÃ©
  exÃ©cutÃ©es pendant les tests pour ne pas interrompre la session.
- Le toggle Bluetooth est cÃ¢blÃ© ; il nâ€™a pas Ã©tÃ© actionnÃ© pour ne pas dÃ©connecter
  les appareils de lâ€™utilisateur.
- Notifications toast Windows non implÃ©mentÃ©es (le F9 est un Control Centre).
- SÃ©lection dâ€™un autre pÃ©riphÃ©rique audio et liste/appairage Bluetooth encore
  dans les paramÃ¨tres Windows.

## 4. Tests rÃ©ellement exÃ©cutÃ©s

### Debug

- `--list-apps` : `TOTAL 213`, Firefox/Discord/Bloc-notes/Calculatrice prÃ©sents.
- `--launch-app "Calculatrice"` : `CalculatorApp` dÃ©marrÃ©.
- UI : clic logo â†’ launcher ; saisie `calc` ; EntrÃ©e â†’ `CalculatorApp` PID
  observÃ© ; Ã‰chap ferme ; styles `transparent=yes noactivate=yes` aprÃ¨s fermeture.
- Favori : clic Ã©toile â†’ `launcher/favorites` contient lâ€™ID de Calculatrice ;
  F9 affiche le favori ; Ã©tat restaurÃ© aprÃ¨s test.
- Dashboard : 4 onglets capturÃ©s ; Media affiche la session Spotify rÃ©elle ;
  Performance affiche CPU/RAM/GPU/rÃ©seau/disque ; Weather affiche Paris.
- MÃ©tÃ©o sans ville : formulaire de configuration capturÃ©, puis `Paris` restaurÃ©.
- Media : session Spotify (`Kalash`, Booba) et Media Player
  (`caelestia-test-tone`) dÃ©tectÃ©es ; pause/reprise vÃ©rifiÃ©es ; artwork en cache.
- Statut : audio `Haut-parleurs (SkillKorp SKP_H21)`, volume 100, mute testÃ© et
  restaurÃ© ; Wi-Fi `Livebox-6190` signal 81/66 ; Bluetooth prÃ©sent/activÃ© ;
  batterie 100 % sur secteur (`minutes=-1`, Windows ne fournit pas dâ€™autonomie).
- Dashboard 100 % â†” 125 % : ouvertures directes, sans `PANEL_CLOSE`
  intermÃ©diaire ; sortie ferme.
- Transitions directes audio â†’ rÃ©seau â†’ Bluetooth : aucune fermeture
  intermÃ©diaire. Balayage rapide : seule la cible finale (Battery) sâ€™ouvre.
- Focus : Discord reste au premier plan pendant lâ€™ouverture hover ; styles
  hover `transparent=no noactivate=yes`.
- Panneaux Audio/RÃ©seau/Bluetooth/Batterie/Power ouverts et capturÃ©s, sans
  erreur QML.
- Step 1e conservÃ© : AppBar, clic Ã  travers, `WS_EX_NOACTIVATE`,
  `WS_EX_TRANSPARENT`, D3D11, F8/F9/F10, multi-Ã©crans/DPI.

### Release

- Build `windows/build-release` en `Release` : succÃ¨s.
- `--dump-status`, `--dump-system`, `--weather Paris`, `--list-apps`,
  `--dump-media`, `--launch-app` : mÃªmes services, mÃªmes valeurs rÃ©elles.
- UI Release : launcher Firefox capturÃ© ; `calc` + EntrÃ©e a lancÃ©
  `CalculatorApp` ; Dashboard 4 onglets ; Audio/RÃ©seau/Batterie ; F9 ;
  Power ; aucune erreur QML.
- CPU idle mesurÃ© : 0,38 s sur 5 s / 8 cÅ“urs â‰ˆ 0,9 % ; working set â‰ˆ 451 Mo.

## 5. Captures

`windows/build/phase3-captures/` :

- `final-launcher-firefox.png`
- `final-dashboard-home.png`, `final-dashboard-media.png`,
  `final-dashboard-performance.png`, `final-dashboard-weather.png`
- `dashboard-weather-config.png`
- `final-panel-audio.png`, `final-panel-network.png`,
  `final-panel-battery.png`, `final-power.png`
- `final-sidebar.png`, `sidebar-favorite.png`
- `panel-bluetooth.png`

Logs : `C:\Users\axsou\AppData\Local\Temp\opencode\phase3-final-ui.err.log`,
`phase3-nonreg.err.log`, `phase3-rapid.err.log`.

## 6. Limites restantes

- Pas de centre de notifications Windows.
- Pas de sÃ©lection du pÃ©riphÃ©rique audio (endpoint par dÃ©faut seulement).
- Pas de liste/appairage Bluetooth dans le shell.
- Les actions Power destructives nâ€™ont pas Ã©tÃ© exÃ©cutÃ©es en test.
- MÃ©tÃ©o dÃ©pend dâ€™Open-Meteo et dâ€™une connexion rÃ©seau ; une seule ville.
- Certaines applications UWP nâ€™exposent pas dâ€™icÃ´ne via le shell ; icÃ´ne
  gÃ©nÃ©rique de secours.
- HDR, contraste Ã©levÃ©, tactile/stylet et masquage fullscreen non traitÃ©s.
- Le warning QSG prÃ©existant sur la gÃ©omÃ©trie des fenÃªtres AppBar subsiste.

## 7. Garanties

- `shell-main` nâ€™a pas Ã©tÃ© modifiÃ©.
- Aucune fonctionnalitÃ© Step 1e supprimÃ©e : AppBar, focus, hover, transitions,
  D3D11, BlobRect, multi-Ã©crans, DPI, F8/F9/F10, Ã‰chap.
- Commit final : voir `git log` sur `phase3-desktop-shell`.

