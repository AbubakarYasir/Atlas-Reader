pragma Singleton
import QtQuick

// Atlas design tokens (docs/DESIGN.md §4). Views never use literal colours,
// sizes or durations; they read them from here.
QtObject {
    id: theme

    // ---- modes (set from preferences at startup and by the user)
    property bool dark: false
    property bool touch: false
    property bool reducedMotion: false
    // Skins (Foxit model): Colourful tints the title bar with the theme colour.
    property bool colorful: true
    property int themeColor: 0
    readonly property var themeColors: ["#28633a", "#3f4a9a", "#16656b", "#6b3a6b", "#8a5a12", "#3c4148"]
    readonly property var themeColorNames: ["green", "indigo", "teal", "plum", "amber", "graphite"]
    // Extra height for scripts with tall line boxes (Urdu Nastaliq).
    property int scriptExtra: 0

    // ---- colour (§4.1). Contrast ratios are documented in DESIGN.md.
    readonly property color canvas: dark ? "#17191d" : "#f5f3ed"
    readonly property color panel: dark ? "#20242a" : "#fffdf8"
    readonly property color raised: dark ? "#292e36" : "#ffffff"
    readonly property color sunken: dark ? "#1b1e23" : "#efece4"
    readonly property color ink: dark ? "#f4f2eb" : "#20231f"
    readonly property color muted: dark ? "#aeb5bf" : "#667064"
    readonly property color line: dark ? "#3a414b" : "#d9ddd4"
    readonly property color controlBorder: dark ? "#757e8a" : "#7d867a"
    readonly property color accent: dark ? "#9fc8a7" : "#28633a"
    readonly property color accentText: dark ? "#17191d" : "#ffffff"
    readonly property color accentSoft: dark ? "#243d2c" : "#e3f0e5"
    readonly property color hover: dark ? "#2f343c" : "#ebe8df"
    readonly property color pressed: dark ? "#373d46" : "#e1ddd2"
    readonly property color danger: dark ? "#ffd7d3" : "#7a201c"
    readonly property color dangerSoft: dark ? "#4a2525" : "#fff0ee"
    readonly property color dangerBorder: "#b54a45"
    readonly property color warning: dark ? "#efc27e" : "#87550b"
    readonly property color warningSoft: dark ? "#3b3330" : "#f6ecdc"
    // Workspace chrome (Foxit's neutral structure in Atlas's warm values; FOXIT_PARITY §1).
    readonly property color chrome: dark ? "#22262c" : "#fbfaf7"
    readonly property color rail: dark ? "#1d2126" : "#f4f2ec"
    readonly property color chromeLine: dark ? "#3a414b" : "#cfd2c8"
    readonly property bool titleBarTinted: colorful && !dark
    readonly property color titleBar: titleBarTinted ? themeColors[themeColor] : (dark ? "#16191d" : "#f4f2ec")
    readonly property color titleBarText: titleBarTinted ? "#ffffff" : ink
    readonly property color titleBarHover: titleBarTinted ? Qt.rgba(1, 1, 1, 0.16) : hover
    readonly property color closeHover: "#c42b1c"   // Windows caption close colour
    readonly property color closeHoverText: "#ffffff"
    readonly property color brand: dark ? accent : themeColors[themeColor]
    readonly property color readerBackdrop: dark ? "#0f1113" : "#e9e6de"
    readonly property color focusRing: accent
    readonly property color pageShadow: dark ? "#66000000" : "#26000000"

    // Reading colours (rendering only, never written to the PDF).
    readonly property var readingPaper: ["#ffffff", "#f4ecd8", "#1b1b1d", "#000000"]

    // ---- spacing (§4.3, 4 px grid)
    readonly property int xs: 4
    readonly property int sm: 8
    readonly property int md: 12
    readonly property int lg: 16
    readonly property int xl: 24
    readonly property int xxl: 32

    // ---- shape
    readonly property int radiusSmall: 4
    readonly property int radiusControl: 8
    readonly property int radiusSurface: 12

    // ---- sizes
    readonly property int controlHeight: (touch ? 40 : 32) + scriptExtra
    readonly property int iconButton: touch ? 40 : 32
    readonly property int iconSize: touch ? 20 : 18
    readonly property int titleBarHeight: (touch ? 56 : 48)
    readonly property int qatButton: touch ? 40 : 32
    readonly property int captionButton: 46
    readonly property int ribbonTabHeight: (touch ? 36 : 28) + scriptExtra
    readonly property int ribbonHeight: (touch ? 80 : 72) + scriptExtra
    readonly property int docTabWidth: 124
    readonly property int railWidth: touch ? 48 : 40
    readonly property int backstageListWidth: 210
    readonly property int tabBarHeight: (touch ? 36 : 28) + scriptExtra
    readonly property int commandBarHeight: (touch ? 48 : 40) + scriptExtra
    readonly property int statusBarHeight: (touch ? 40 : 32) + scriptExtra

    // ---- motion
    readonly property int fast: reducedMotion ? 0 : 120
    readonly property int normal: reducedMotion ? 0 : 180
    readonly property int slow: reducedMotion ? 0 : 240

    // ---- icons (assets/icons, Fluent UI System Icons, MIT)
    function icon(name) {
        return Qt.resolvedUrl("assets/icons/" + name + ".svg")
    }
}
