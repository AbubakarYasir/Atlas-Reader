enum ReadingMode {
  singlePage,
  continuousVertical,
  twoPageSpread,
  twoColumnSplit,
}

enum ReaderThemeMode { day, night, oled, warmParchment }

enum ReaderZoomPreset { fitPage, fitWidth, custom }

class ReaderPreferences {
  const ReaderPreferences({
    this.mode = ReadingMode.continuousVertical,
    this.theme = ReaderThemeMode.day,
    this.brightness = 1.0,
    this.marginCrop = 0.0,
    this.fontFamily = 'Segoe UI',
    this.fontSize = 16.0,
    this.lineSpacing = 1.5,
    this.isResearchMode = false,
    this.pageOffset = 0,
  });

  final ReadingMode mode;
  final ReaderThemeMode theme;
  final double brightness;
  final double marginCrop;
  final String fontFamily;
  final double fontSize;
  final double lineSpacing;
  final bool isResearchMode;
  final int pageOffset;

  ReaderPreferences copyWith({
    ReadingMode? mode,
    ReaderThemeMode? theme,
    double? brightness,
    double? marginCrop,
    String? fontFamily,
    double? fontSize,
    double? lineSpacing,
    bool? isResearchMode,
    int? pageOffset,
  }) {
    return ReaderPreferences(
      mode: mode ?? this.mode,
      theme: theme ?? this.theme,
      brightness: brightness ?? this.brightness,
      marginCrop: marginCrop ?? this.marginCrop,
      fontFamily: fontFamily ?? this.fontFamily,
      fontSize: fontSize ?? this.fontSize,
      lineSpacing: lineSpacing ?? this.lineSpacing,
      isResearchMode: isResearchMode ?? this.isResearchMode,
      pageOffset: pageOffset ?? this.pageOffset,
    );
  }
}
