.pragma library

// Caelestia draws one category glyph per window in its workspace pills
// (`utils/Icons.qml`, `getAppCategoryIcon`), fed by each window's DesktopEntry
// categories. Windows has no such categories: `VirtualDesktopService` publishes
// the process executable name as `app`, so the same idea is written here as
// ordered patterns over that name. Most specific families first; `window` last.

const rules = [
    // Terminals. Needles of three characters or fewer must match the whole
    // name, otherwise "wt" would hit any word containing it.
    [["terminal", "powershell", "pwsh", "windowsterminal", "conhost", "alacritty",
        "wezterm", "mintty", "tabby", "kitty", "hyper", "cmd", "wt"], "terminal"],

    [["chrome", "firefox", "msedge", "brave", "vivaldi", "librewolf", "waterfox",
        "chromium", "opera"], "web"],

    [["discord", "whatsapp", "telegram", "slack", "teams", "signal", "messenger",
        "zoom", "skype", "element"], "chat"],

    // Editors before office: "wordpad" must not be read as Word.
    [["notepad", "wordpad", "sublime", "gedit", "kate", "geany"], "edit_note"],

    [["devenv", "pycharm", "webstorm", "phpstorm", "androidstudio", "clion",
        "goland", "rider", "cursor", "windsurf", "neovim", "emacs", "code",
        "zed"], "code"],

    [["explorer", "totalcmd", "doublecmd", "files"], "files"],

    [["spotify", "foobar", "aimp", "musicbee", "groove", "deezer", "tidal",
        "itunes", "music"], "music_note"],

    [["vlc", "mpv", "potplayer", "mediaplayer", "films", "netflix"], "videocam"],

    [["roblox", "steam", "epicgames", "battle", "riot", "valorant", "league",
        "minecraft", "fivem", "fortnite", "genshin", "rockstar", "ubisoft",
        "eaapp", "origin", "gta", "cs2"], "sports_esports"],

    [["winword", "excel", "powerpnt", "onenote", "outlook", "acrobat",
        "acrord32", "word", "wps"], "content_paste"],

    [["photos", "gimp", "photoshop", "illustrator", "krita", "inkscape",
        "paint"], "photo_library"],

    [["thunderbird", "gmail", "mail"], "mail"],

    [["audacity", "flstudio", "ableton", "reaper", "lmms", "bitwig"], "graphic_eq"],

    [["taskmgr", "systeminformer", "processhacker", "hwinfo", "nvidia", "amd",
        "control", "settings"], "monitor_heart"],

    [["store", "winget"], "storefront"]
];

function iconFor(app) {
    const name = String(app === undefined || app === null ? "" : app).toLowerCase();
    if (name === "")
        return "window";

    for (let i = 0; i < rules.length; ++i) {
        const needles = rules[i][0];
        for (let j = 0; j < needles.length; ++j) {
            const needle = needles[j];
            // Short needles ("wt", "cmd", "gta", "cs2") are matched as a
            // prefix so GTA5.exe still lands on the gamepad, while being far
            // too short to be searched for in the middle of a name.
            if (needle.length <= 3 ? name.indexOf(needle) === 0
                                   : name.indexOf(needle) >= 0) {
                return rules[i][1];
            }
        }
    }
    return "window";
}
