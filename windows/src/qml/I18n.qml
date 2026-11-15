pragma Singleton

import QtQuick
import Caelestia.Platform 1.0

// The shell is written in French: the literals in the QML files are the French
// source strings. `t()` returns them untouched when the Windows display
// language is French, and the English translation otherwise — anything missing
// from the table falls back to the French text rather than to a blank label.
//
// CAELESTIA_LANG=en forces a language without touching the Windows settings,
// which is how the translation gets checked on a French machine.
QtObject {
    id: root

    readonly property string language: Platform.systemLanguage
    readonly property bool french: language === "fr"

    readonly property var english: ({
        "À propos": "About",
        "À propos de Windows": "About Windows",
        "ACCÈS RAPIDE": "QUICK ACCESS",
        "Ajoutez des favoris avec l'étoile dans le launcher.":
            "Add favourites with the star in the launcher.",
        "Arrière-plan Windows": "Windows background",
        "Aucun lecteur actif": "No active player",
        "Aucune application": "No app",
        "Aucune fenêtre active": "No active window",
        "Aucune fenêtre sur le bureau actuel.": "No window on this desktop.",
        "Aucune lecture": "Nothing playing",
        "Aucune notification": "No notification",
        "Aucune session média": "No media session",
        "Barre latérale": "Sidebar",
        "Bureau actuel": "Current desktop",
        "Collecte des données…": "Collecting data…",
        "Configurer la météo": "Set up the weather",
        "Confirmer :": "Confirm:",
        "Couleur libre": "Free colour",
        "Couleurs du shell": "Shell colours",
        "Couleurs Windows": "Windows colours",
        "Coupe l'entrée par défaut, comme le bouton du panneau audio":
            "Mutes the default input, like the audio panel button",
        "Créer une notification de test": "Create a test notification",
        "Échelle système": "System scale",
        "Écran": "Screen",
        "Écran de verrouillage Windows": "Windows lock screen",
        "Écrans": "Screens",
        "Empêcher la mise en veille": "Prevent sleep",
        "Entrée": "Input",
        "État": "Status",
        "ÉTAT DU SYSTÈME": "SYSTEM STATUS",
        "Ex. Paris, Montréal, Tokyo…": "E.g. London, Paris, Tokyo…",
        "F8 prévisualise le thème opposé sans toucher aux réglages Windows.":
            "F8 previews the opposite theme without touching Windows settings.",
        "Fenêtre active": "Active window",
        "Fermer la fenêtre": "Close window",
        "Fond d'écran": "Wallpaper",
        "Garder éveillé": "Keep awake",
        "Humidité": "Humidity",
        "Indiquez une ville. Les données proviennent d’Open-Meteo, sans clé API.":
            "Enter a city. Data comes from Open-Meteo, no API key needed.",
        "Lancez un média compatible GSMTC pour l’afficher ici.":
            "Play a GSMTC-compatible media to show it here.",
        "Les notifications restent dans l'historique sans être affichées":
            "Notifications stay in the history without being shown",
        "Luminosité": "Brightness",
        "Mémoire": "Memory",
        "Microphone coupé": "Microphone muted",
        "Mode d'alimentation": "Power mode",
        "N/D": "N/A",
        "Ne pas déranger": "Do not disturb",
        "Notification de test": "Test notification",
        "Opacité": "Opacity",
        "Paramètres": "Settings",
        "Paramètres audio": "Audio settings",
        "Paramètres Bluetooth": "Bluetooth settings",
        "Paramètres d'alimentation": "Power settings",
        "Paramètres réseau": "Network settings",
        "Périphérique de sortie": "Output device",
        "Périphérique d'entrée": "Input device",
        "PIN et Windows Hello ne sont pas acceptés ici.":
            "PIN and Windows Hello are not accepted here.",
        "Portage Windows du shell Quickshell d'origine, en Qt 6 et C++.":
            "Windows port of the original Quickshell shell, in Qt 6 and C++.",
        "Prévisions": "Forecast",
        "Profil d'alimentation": "Power profile",
        "Raccourcis actifs": "Active shortcuts",
        "Rechercher une application… ou > pour les actions":
            "Search an app… or > for actions",
        "Redessine le shell au rythme de l'écran même à l'arrêt.":
            "Redraws the shell at the screen rate even when idle.",
        "Réduire": "Minimise",
        "Réglages": "Settings",
        "Rendu à 144 fps": "Rendered at 144 fps",
        "Réseau": "Network",
        "Son coupé": "Sound muted",
        "Surfaces du shell": "Shell surfaces",
        "Système": "System",
        "Tableau de bord": "Dashboard",
        "Téléchargement": "Download",
        "Tentative": "Attempt",
        "sur": "of",
        "jours": "days",
        "Dissocier": "Unpair",
        "Confirmer ?": "Confirm?",
        "Appairer": "Pair",
        "appairé": "paired",
        "non appairé": "not paired",
        "Thème": "Theme",
        "Titre inconnu": "Unknown title",
        "Tout est à jour !": "Everything is up to date!",
        "Utilisé": "Used",
        "Vent": "Wind",
        "Verrouillage majuscules activé": "Caps lock is on",
        "Windows réserve certains raccourcis ; ceux qui n'ont pas pu être pris":
            "Windows owns some shortcuts; the ones that could not be taken",
        "Windows réserve certains raccourcis ; ceux qui n'ont pas pu être pris sont listés comme indisponibles et le shell n'a aucun moyen de les forcer. Les raccourcis personnalisés se déclarent dans la configuration du gestionnaire de fenêtres.":
            "Windows owns some shortcuts; the ones that could not be taken are "
            + "listed as unavailable and the shell has no way to force them. "
            + "Custom shortcuts are declared in the window manager settings.",
        "Redessine le shell au rythme de l'écran même à l'arrêt. Plus fluide, mais consomme la batterie.":
            "Redraws the shell at the screen rate even when idle. Smoother, but "
            + "uses more battery.",
        "F8 prévisualise le thème opposé sans toucher aux réglages Windows.":
            "F8 previews the opposite theme without touching Windows settings.",
        "F8 prévisualise le thème opposé sans toucher aux réglages Windows. ":
            "F8 previews the opposite theme without touching Windows settings. ",

        // Second pass: the strings that live inside concatenations, ternaries
        // and the shortcut label table.
        "  ·  Échap fermer": "  ·  Esc close",
        " enregistrés": " registered",
        " fenêtre": " window",
        " fonds d’écran": " wallpapers",
        "↑ ↓ naviguer  ·  Entrée ": "↑ ↓ navigate  ·  Enter ",
        "à l’instant": "just now",
        "à l'instant": "just now",
        "Activer/désactiver le pavage": "Enable/disable tiling",
        "Alimenté par le secteur": "Plugged in",
        "Appliqué immédiatement par le service d'alimentation Windows.":
            "Applied immediately by the Windows power service.",
        "Apparence": "Appearance",
        "Arrêter": "Shut down",
        "Aucun écran disponible.": "No screen available.",
        "Aucun fond d’écran trouvé": "No wallpaper found",
        "Aucun GPU détecté": "No GPU detected",
        "Aucun périphérique de lecture": "No playback device",
        "Aucune action trouvée": "No action found",
        "Aucune application trouvée": "No app found",
        "Aucune donnée": "No data",
        "Aucune fenêtre": "No window",
        "Aucune image trouvée dans les dossiers Windows.":
            "No image found in the Windows folders.",
        "Autonomie estimée par Windows : ": "Battery life estimated by Windows: ",
        "Bluetooth activé": "Bluetooth on",
        "Bluetooth désactivé": "Bluetooth off",
        "Bureau précédent": "Previous desktop",
        "Caelestia suit le thème des applications Windows":
            "Caelestia follows the Windows app theme",
        "Capture d'écran": "Screenshot",
        "Cliquez sur une vignette : Windows applique l'image immédiatement.":
            "Click a thumbnail: Windows applies the image immediately.",
        "Connecté": "Connected",
        "Décharge": "On battery",
        "Déconnecté": "Disconnected",
        "Déplacer à droite": "Move right",
        "Déplacer à gauche": "Move left",
        "Déplacer en bas": "Move down",
        "Déplacer en haut": "Move up",
        "Déplacer vers le bureau ": "Move to desktop ",
        "Désactivation d'urgence": "Emergency disable",
        "Désactivé": "Disabled",
        "Données indisponibles": "Data unavailable",
        "Éditeur": "Editor",
        "Élargir": "Widen",
        "Enregistrée dans ": "Saved to ",
        "Entrée copie le résultat": "Enter copies the result",
        "Entrée copier le résultat  ·  Échap fermer": "Enter copy the result  ·  Esc close",
        "État fourni par Windows": "Status reported by Windows",
        "État inconnu": "Unknown status",
        "État indisponible": "Status unavailable",
        "exécuter": "run",
        "Fenêtre à droite": "Window right",
        "Fenêtre à gauche": "Window left",
        "Fenêtre en bas": "Window down",
        "Fenêtre en haut": "Window up",
        "Fenêtre flottante": "Floating window",
        "Fenêtre précédente": "Previous window",
        "Fenêtre suivante": "Next window",
        "Gestionnaire des tâches": "Task Manager",
        "Humidité ": "Humidity ",
        "L’écran de verrouillage Caelestia va s’ouvrir.":
            "The Caelestia lock screen is about to open.",
        "Le fond du shell laisse voir le fond d'écran. Les cartes des ":
            "The shell surface shows the wallpaper through. Panel cards ",
        "L'enregistreur d'écran du shell d'origine n'a pas été porté : ":
            "The original shell's screen recorder was not ported: ",
        "MÉMOIRE": "MEMORY",
        "Météo": "Weather",
        "Mise en veille autorisée": "Sleep allowed",
        "Non configurée": "Not configured",
        "Ouvrir les réglages du shell et de Windows":
            "Open the shell and Windows settings",
        "Pavage précédent": "Previous layout",
        "Plein écran": "Full screen",
        "Promouvoir en maître": "Promote to master",
        "Redémarrer": "Restart",
        "Réduire la hauteur": "Shrink height",
        "Réduire la largeur": "Shrink width",
        "Réduite": "Minimised",
        "Se déconnecter": "Sign out",
        "Vérification…": "Verifying…",
        "Visualiseur temps réel": "Real-time visualiser",
        "Voir et gérer les processus": "See and manage processes",
        "Windows a refusé de changer la sortie audio.":
            "Windows refused to change the audio output.",
        "Windows a refusé de changer l'entrée audio.":
            "Windows refused to change the audio input.",
        "Windows a refusé le verrouillage de la session.":
            "Windows refused to lock the session.",
        "Zone copiée dans le presse-papiers.": "Area copied to the clipboard."
    })

    function t(text) {
        if (root.french || text === undefined || text === null)
            return text;
        const translated = root.english[text];
        return translated === undefined ? text : translated;
    }

    // Same, for a string built from several pieces: t2("Clé n°", "1/4") would
    // lose the word order in another language.
    function has(text) {
        return root.french || root.english[text] !== undefined;
    }
}
