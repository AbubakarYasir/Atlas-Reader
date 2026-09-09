// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Arabic (`ar`).
class AppLocalizationsAr extends AppLocalizations {
  AppLocalizationsAr([String locale = 'ar']) : super(locale);

  @override
  String get appTitle => 'قارئ أطلس';

  @override
  String get settings => 'الإعدادات';

  @override
  String get language => 'اللغة';

  @override
  String get english => 'الإنجليزية';

  @override
  String get arabic => 'العربية';

  @override
  String get languageDescription => 'اختر لغة التطبيق واتجاه الواجهة';

  @override
  String get libraryFolders => 'مجلدات المكتبة';

  @override
  String get libraryFoldersDescription =>
      'اختر المجلدات التي تريد فحص ملفات PDF فيها';

  @override
  String get browseLibrary => 'تصفح المكتبة';

  @override
  String get searchBookmarks => 'البحث في كل العلامات المرجعية';

  @override
  String get searchHint => 'عنوان علامة مرجعية أو اسم ملف PDF...';

  @override
  String get commandCenterHint => 'أدخل عبارة بحث. اضغط Escape للإغلاق.';

  @override
  String get noMatches => 'لا توجد نتائج.';

  @override
  String get searchLibrary => 'ابحث في العلامات المرجعية ضمن مكتبتك.';

  @override
  String pageNumber(Object number) {
    return 'الصفحة $number';
  }

  @override
  String pageNumberOf(Object page, Object total) {
    return 'الصفحة $page من $total';
  }

  @override
  String get pdfFile => 'ملف PDF';

  @override
  String get folder => 'مجلد';

  @override
  String get syncing => 'تجري مزامنة العلامات المرجعية…';

  @override
  String syncComplete(Object count) {
    return 'اكتملت المزامنة: عُثر على $count علامة مرجعية جديدة.';
  }

  @override
  String get saveComplete => 'تم الحفظ بنجاح.';

  @override
  String saveError(Object error) {
    return 'تعذر الحفظ: $error';
  }

  @override
  String get fileMissing => 'تعذر العثور على ملف PDF.';

  @override
  String bookmarkSaved(Object page) {
    return 'تم حفظ العلامة المرجعية للصفحة $page.';
  }

  @override
  String get bookmarkTitle => 'عنوان العلامة المرجعية';

  @override
  String get rename => 'إعادة تسمية';

  @override
  String get remove => 'إزالة';

  @override
  String get keyboardTreeHint =>
      'استخدم السهمين للأعلى والأسفل للتنقل، واليمين واليسار للتوسيع أو الطي، وEnter للفتح، وF2 لإعادة التسمية، وDelete للإزالة.';

  @override
  String bookmarkTreeItem(
    Object kind,
    Object level,
    Object page,
    Object title,
  ) {
    return '$title، المستوى $level، $kind، $page';
  }

  @override
  String get bookmarkTreeFolder => 'مجلد';

  @override
  String get bookmarkTreeBookmark => 'علامة مرجعية';

  @override
  String openAtPage(Object page, Object title) {
    return 'افتح $title عند الصفحة $page';
  }

  @override
  String openFile(Object title) {
    return 'افتح $title';
  }

  @override
  String get theme => 'السمة';

  @override
  String get themeDescription => 'سمة النظام والفاتحة والداكنة وعالية التباين';
}
