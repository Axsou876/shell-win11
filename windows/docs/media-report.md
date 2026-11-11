# Reconstruction Media — rapport final

Date : 2026-11-11 · Build : Release + Debug Qt 6.9.3.
Écran 1920×1080, DPI 96 (100 %), thème monochrome.

## 1. Cause des problèmes Media précédents

- Le visualiseur était décoratif : aucune capture du flux de sortie Windows,
  aucune FFT, donc aucune réaction au son réel.
- La page Media était une colonne approximative, sans la structure Linux
  `CoverVisualiser / Details / LyricsAndSelector` ni les 300 px de sections.
- Aucun arrêt de capture quand la page n’était plus visible.
- Pas de seek GSMTC ; les contrôles se limitaient à précédent/lecture/suivant.
- Animations limitées à l’arc de progression ; pas de transition de pochette,
  de textes, de fond animé ni d’apparition de page.

## 2. Fichiers créés

- `windows/src/platform/services/audioanalyzer.hpp/.cpp`
- `windows/src/platform/services/audiocaptureservice.hpp/.cpp`
- `windows/src/qml/components/AudioVisualizer.qml`
- `windows/docs/media-report.md`

## 3. Fichiers modifiés

- `windows/src/qml/overlay/DashboardMedia.qml` — reconstruction complète.
- `windows/src/qml/overlay/DashboardShell.qml` — `pageActive` Media.
- `windows/src/platform/services/mediaservice.hpp/.cpp` — `canSeek` + `seek()`.
- `windows/src/main.cpp` — singleton QML `AudioCapture`.
- `windows/CMakeLists.txt` — sources et QML.
- `README.md`, `windows/docs/phase3-report.md`.

## 4. API Windows utilisées

| Domaine | API |
|---|---|
| Capture sortie | WASAPI loopback : `IMMDeviceEnumerator::GetDefaultAudioEndpoint`, `IMMDevice::Activate`, `IAudioClient::GetMixFormat/Initialize/Start/Stop`, `IAudioCaptureClient::GetBuffer/ReleaseBuffer` |
| Périphérique | `IMMDevice::GetId`, `IPropertyStore::GetValue` (nom convivial), détection du changement de périphérique par défaut toutes les 2 s, `AUDCLNT_E_DEVICE_INVALIDATED` |
| Session média | GSMTC `GlobalSystemMediaTransportControlsSessionManager`, `TryChangePlaybackPositionAsync`, `IsPlaybackPositionEnabled` |
| Threads | `std::thread`, COM MTA, publication Qt `QueuedConnection` |
| Rendu | Qt Quick Canvas 64 barres radiales, données `QVariantList` |

## 5. Capture audio

- Périphérique de rendu par défaut (`eRender`, `eConsole`), mode **shared
  loopback**.
- Thread dédié, priorité au-dessus de la normale, `CoInitializeEx(MTA)`.
- Lecture par polling 10 ms (`GetNextPacketSize`/`GetBuffer`) ; aucun sample
  écrit sur disque.
- Conversion en mono float : float32, PCM 16, PCM 24, PCM 32 ; downmix moyenne
  des canaux.
- Silence (`AUDCLNT_BUFFERFLAGS_SILENT`) injecté comme zéros.
- Changement de périphérique : comparaison de l’ID par défaut toutes les 2 s et
  redémarrage du client ; erreurs/invalidation : arrêt propre puis nouvelle
  tentative.
- Démarrage uniquement quand l’onglet Media est visible
  (`DashboardMedia.pageActive` = dashboard visible + onglet Media) ; arrêt et
  join du thread sinon.

## 6. Analyse FFT

- Fenêtre **2048 échantillons**, fenêtre de Hann.
- FFT radix-2 itérative maison, magnitudes normalisées.
- **64 bandes log-spacées de 30 Hz à 18 kHz**.
- Normalisation dB, peak par bande, lissage attaque 0,55 / relâchement 0,095
  (retour progressif au repos).
- Publication à ~60 Hz (16 ms) de 64 floats ; aucun traitement FFT sur le
  thread GUI.

## 7. Architecture

```
WASAPI loopback (thread)
  → downmix mono float
  → AudioAnalyzer (ring 2048, Hann, FFT, 64 bandes)
  → AudioCaptureService (QVariantList, 60 Hz, QueuedConnection)
  → AudioVisualizer.qml (Canvas radial)
```

Le service expose aussi `available`, `error` et `deviceName` pour les états
indisponibles. L’arrêt libère le client audio et le thread.

## 8. Animations reproduites

- Barres radiales réelles autour de la pochette (64 barres, forme Linux
  `CoverVisualiser`).
- Transition de pochette : fondu + échelle à chaque changement d’artwork.
- Textes titre/artiste/album avec crossfade (`StyledText.animate`).
- Apparition de page : fondu + échelle quand l’onglet Media devient actif.
- Fond : 14 formes monochromes qui dérivent et tournent pendant la lecture
  (équivalent approché de `BackgroundShapes`).
- Contrôles : survol/ripple/états actifs/désactivés existants.
- Curseur de progression réel et seek GSMTC.

## 9. Captures avant / après

**Avant** : `weather-fixed-media.png` (ancienne colonne, visualiseur absent).

**Après** (dans `windows/build-release/smoke/`) :

- `media-final-a.png` — Spotify « Zoo », lecture.
- `media-final-paused.png` — pause réelle, titre figé à 4:21.
- `media-final-playing.png` — WAV de test, visualiseur actif.
- `media-final-silence.png` — silence, barres retombées.
- `media-final-reopen.png` — dashboard fermé/rouvert, capture relancée.
- `media-125.png` — 125 % (QT_SCALE_FACTOR).
- `vis-02.png`, `vis-09.png` — audio réel pendant les tons de test.

## 10. Tests réellement exécutés

- **Audio réel Windows** : WAV généré (100 Hz, 1 kHz, 8 kHz, mix, silence)
  joué sur la sortie par défaut, plus Spotify réel.
- **Visualiseur** : comptage de pixels lumineux dans l’anneau (118–158 px) :
  - Spotify en lecture : **2368** ;
  - Spotify en pause : **509** ;
  - WAV de test : **3059** ;
  - silence après arrêt du WAV : **534** ;
  - après fermeture/réouverture du dashboard + WAV : **3059**.
  Cela démontre une réaction au flux réel et un retour au repos.
- **Pause** : position figée à 4:21, libellé « En pause ».
- **Changement de morceau** : Amsterdam (Moha La Squale) → Zoo (Kaaris),
  pochette/titres mis à jour sans décalage de layout.
- **Seek** : glissement du curseur, position déplacée vers le début.
- **Fermeture/réouverture** : capture arrêtée puis relancée (comptage 3059).
- **100 %** : captures ci-dessus, aucune erreur QML.
- **125 %** : `QT_SCALE_FACTOR=1.25`, layout conservé, visualiseur actif
  (6154 pixels dans l’anneau), aucune erreur QML.
- **CPU** : ~4–6 % du CPU total (8 cœurs) pendant la session Media ; mesure
  bruitée par les autres timers du shell, pas un benchmark isolé.
- **Build** : Release et Debug réussissent ; aucun `qrc:`, `TypeError`,
  `ReferenceError`, `polish` ou `Cannot specify`.

## 11. Limites restantes

- Changement de périphérique audio réel et déconnexion non testés
  physiquement ; le code les détecte et réessaie.
- Sélecteur multi-lecteurs et paroles non implémentés (GSMTC n’expose pas les
  paroles ; un seul lecteur courant est utilisé).
- Forme de pochette Linux `Cookie9Sided` approchée par un cercle.
- Test 60 Hz / 144 Hz non réalisé (le rendu est synchronisé au compositeur ;
  la fréquence d’écran n’a pas été modifiée).
- Le fond animé utilise des formes arrondies monochromes, pas les
  `MaterialShape` exactes.
