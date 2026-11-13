# Phase 3 — Gestion de fenêtres inspirée de Hyprland

Branche : `phase3-desktop-shell`  
Windows de test : Windows 11 26H2, build 26300.9711  
Écrans : ZOWIE XL LCD 1920×1080 à 100 % (principal) et DISPLAY1
1536×864 à 125 % (secondaire, à gauche).  
Références Linux : `shell-main` (lecture seule), `daemons/Shortcuts.qml`,
`services/Hypr.qml`, les composants Workspaces et la configuration Caelestia
`hypr/hyprland/keybinds.lua` / `hypr/variables.lua` liée depuis le README de
`shell-main`. Aucun fichier Linux n’a été modifié.

## 1. Fichiers créés et modifiés

Créés :

- `windows/src/platform/services/tilingengine.hpp`
- `windows/src/platform/services/tilingengine.cpp`
- `windows/src/platform/services/windowmanagerservice.hpp`
- `windows/src/platform/services/windowmanagerservice.cpp`
- `windows/docs/hyprland-window-management-report.md`

Modifiés :

- `windows/src/main.cpp` — singleton `WindowManager`, connexions vers
  `VirtualDesktopService`, arrêt propre sur `aboutToQuit`.
- `windows/src/qml/Main.qml` — connexions QML `launcherRequested`,
  `sidebarRequested`, `sessionRequested`.
- `windows/CMakeLists.txt` — sources du moteur/service, `winmm`.
- `windows/README.md` — section Tiling et raccourcis de secours.
- `windows/docs/phase3-report.md` — lien vers ce rapport.

## 2. Architecture finale

```
Win32 worker thread (un seul thread, boucle de messages)
  ├─ fenêtre cachée CaelestiaWindowManager
  │   ├─ RegisterHotKey (raccourcis globaux)
  │   ├─ SetWinEventHook (création/destruction/show/minimize/move/focus/cloak)
  │   ├─ timers : animation 7 ms, rescan de secours 1 s, debounce
  │   └─ WM_HOTKEY / commandes venant de Qt
  ├─ hook souris (WH_MOUSE_LL) : Win + clic gauche = déplacement
  ├─ IVirtualDesktopManager (documenté) : bureau courant des fenêtres
  └─ TilingEngine par écran + par bureau
        │
        ▼  QAbstractListModel / propriétés Qt (thread GUI)
WindowManagerService (façade QML) ── signaux vers VirtualDesktopService
        │
        ├─ BarWorkspaces / Workspaces existants (inchangés)
        └─ barre active, overlays, AppBar (inchangés)
```

- Le moteur `TilingEngine` est **pur** : il ne connaît que des HWND et des
  rectangles. Il est testé sans déplacer de fenêtre (`--test-tiling`).
- Le service Win32 fait le suivi réel, les appels `SetWindowPos`, les hooks et
  les hotkeys. La bordure de fenêtre active et les boutons de légende ont été
  retirés depuis (voir `windows/README.md`, « Déplacer, fermer, encadrer ») :
  déplacer se fait avec `Win` + bouton gauche, fermer avec `Win+C`.
- Un thread dédié possède le message loop. Le thread QML ne fait que publier
  l’état et envoyer des commandes.
- **Aucun deuxième système de bureaux virtuels** : les changements, créations,
  fermetures et déplacements passent par `VirtualDesktopService` existant. Le
  gestionnaire de fenêtres reçoit seulement le bureau courant et son nombre.
- **Layouts par bureau et par écran** : chaque `MonitorState` possède un
  `TilingEngine` avec son arbre Dwindle, son ratio Master et son ordre. Les
  layouts sont sérialisés en mémoire quand on quitte un bureau et restaurés
  au retour. Les fenêtres d’un bureau inactif ne sont jamais déplacées.
- **Sécurité** :
  - fenêtres du shell, fenêtres sans cadre, dialogues `#32770`, fenêtres
    possédées, tool windows, topmost et non redimensionnables ne sont pas
    tilées ;
  - `showCmd`/styles/placement d’origine sont sauvegardés avant la première
    prise de contrôle ;
  - `Ctrl+Alt+Shift+Escape` désactive le tiling et restaure immédiatement les
    géométries ;
  - `aboutToQuit` appelle `shutdown()` et restaure avant la fermeture ;
  - aucun hook clavier bas niveau, aucune touche Windows vitale n’est
    interceptée.

## 3. Layouts réellement implémentés

Le layout par défaut est **Dwindle**, comme `general.lua` du dépôt Caelestia
(`layout = "dwindle"`).

| Layout | Implémentation |
|---|---|
| Dwindle | Arbre binaire ; chaque nouvelle fenêtre divise la feuille focalisée ; axe alterné par profondeur ; ratio 0.5 ; suppression avec remontée du frère ; ratios ajustables. |
| Master | Première fenêtre à gauche (ratio 0.6 par défaut), autres empilées à droite ; `promote-master` déplace la fenêtre active en maître ; largeur du maître ajustable. |
| Split horizontal | Fenêtres côte à côte, largeurs égales, gaps internes. |
| Split vertical | Fenêtres empilées, hauteurs égales, gaps internes. |

Gaps : 10 px extérieurs (20 px pour une fenêtre seule), 5 px intérieurs.
La zone de travail vient de `MONITORINFO.rcWork` (donc après l’AppBar
Caelestia). Une fenêtre unique occupe la zone avec 20 px de marge.

Les quatre layouts ont été vérifiés géométriquement : aucune intersection,
toutes les fenêtres dans la zone, après ajout/suppression de fenêtres.

## 4. Raccourcis effectivement implémentés

La configuration Caelestia réelle (dotfiles liés par `shell-main`) utilise
`SUPER` et des combinaisons dont beaucoup sont réservées par Windows. Le
service tente d’abord la combinaison d’origine, puis une alternative
`Ctrl+Alt…`. Toutes les tentatives sont enregistrées et publiées dans la
propriété `bindings` du service.

### Résultat des tests `RegisterHotKey`

- `Win+Alt+Space` (floating Caelestia) : **disponible et enregistré**.
- `Win+Minus`, `Win+Shift+Minus`, `Win+Shift+Equal` : enregistrés.
- Tous les autres `Win+lettre`, `Win+chiffre`, `Win+flèches`,
  `Win+Shift+flèches` testés : **indisponibles** (erreur 1409, réservés par
  Explorer/Windows).
- Tous les `Ctrl+Alt…` testés : disponibles.
- Aucun raccourci vital n’est capturé : `Win+L`, `Win+Tab`,
  `Win+Ctrl+←/→`, `Win+Ctrl+Shift+B`, `Alt+Tab`, `Ctrl+Alt+Delete`,
  `Win+D`, `Win+Shift+S`, etc. restent natifs.

### Table des raccourcis actifs

| Action | Raccourci effectif | Remarque |
|---|---|---|
| Focus gauche/droite/haut/bas | `Ctrl+Alt+←/→/↑/↓` et `Ctrl+Alt+H/L/K/J` | `Win+flèches` réservé (Snap) |
| Déplacer la fenêtre dans le layout | `Ctrl+Alt+Shift+←/→/↑/↓` et `Ctrl+Alt+Shift+H/L/K/J` | swap avec le voisin directionnel |
| Focus suivant/précédent | `Ctrl+Alt+PageDown/PageUp` | boucle sur l’ordre du layout |
| Floating on/off | `Win+Alt+Space` (original) et `Ctrl+Alt+V` | testé |
| Plein écran | `Ctrl+Alt+F` | `Win+F` réservé |
| Fermer la fenêtre active | `Ctrl+Alt+Q` | `WM_CLOSE` réel |
| Layout suivant/précédent | `Ctrl+Alt+P` / `Ctrl+Alt+Shift+P` | Dwindle → Master → SplitH → SplitV |
| Passer la fenêtre active en maître | `Ctrl+Alt+Enter` | Master |
| Activer/désactiver le tiling | `Ctrl+Alt+Shift+T` | état par défaut : activé |
| Verrouillage strict on/off | `Ctrl+Alt+Shift+S` | par défaut : on |
| Coupe-circuit / restauration | `Ctrl+Alt+Shift+Escape` | immédiat, sans animation |
| Bureau 1..10 | `Ctrl+Alt+1..9/0` | `Win+chiffres` réservé |
| Déplacer la fenêtre vers un bureau | `Ctrl+Alt+Shift+1..9/0` | `Win+Alt+chiffres` réservé |
| Bureau précédent/suivant | `Ctrl+Alt+Shift+PageUp/PageDown` | en plus de `Win+Ctrl+←/→` natif |
| Réduire/agrandir la largeur | `Ctrl+Alt+Minus/Equal` (et `Win+Minus`) | ajuste le split Dwindle/Master |
| Réduire/agrandir la hauteur | `Ctrl+Alt+Shift+Minus/Equal` | ajuste le split Dwindle |
| Terminal | `Ctrl+Alt+T` | Windows Terminal, fallback `cmd.exe` |
| Gestionnaire de fichiers | `Ctrl+Alt+E` | Explorer |
| Navigateur | `Ctrl+Alt+W` | navigateur par défaut |
| Éditeur | `Ctrl+Alt+C` | VS Code, fallback Bloc-notes |
| Launcher | `Ctrl+Alt+Space` | ouvre le panneau launcher Caelestia |
| Sidebar | `Ctrl+Alt+N` | sidebar Caelestia |
| Menu de session / power | `Ctrl+Alt+Shift+M` | `Ctrl+Alt+Delete` est impossible |
| Repos/overlays | `Esc` existant | non modifié |

### Configuration des alternatives

Le service lit des surcharges dans
`%APPDATA%\Caelestia\window-manager.ini` (QSettings, format INI) :

```ini
[bindings]
focus-left=Ctrl+Alt+H
toggle-floating=Alt+Space
workspace-1=Alt+1
layout=dwindle
strict=true
animations=true
```

Une surcharge est essayée avant les valeurs par défaut, sans supprimer le
fallback de sécurité. Les noms d’action sont ceux publiés par
`WindowManager.bindings`.

## 5. Différences avec Hyprland

- **Bureaux virtuels** : Windows gère une session unique pour tous les
  écrans ; Hyprland a des workspaces par moniteur. Le projet ne simule pas
  des bureaux par moniteur.
- **Workspaces spéciaux et groupes de fenêtres** : absents de Windows ; non
  simulés.
- **Dwindle** : alternance par profondeur et ratio 0.5. Les options
  `smart_split` / `preserve_split` / `smart_resizing` de Hyprland ne sont pas
  reproduites à l’identique.
- **Master** : ratio 0.6, maître à gauche, pile à droite ; pas de variantes
  d’orientation ni de `layoutmsg mfact` au-delà du redimensionnement.
- **Animations** : interpolation ease-out cubique sur ~180 ms, `DwmFlush`
  pour suivre le refresh (60/144 Hz). Ce ne sont pas les courbes bezier
  exactes de Hyprland.
- **Tiling strict** : les déplacements/redimensionnements libres sont
  réappliqués par le gestionnaire ; Hyprland autorise le drag avec un bind
  souris. Le mode strict est activé par défaut, comme demandé.
- **Floating** : bascule individuelle ; les fenêtres topmost, tool windows,
  dialogues et fenêtres sans cadre sont toujours laissées flottantes.
- **Plein écran** : fenêtre sans bordure sur le moniteur entier ; la barre
  Caelestia reste topmost et peut rester visible au-dessus.
- **Persistance des layouts** : en mémoire pendant la session du shell ; pas
  de fichier de layouts au redémarrage. Les fenêtres et les bureaux Windows,
  eux, persistent.
- **Raccourcis** : les combinaisons `SUPER` de Caelestia sont remplacées par
  des alternatives sans hook clavier bas niveau, afin de ne pas bloquer les
  raccourcis système.

## 6. Limitations techniques Windows

- **RegisterHotKey** : de nombreuses combinaisons `Win+…` sont réservées
  (1409). Les alternatives sont explicites et configurables ; le shell ne
  vole pas les raccourcis vitaux.
- **Focus** : `SetForegroundWindow` est soumis à la politique de premier plan
  de Windows. Le service utilise `AttachThreadInput`, puis
  `SwitchToThisWindow`, puis une pression Alt synthétique en dernier recours.
  Testé : le focus directionnel a fonctionné sur Notepad/Explorer/Terminal.
- **Applications élevées / UIPI** : le tiling ne peut pas toujours déplacer ou
  redimensionner une fenêtre élevée. Le service ne force pas ; il peut la
  laisser hors layout. Le déplacement entre bureaux dispose du fallback privé
  déjà présent dans `VirtualDesktopService`.
- **Tailles minimales** : certaines applications refusent une géométrie plus
  petite que leur minimum ; le rescan peut alors réappliquer la géométrie et
  créer un conflit visuel.
- **Applications qui se redimensionnent elles-mêmes** : le mode strict peut
  les corriger à chaque rescan de 1 s.
- **Fenêtres UWP/dialogues** : les fenêtres sans `WS_THICKFRAME` et les
  dialogues sont exclues ; une application UWP peut donc rester flottante.
- **Bordure active** : fenêtre layered topmost click-through ; elle est
  au-dessus des applications et peut passer au-dessus d’un panneau Caelestia
  dans les cas extrêmes. Elle ne vole jamais le focus.
- **Fermeture du shell** : la restauration est faite par `aboutToQuit`. Un
  `TerminateProcess`/kill forcé ne peut pas restaurer ; les fenêtres restent
  alors dans leur état tilé. Utiliser `Ctrl+Alt+Shift+Escape` avant un arrêt
  brutal.
- **Moniteurs** : le rebranchement physique n’a pas été testé dans cette
  phase ; le rescan/`WM_DISPLAYCHANGE` et les zones de travail par moniteur
  sont en place.
- **Débogage** : `CAELESTIA_DEBUG_WM=1` journalise les actions, focus,
  bindings et déplacements.

## 7. Tests réellement effectués

Machine réelle, fenêtres réelles. Les noms de classes/HWND et les géométries
ci-dessous viennent de `GetWindowRect`, de `EnumWindows` et des journaux du
service.

| # | Scénario | Résultat |
|---|---|---|
| 1 | Une fenêtre | Notepad/Explorer/Console : une fenêtre prend la zone de travail avec 20 px de marge. |
| 2 | Deux fenêtres | Dwindle : deux colonnes (gauche/droite). |
| 3 | Cinq/six fenêtres | Six fenêtres réelles (2 cmd/Terminal, 2 Explorer, Notepad, VS Code) tilées sans chevauchement ; Master avec maître + cinq piles. |
| 4 | Fermer au milieu | `Ctrl+Alt+Q` sur un cmd : fenêtre fermée, cinq fenêtres restantes réorganisées, focus passé à la suivante (`WM focusWindow`). |
| 5 | Focus clavier | Droite : cmd → Explorer ; bas : Explorer → Notepad ; `Ctrl+Alt+PageDown/PageUp` cycles. |
| 6 | Déplacer vers un bureau | `Ctrl+Alt+Shift+3` déplace la fenêtre active vers le bureau 3 ; elle disparaît du bureau 2. |
| 7 | Changer de bureau | Passage test → bureau 3 → test ; layouts distincts restaurés. |
| 8 | Revenir au bureau | Retour test : fenêtres et positions du layout restaurées ; bureau 3 conserve sa fenêtre plein cadre. |
| 9 | Changer de layout | Dwindle → Master (`1101x1060` + piles `734x527`) → SplitH (`610x1060` ×3) → SplitV (`1840x350` ×3) → Dwindle. |
| 10 | Floating | `Win+Alt+Space` : Notepad reste `993,543 917x527`, les deux autres se réorganisent ; retour au tiling. |
| 11 | Tiling strict | Strict off : déplacement libre à `300,200 500x400` conservé. Strict on : re-tilé à `993,543 917x527`. |
| 12 | Maximiser/restaurer | `SW_MAXIMIZE` : `IsZoomed` revient à `False`, la fenêtre est re-tilée ; layout conservé. Minimiser/restaurer : retirée puis réinsérée et re-tilée. |
| 13 | Écran secondaire | VS Code tilé à `-1844,16 1444x832` sur DISPLAY1 (125 %), pendant que cinq fenêtres restent tilées sur le principal. |
| 14 | DPI | Principal 100 %, secondaire 125 % ; coordonnées physiques correctes sur les deux. |
| 15 | Fermer Caelestia | `Ctrl+Q` via l’overlay : le processus s’arrête, Windows reste utilisable ; restauration des fenêtres (voir limite ci-dessous). |
| 16 | Focus avec shell en arrière-plan | Hotkeys `Ctrl+Alt…` reçues alors que le shell n’a pas le focus ; les fenêtres changent réellement. |
| 17 | Apps | Notepad, Explorateur, Windows Terminal (`CASCADIA_HOSTING_WINDOW_CLASS`), VS Code, deux fenêtres Firefox (bureau d’origine), plusieurs fenêtres d’une même application. |
| 18 | Raccourcis Caelestia | Launcher `Ctrl+Alt+Space`, sidebar `Ctrl+Alt+N`, power `Ctrl+Alt+Shift+M` : journaux `PANEL_OPEN launcher/sidebar/power` ; Esc ferme. |
| 19 | Coupe-circuit | `Ctrl+Alt+Shift+Escape` : restauration immédiate des six fenêtres du bureau de test ; comparaison exacte des rects avant/après (`RESTORE_OK=True`). |
| 20 | Redémarrage du shell | Arrêt/relance : nouveaux layouts reconstruits ; les fenêtres restent sur leurs bureaux Windows. |

### Preuves géométriques (extraits)

Dwindle, 3 fenêtres, bureau de test (zone 70..1840 × 10..1070) :

```
cmd       : 70,10   918x1060
Explorer  : 993,10  917x528
Notepad   : 993,543 917x527
```

Master : `cmd 1101x1060` à gauche, deux piles `734x527` à droite.  
Split horizontal : trois colonnes `610x1060`.  
Split vertical : trois lignes `1840x350`.  
Secondaire 125 % : VS Code `-1844,16 1444x832`.  
Strict off : Notepad `300,200 500x400` conservé.  
Strict on : Notepad `993,543 917x527`.  
Fermeture : un cmd fermé, cinq fenêtres restantes, focus sur le cmd suivant.

### Non-régression

- L’AppBar reste enregistrée sur les deux écrans (`Caelestia AppBar screen`).
- Les Workspaces, le Dashboard, la Sidebar, le Launcher, Media, Weather,
  Performance, D3D11, les flags `WS_EX_NOACTIVATE`/`WS_EX_TRANSPARENT`, la
  barre des tâches et le multi-écrans n’ont pas été modifiés.
- Le shell n’est jamais tilé : ses fenêtres (barres, overlays) sont
  exclues par PID et par classe.
- Aucune erreur QML (`qrc:`, `TypeError`, `ReferenceError`) dans les journaux
  des tests finaux.

## 8. Tests non effectués

- Débranchement/rebranchement **physique** d’un écran pendant un layout
  actif. Le code écoute `WM_DISPLAYCHANGE` et recalcule les moniteurs, mais le
  scénario matériel n’a pas été reproduit.
- Application élevée (UAC) tilée : le comportement UIPI n’a pas été validé.
- Applications UWP avec des cadres non standard (Calculator, Settings) :
  non testées.
- Profilage frame par frame à 144 Hz et test de charge avec beaucoup de
  fenêtres (10+).
- Test de longue durée / fuites mémoire.
- Surcharge effective du fichier `window-manager.ini` : le parseur et les
  valeurs par défaut sont testés, pas une configuration personnalisée.
- Réorganisation manuelle des bureaux dans Task View pendant un layout
  actif (le modèle VirtualDesktopService gère déjà les réordres par reset).

## 9. Builds et état final

- **Release** : `windows/build-release` — compilation et lien réussis.
- **Debug** : `windows/build` — compilation et lien réussis.
- Test unitaire du moteur : `caelestia-shell.exe --test-tiling` → `TILING PASS`
  pour Dwindle, Master, SplitH, SplitV (5 fenêtres, pas de chevauchement,
  focus/déplacement/suppression).
- État final observé : 1 bureau Windows d’origine, 5 fenêtres connues,
  4 fenêtres visibles tilées (Discord + 2 Firefox sur l’écran principal,
  OpenCode sur le secondaire), 1 Explorateur minimisé. Le shell tourne avec
  `enabled=true`, `strict=true`, layout Dwindle.

## 10. Problèmes restants / limites connues

1. La restauration après fermeture normale est fonctionnelle mais pas
   pixel-perfect : le retrait de l’AppBar et le retour de la barre des tâches
   décalent certaines fenêtres de 60 px horizontalement ; une fenêtre Firefox
   a conservé sa largeur tilée lors du test de sortie. À corriger en
   restaurant après la désinscription complète de l’AppBar si une précision
   absolue est exigée.
2. Le tiling strict ne gère pas les tailles minimales imposées par certaines
   applications ; le rescan peut réappliquer la géométrie et provoquer un
   conflit visuel.
3. Le plein écran borderless laisse la barre Caelestia topmost au-dessus dans
   certains cas ; il faudrait la masquer explicitement pendant le fullscreen.
4. Les layouts par bureau sont conservés en mémoire du shell, pas sur disque.
5. Les raccourcis `Win+…` originaux de Caelestia sont remplacés par des
   alternatives `Ctrl+Alt…` ; c’est une contrainte Windows, documentée et
   configurable.
6. Aucun indicateur de layout dans la barre (le layout se change au clavier) ;
   l’ajout d’un petit indicateur est une amélioration future, pas un mock.

## 11. Commit

Commit de la phase : `03e832f` sur la branche `phase3-desktop-shell`.
Ce rapport est inclus dans le commit de documentation.
