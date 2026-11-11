# Phase 3 — Workspaces : vrais bureaux virtuels Windows

Branche : `phase3-desktop-shell`  
Projet Linux de référence : `shell-main` (lecture seule)  
Machine de test : Windows 11 26H2, build 26300.9711  
Écrans : ZOWIE XL LCD 1920×1080 à 100 % (principal) et DISPLAY1
1536×864 à 125 % (secondaire, à gauche).

## 1. Cause exacte du système fictif

La barre gauche affichait un composant `BarWorkspaces.qml` purement décoratif,
sans aucun lien avec Windows. Ses données étaient codées en dur :

```qml
property int activeWorkspace: 1
property var occupiedWorkspaces: [1, 3]
property var windowIcons: ({ 1: ["terminal"], 3: ["folder"] })
```

Aucun service C++, aucun appel COM, aucune énumération de fenêtres et aucun
changement de bureau réel n'existaient derrière ces propriétés. Les lignes
affichées ne correspondaient donc ni aux bureaux Windows, ni à leurs fenêtres.
L'onglet « Workspaces » du Dashboard utilisait déjà `WindowService` et
`EnumWindows`, mais uniquement pour les fenêtres du bureau courant ; il ne
gérait pas la liste des bureaux et n'était pas relié à la barre.

## 2. Architecture choisie

```
Explorer / ImmersiveShell
  ├─ IVirtualDesktopManagerInternal (privé)  énumération, changement,
  │                                          création, fermeture
  ├─ IVirtualDesktop (privé)                 identifiant stable GUID
  ├─ IVirtualDesktopManager (documenté)      fenêtre → bureau, déplacement
  └─ IApplicationViewCollection (privé)      fallback de déplacement
                    │
                    ▼   thread STA dédié, file de commandes, rafraîchissement 1 s
        VirtualDesktopService (QAbstractListModel, singleton QML)
                    │   rôles : id, name, desktopIndex, active, occupied, windows
                    ▼
        BarWorkspaces.qml (ListView, indicateur, icônes, +/−, drag)
```

- **Un seul thread COM** : `VirtualDesktopService` crée un thread
  `CoInitializeEx(COINIT_APARTMENTTHREADED)`. Tous les appels COM, `EnumWindows`
  et les lectures de processus y sont exécutés. Le thread graphique ne fait
  que recevoir le modèle déjà construit.
- **File de commandes** : `Refresh`, `Switch`, `Create`, `Close`, `Move`. La
  boucle attend une commande ou 1 s, traite les commandes, puis relit l'état
  réel et publie le modèle.
- **Source de vérité unique** : Windows/Explorer. Le service ne conserve aucun
  état persisté et ne simule rien ; au redémarrage il relit les bureaux réels.
- **Modèle stable** : `QAbstractListModel` avec insertion/suppression de lignes
  et `dataChanged` par rôle. Une réorganisation manuelle dans Task View
  provoque un `beginResetModel` ; les changements de titre de fenêtre ne
  recréent pas les délégués.
- **Synchronisation** : il n'existe pas d'API documentée de notification de
  changement de bureau. Le service interroge donc Explorer toutes les secondes
  (et immédiatement après chaque commande). Les interfaces privées de
  notification n'ont pas été ajoutées pour limiter la surface non documentée.
  Aucune boucle : le rafraîchissement est en lecture seule, seules les actions
  QML émettent des commandes.
- **Multi-écrans** : les bureaux virtuels Windows sont gérés à l'échelle de la
  session. Chaque écran possède sa barre, mais toutes les barres sont liées au
  même singleton : elles affichent la même liste et le même bureau actif.
  Aucun modèle par moniteur n'a été inventé.
- **Fallback** : si `IVirtualDesktopManagerInternal` ou
  `IVirtualDesktopManager` est indisponible, `available` passe à `false` et le
  composant QML disparaît entièrement (aucune ligne fictive). Les erreurs de
  commande sont exposées par la propriété `error` du service.

## 3. Interfaces Windows utilisées

| Usage | Interface | Statut | Identifiants |
|---|---|---|---|
| Énumérer, changer, créer, fermer | `IVirtualDesktopManagerInternal` | **privé**, isolé dans un thread STA | service `{C5E0CDCA-7B6E-41B2-9FC4-D93975CC467B}`, IID `{53F5CA0B-158F-4124-900C-057158060B27}` |
| Identifiant d'un bureau | `IVirtualDesktop` | **privé** | IID `{3F07F4BE-B107-441A-AF0F-39D82529072C}` |
| Fenêtre → bureau, déplacement documenté | `IVirtualDesktopManager` | **documenté** | CLSID `{AA509086-5CA9-4C25-8F95-589D3C07B48A}`, IID `{A5CD92FF-29BE-454C-8D04-D82879FB3F1B}` |
| Fallback de déplacement | `IApplicationViewCollection` + `MoveViewToDesktop` | **privé** | IID `{1841C6D7-4F9D-42C0-AF41-8747538F10E5}`, vue `IApplicationView` `{372E1D3B-38D3-42E4-A15B-8AB2B178F513}` |
| Énumération des fenêtres | `EnumWindows`, `GetWindowTextW`, `GetWindowLongPtrW`, `QueryFullProcessImageNameW` | documenté | — |

`ImmersiveShell` est obtenu via `CLSID_ImmersiveShell`
`{C2F03A33-21F5-47FA-B4BB-156362A2F239}`, puis `QueryService` expose
`IVirtualDesktopManagerInternal` et `IApplicationViewCollection`. L'ordre des
méthodes de `IVirtualDesktopManagerInternal` a été validé sur Windows 11
24H2/26H2 (build 26300) : `GetCount`, `MoveViewToDesktop`,
`CanViewMoveDesktops`, `GetCurrentDesktop`, `GetDesktops`,
`GetAdjacentDesktop`, `SwitchDesktop`, `SwitchDesktopAndMoveForegroundView`,
`CreateDesktop`, `MoveDesktop`, `RemoveDesktop`, `FindDesktop`, etc.

`CreateDesktop` ne change pas le bureau actif : le service attend 250 ms puis
appelle `SwitchDesktop`. `RemoveDesktop(target, fallback)` laisse Windows
déplacer les fenêtres du bureau fermé vers le bureau de repli : aucune
application n'est fermée par le shell.

### Détection des fenêtres

À chaque rafraîchissement, `EnumWindows` parcourt les fenêtres et exclut :

- les fenêtres invisibles (`IsWindowVisible`) ;
- les fenêtres sans titre ;
- les fenêtres `WS_EX_TOOLWINDOW` ;
- les classes `Progman`, `WorkerW`, `Shell_TrayWnd`,
  `Shell_SecondaryTrayWnd`, `Windows.UI.Core.CoreWindow` ;
- les fenêtres du propre processus du shell (PID), car
  `IVirtualDesktopManager` renvoie `E_INVALIDARG` pour elles.

Les fenêtres des bureaux **inactifs** sont volontairement conservées : elles
sont cloaked par DWM, mais leur identifiant de bureau est justement
l'information recherchée. Le test de cloak (`DWMWA_CLOAKED`) a été retiré pour
cette raison. Les fenêtres minimisées restent visibles et donc listées ; les
fenêtres réellement cachées ne le sont pas. Chaque entrée contient l'ID HWND,
le titre réel et le nom du processus.

## 4. Fonctionnalités réellement opérationnelles

| Fonction | Résultat |
|---|---|
| Affichage des bureaux | Liste réelle issue d'Explorer, avec GUID stable, numéro (« Bureau N »), actif/inactif et présence de fenêtres. |
| Changement par clic | Clic sur une ligne → `VirtualDesktops.switchTo(id)` → `SwitchDesktop` réel. L'indicateur se déplace avec l'animation. |
| Changement par raccourci | `Win+Ctrl+←/→` testés : la barre suit le bureau Windows en moins d'une seconde. |
| Changement par Task View | Task View ouverte (`Win+Tab`), vignette « Bureau 2 » cliquée : le bureau réel a changé et la barre s'est synchronisée. |
| Création | Bouton `+` → `CreateDesktop` réel + activation du nouveau bureau, liste et indicateur actualisés. |
| Fermeture | Clic du milieu sur une ligne ou bouton `−` (bureau actif) → `RemoveDesktop` avec repli. Le dernier bureau ne peut pas être fermé. |
| Déplacement de fenêtre | Drag interne d'une icône de fenêtre vers une autre ligne → `MoveWindowToDesktop` (documenté) ; si Windows répond `E_ACCESSDENIED`, fallback privé `GetViewForHwnd` + `MoveViewToDesktop`. La vraie fenêtre change de bureau. |
| Focus d'une fenêtre | Clic sur une icône → `AppWindows.focusWindow(hwnd)` ; la fenêtre réelle est activée et Windows suit son bureau si nécessaire. |
| Synchronisation Windows → Caelestia | Changements externes par l'API Explorer, par Task View et par les raccourcis : modèle et indicateur à jour en ~1 s. |
| Redémarrage | Les bureaux et leur état actif survivent au redémarrage du shell ; le service les relit depuis Windows. |
| Absence de doublons | Les GUID sont uniques, le modèle est diffé par identifiant, et l'UI disparaît si le service n'est pas disponible. |

### Détail du déplacement de fenêtre

`IVirtualDesktopManager::MoveWindowToDesktop` a renvoyé
`E_ACCESSDENIED` (`0x80070005`, HRESULT `-2147024891`) pour une fenêtre
OpenCode. Le service tente alors la vue Explorer de la fenêtre
(`IApplicationViewCollection::GetViewForHwnd`) puis
`IVirtualDesktopManagerInternal::MoveViewToDesktop`. Ce chemin a fonctionné :

```
VDESK move 985320 to AED9FF68-8FAE-4016-B335-BAFF03886C3B fallback true result 0
VDESK desktop 0 active true windows 8
VDESK desktop 1 active false windows 1
```

Le déplacement inverse a ramené la fenêtre et le modèle à 9/0. Le fallback
n'est utilisé qu'après l'échec du chemin documenté ; si les deux échouent, la
commande est signalée en erreur et le modèle reste inchangé (aucun faux
déplacement).

## 5. Limitations des API Windows

- **Interfaces privées** : `IVirtualDesktopManagerInternal`,
  `IVirtualDesktop` et `IApplicationViewCollection` ne sont pas documentées et
  peuvent changer avec une build de Windows. Elles sont confinées dans un seul
  fichier et un seul thread STA, avec détection d'échec et fallback.
- **Pas de notification documentée** : l'actualisation se fait à 1 s. Ce n'est
  pas un polling extrêmement fréquent, mais un changement externe peut mettre
  jusqu'à ~1 s à apparaître.
- **Fenêtres élevées / intégrité** : le déplacement documenté peut renvoyer
  `E_ACCESSDENIED` ; le fallback privé le contourne lorsqu'Explorer expose la
  vue de la fenêtre. Si les deux voies échouent, aucun déplacement n'est
  simulé.
- **Noms de bureaux** : le nom affiché est « Bureau N », dérivé de l'ordre réel.
  Les noms personnalisés saisis dans Windows ne sont pas lus
  (`IVirtualDesktop::GetName` n'est pas utilisé).
- **Icônes de fenêtres** : la barre affiche un glyphe de fenêtre neutre et les
  trois premières fenêtres du bureau. Les icônes spécifiques à chaque
  application ne sont pas encore résolues ; Linux les obtient via son
  `WsIconCacher`.
- **Bureaux spéciaux** : Hyprland a des workspaces spéciaux ; Windows n'a pas
  d'équivalent. Ils ne sont pas simulés.
- **Bureaux par moniteur** : non implémentés, car Windows ne fonctionne pas
  ainsi ; les deux barres partagent la même liste.
- **Nombre de bureaux** : le pastille grandit avec la liste ; il n'y a pas
  encore de défilement ni de limite « shown » comme dans la configuration
  Linux. Le comportement reste correct pour un usage courant.
- **Drag & drop** : le drag est interne à la barre (icône flottante + ligne
  cible surlignée), pas un drag OLE vers une autre application. Il déplace
  bien la vraie fenêtre via l'API Windows.
- **Erreurs de commande** : `error` existe côté service, mais la barre n'affiche
  pas encore de notification d'erreur ; elle ne montre jamais un état faux.
- **Filtrage** : les fenêtres sans titre, invisibles, tool windows et les
  fenêtres du shell sont exclues ; les fenêtres minimisées sont incluses.
- **Task View / transitions** : le clic programmatique sur la surface XAML de
  Task View dépend de la géométrie de la build ; la synchronisation, elle, ne
  dépend pas de ce clic et a été validée par l'API et les raccourcis.

## 6. Fichiers créés et modifiés

Créés :

- `windows/src/platform/services/virtualdesktopservice.hpp`
- `windows/src/platform/services/virtualdesktopservice.cpp`
- `windows/docs/virtual-desktops-report.md`

Modifiés :

- `windows/src/qml/bar/BarWorkspaces.qml` — remplacement complet du mock par
  la `ListView` réelle, l'indicateur, les icônes, le footer et le drag.
- `windows/src/main.cpp` — singleton `VirtualDesktops`, mode diagnostic
  `--dump-vdesktops` (liste les GUID, noms, états, fenêtres réelles).
- `windows/CMakeLists.txt` — ajout des sources du service.
- `windows/README.md` — la mention « workspaces décoratifs » est corrigée.
- `windows/docs/phase3-report.md` — architecture/API Workspaces mises à jour.

Lecture seule : `shell-main/modules/bar/components/workspaces/*` (Workspaces,
Workspace, ActiveIndicator, OccupiedBg, GapMarkers, SpecialWorkspaces).

## 7. Captures d'écran

Dans `windows/build-release/smoke/` :

- `vdesk-two-desktops.png`, `vdesk-switched-back.png` : deux bureaux, indicateur
  actif et fenêtres réelles.
- `vdesk-drag-inactive.png` : drag d'une icône vers un bureau inactif, icône
  flottante et ligne cible surlignée.
- `vdesk-multiscreen-primary-bar.png`, `vdesk-multiscreen-secondary-bar.png` :
  les deux barres avec la même liste.
- `vdesk-multiscreen-primary.png`, `vdesk-multiscreen-secondary.png` : les deux
  écrans complets.
- `vdesk-final-primary-bar.png`, `vdesk-final-secondary-bar.png` : après
  redémarrage, deux bureaux persistés.
- `vdesk-final-primary.png`, `vdesk-final-secondary.png` : état final
  multi-écrans avant nettoyage.
- `vdesk-final-single-bar.png`, `vdesk-final-single-secondary-bar.png` : état
  final après nettoyage, un seul bureau d'origine.
- `vdesk-final-single.png` : écran principal final (un seul bureau).
- `vdesk-dpi125.png`, `vdesk-dpi125-bar.png`, `vdesk-dpi125-three.png` :
  `QT_SCALE_FACTOR=1.25`, lignes et footer à l'échelle.
- `vdesk-taskview.png`, `vdesk-taskview2.png` : Task View avec « Bureau 1 »,
  « Bureau 2 » et « Nouveau bureau ».
- `overlay-dashboard.png`, `overlay-theme-toggle.png` : smoke test de
  non-régression F10 / F8.

## 8. Tests exécutés

Tous les tests ci-dessous ont été exécutés sur la machine réelle, avec le
service Release, des changements réels dans Windows et des captures. Les
valeurs entre crochets sont celles observées dans le journal
`CAELESTIA_DEBUG_VDESK=1` ou dans `--dump-vdesktops`.

| # | Test | Résultat |
|---|---|---|
| 1 | Affichage des bureaux existants | 1 bureau initial (`7DE1789E-…`, « Bureau 1 », 9–10 fenêtres réelles). Après créations : 2 puis 3 bureaux, avec GUID distincts. |
| 2 | Changement par clic | Clic sur la ligne 2 : `VDESK enqueue 1 <guid>` puis `SwitchDesktop`, modèle actif mis à jour. Testé à 100 %, à 125 % et depuis l'écran secondaire. |
| 3 | Changement par raccourci | `Win+Ctrl+→` puis `Win+Ctrl+←` : le journal du shell passe au bureau adjacent puis revient, en ≤ 1 s. |
| 4 | Changement via Task View | `Win+Tab`, clic sur la carte « Bureau 2 » à `(873,840)` : le bureau réel devient `03B6A314-…`, la barre se synchronise. |
| 5 | Création d'un bureau | Bouton `+` : `CreateDesktop` + activation, compteur 1→2 et 2→3, nouvelle ligne visible. Testé à 100 % et 125 %. |
| 6 | Fermeture d'un bureau | Clic du milieu sur une ligne et bouton `−` : `RemoveDesktop`, compteur décrémenté. Un bureau contenant OpenCode a été fermé : la fenêtre a survécu (`IsWindow=True`) et a été déplacée vers le bureau de repli. |
| 7 | Déplacement d'une fenêtre | Drag d'une icône vers un autre bureau : `VDESK command 4`, déplacement réel. OpenCode est passé de 9/0 à 8/1 puis revenu à 9/0. Fallback privé confirmé (`fallback true result 0`). |
| 8 | Applications sur plusieurs bureaux | OpenCode sur « Bureau 2 », Discord/Firefox/Explorer/VS Code sur « Bureau 1 » ; le modèle expose les bonnes listes et les bons compteurs. |
| 9 | Détection des fenêtres | `--dump-vdesktops` liste 9 fenêtres avec HWND, application et titre réels (Discord, Firefox, OpenCode, Explorateur, VS Code…). Les fenêtres du shell sont exclues. |
| 10 | Synchronisation après changements rapides | Enchaînement de changements externes par COM, Task View et raccourcis : aucun doublon, aucun retour en boucle, état actif correct après chaque changement. |
| 11 | Deux écrans | Barres à `(0,0)` et `(-1920,0)` ; même liste de 2 bureaux. Clic du bureau 2 depuis la barre secondaire à `(-1885,350)` : commande réelle. La fenêtre Firefox à `x=-1867` (écran secondaire) est bien comptée dans le bureau courant. |
| 12 | DPI 100 % | Tous les tests ci-dessus sur le ZOWIE 1920×1080, `dpr 1`. |
| 13 | DPI 125 % | `QT_SCALE_FACTOR=1.25` : barre principale 60×1080, secondaire 75×1080 ; changement de ligne à `(30,350)`, création à `(20,386)`, fermeture au clic du milieu à `(30,395)` ; captures `vdesk-dpi125*`. |
| 14 | Redémarrage de Caelestia | Arrêt/relance avec 2 bureaux : le service retrouve 2 bureaux, le bureau d'origine actif et 9/0 fenêtres ; l'UI affiche 2 lignes. |
| 15 | Absence de doublons / d'états fictifs | GUID uniques, modèle diffé par identifiant, 1 bureau final après nettoyage. En cas d'indisponibilité COM, `available=false` et le composant est masqué (aucune ligne inventée). |

### Nettoyage final

Tous les bureaux de test ont été fermés. Le bureau d'origine
`7DE1789E-C340-4608-B167-B08818ACB5B0` (« Bureau 1 ») est conservé, actif, avec
ses 9 fenêtres réelles. Aucun bureau vide supplémentaire ne subsiste.

### Non-régression

- Aucune modification de `platform.cpp`, des overlays, de la taskbar/AppBar,
  du renderer D3D11 ni des flags `WS_EX_NOACTIVATE`/`WS_EX_TRANSPARENT`.
- L'AppBar reste enregistrée sur les deux écrans (journal `Caelestia AppBar
  screen`).
- Les clics de la barre, le clic d'icône de fenêtre (focus OpenCode confirmé
  par `GetForegroundWindow`) et les panneaux fonctionnent.
- Smoke test overlay : F10 ouvre le Dashboard (OCR « Dashboard / Performance /
  Weather / Media / Workspaces »), F8 bascule le thème (luminance moyenne
  81,9 puis retour à 22,6) et Esc ferme ; F9 a été envoyé dans la même
  séquence. Aucune erreur QML dans les journaux.

## 9. Résultats multi-écrans

- Écran principal : ZOWIE XL LCD `QRect(0,0 1920x1080)`, DPI 96, `dpr 1`.
- Écran secondaire : DISPLAY1 `QRect(-1920,0 1536x864)`, DPI 120, `dpr 1.25`.
- Barre principale : fenêtre 60×1080 ; barre secondaire : 60×864 (et 75×1080
  lorsque `QT_SCALE_FACTOR=1.25` est forcé).
- Les deux barres affichent les mêmes bureaux et le même actif ; changer de
  bureau depuis l'une met l'autre à jour via le même singleton.
- Une fenêtre Firefox positionnée sur l'écran secondaire (`x=-1867`) est
  incluse dans les fenêtres du bureau courant, aux côtés des fenêtres de
  l'écran principal.
- Les bureaux restent ceux de la session Windows : aucun bureau indépendant
  par écran n'est créé.

## 10. Builds

- **Release** : `windows/build-release` — compilation et édition de liens
  réussies après les derniers changements.
- **Debug** : `windows/build` — compilation et édition de liens réussies.

## 11. Commit Git

Commit de la phase Workspaces sur la branche `phase3-desktop-shell` : `e677fa4`.
