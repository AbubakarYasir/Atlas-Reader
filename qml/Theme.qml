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
    readonly property var themeColors: ["#0b3d91", "#0a2f6e", "#0b4fae", "#155e75", "#6b3a6b", "#3c4148"]
    readonly property var themeColorNames: ["blue", "midnight", "sky", "teal", "plum", "graphite"]
    // Extra height for scripts with tall line boxes (Urdu Nastaliq).
    property int scriptExtra: 0

    // ---- colour (§4.1). Neutral roles use Foxit's measured skin values
    // (FOXIT_PARITY §2); the accent is Atlas's own. Contrast in DESIGN.md.
    readonly property color canvas: dark ? "#242424" : "#f0f1f5"        // content background
    readonly property color panel: dark ? "#333333" : "#fbfbfd"         // primary background
    readonly property color raised: dark ? "#333333" : "#ffffff"        // cards, fields, dialogs
    readonly property color sunken: dark ? "#333333" : "#f0f1f5"        // gap above tabs, tab strip
    readonly property color docTab: dark ? "#424242" : "#f0f1f5"        // inactive document tab
    readonly property color ink: dark ? "#e3e3e3" : "#131313"           // primary text
    readonly property color inkStrong: dark ? "#f7f7f7" : "#333333"     // titles
    readonly property color muted: dark ? "#bdbdbd" : "#515151"         // introductory text
    readonly property color line: dark ? "#242424" : "#d8dbe5"          // tab and panel borders
    readonly property color spacing: dark ? "#242424" : "#e6e9f0"       // ribbon and menu separators
    readonly property color controlBorder: dark ? "#7a7a7a" : "#9197a8" // inputs, check and radio
    readonly property color buttonBorder: dark ? "#999999" : "#bcc1d1"
    readonly property color accent: dark ? "#6cc4ff" : "#0b4fae"          // Atlas Blue / Sky (logo)
    readonly property color accentText: dark ? "#131313" : "#ffffff"
    readonly property color accentSoft: dark ? "#123a66" : "#e3f1fb"
    readonly property color hover: dark ? "#515151" : "#d8dbe5"
    readonly property color pressed: dark ? "#424242" : "#e6e9f0"       // active / selected tool
    readonly property color danger: dark ? "#f04439" : "#b3261e"
    readonly property color dangerSoft: dark ? "#4a2525" : "#fff0ee"
    readonly property color dangerBorder: "#e34036"
    readonly property color warning: dark ? "#efc27e" : "#87550b"
    readonly property color warningSoft: dark ? "#3b3330" : "#f6ecdc"
    // Workspace chrome (FOXIT_PARITY §1).
    readonly property color chrome: panel                               // title area, ribbon, status bar
    readonly property color rail: dark ? "#424242" : "#f5f6fa"          // nav rail, File page list
    readonly property color chromeLine: dark ? "#131313" : "#c7ccd9"    // frame borders
    readonly property bool titleBarTinted: colorful && !dark
    readonly property color titleBar: titleBarTinted ? themeColors[themeColor] : panel
    readonly property color titleBarText: titleBarTinted ? "#ffffff" : ink
    readonly property color titleBarHover: titleBarTinted ? Qt.rgba(1, 1, 1, 0.16) : hover
    readonly property color highlight: "#00a0f8"   // logo cyan, decorative only
    readonly property color closeHover: "#c42b1c"   // Windows caption close colour
    readonly property color closeHoverText: "#ffffff"
    readonly property color brand: dark ? accent : themeColors[themeColor]
    readonly property color readerBackdrop: canvas
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
