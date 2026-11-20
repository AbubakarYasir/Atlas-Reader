pragma Singleton
import QtQuick
import "i18n/Strings.js" as Strings

// Interface language (ADR-0006). English is the default; Arabic and Urdu are
// complete translations. Every binding that calls t() re-evaluates when
// `language` changes, so switching is instant.
QtObject {
    id: i18n

    property string language: "en"
    readonly property bool rtl: language === "ar" || language === "ur"
    readonly property var languages: [
        { code: "en", name: "English" },
        { code: "ar", name: "العربية" },
        { code: "ur", name: "اردو" }
    ]

    // Urdu is read in Nastaliq, which needs a bundled font and more room.
    readonly property string systemFamily: atlasSystemFontFamily
    readonly property string arabicFamily: atlasArabicFontFamily
    readonly property string urduFamily: atlasUrduFontFamily
    readonly property string uiFamily: language === "ur" ? urduFamily : language === "ar" ? arabicFamily : systemFamily
    readonly property real sizeScale: language === "ur" ? 1.12 : 1.0

    // Font for user content (titles, names), chosen by the text's own script so
    // Arabic stays in Naskh and Urdu in Nastaliq whatever the interface language.
    function contentFamily(text) {
        if (/[\u0679\u0688\u0691\u06BA\u06BE\u06C1\u06D2\u06D3]/.test(text)) return urduFamily
        if (/[\u0600-\u06FF\u0750-\u077F\u08A0-\u08FF\uFB50-\uFDFF\uFE70-\uFEFF]/.test(text)) return arabicFamily
        return systemFamily  // other scripts: the system interface font and its fallbacks
    }

    function t(key) {
        const table = Strings.tables[language] || Strings.tables.en
        let text = table[key]
        if (text === undefined || text === "") text = Strings.tables.en[key]
        if (text === undefined) {
            console.warn("I18n: missing key", key)
            return key
        }
        for (let i = 1; i < arguments.length; ++i) {
            text = text.split("%" + i).join(String(arguments[i]))
        }
        return text
    }

    // Plural-aware helper: picks key + ".one" or key + ".other".
    function n(key, count) {
        return t(count === 1 ? key + ".one" : key + ".other", count)
    }

    // Direction marks for text placed inside a sentence (file names, paths,
    // numbers), so it never reorders the words around it. Embedding marks
    // (U+202A-U+202C) are used, not the newer isolates (U+2066-U+2069): the
    // Windows interface font has no isolates, and one missing character makes
    // Qt read every installed font (see src/app/fonts/InterfaceFonts.h).

    // Left to right: paths, versions, addresses.
    function ltr(text) {
        return "\u202A" + text + "\u202C"
    }

    // In the text's own direction: its first Arabic-script or Hebrew letter
    // makes it right to left; numbers and Latin text stay left to right.
    function isolate(text) {
        const value = String(text)
        const strong = /[A-Za-z\u00C0-\u024F\u0370-\u03FF\u0400-\u04FF\u0590-\u08FF\uFB1D-\uFDFF\uFE70-\uFEFC]/.exec(value)
        const rightToLeft = strong !== null && /[\u0590-\u08FF\uFB1D-\uFDFF\uFE70-\uFEFC]/.test(strong[0])
        return (rightToLeft ? "\u202B" : "\u202A") + value + "\u202C"
    }
}
