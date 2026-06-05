// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'database.dart';

// ignore_for_file: type=lint
class $BookmarksTable extends Bookmarks
    with TableInfo<$BookmarksTable, Bookmark> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $BookmarksTable(this.attachedDatabase, [this._alias]);
  static const VerificationMeta _idMeta = const VerificationMeta('id');
  @override
  late final GeneratedColumn<int> id = GeneratedColumn<int>(
    'id',
    aliasedName,
    false,
    hasAutoIncrement: true,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'PRIMARY KEY AUTOINCREMENT',
    ),
  );
  static const VerificationMeta _filePathMeta = const VerificationMeta(
    'filePath',
  );
  @override
  late final GeneratedColumn<String> filePath = GeneratedColumn<String>(
    'file_path',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _titleMeta = const VerificationMeta('title');
  @override
  late final GeneratedColumn<String> title = GeneratedColumn<String>(
    'title',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _pageIndexMeta = const VerificationMeta(
    'pageIndex',
  );
  @override
  late final GeneratedColumn<int> pageIndex = GeneratedColumn<int>(
    'page_index',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _createdAtMeta = const VerificationMeta(
    'createdAt',
  );
  @override
  late final GeneratedColumn<DateTime> createdAt = GeneratedColumn<DateTime>(
    'created_at',
    aliasedName,
    false,
    type: DriftSqlType.dateTime,
    requiredDuringInsert: false,
    defaultValue: currentDateAndTime,
  );
  static const VerificationMeta _modifiedAtMeta = const VerificationMeta(
    'modifiedAt',
  );
  @override
  late final GeneratedColumn<DateTime> modifiedAt = GeneratedColumn<DateTime>(
    'modified_at',
    aliasedName,
    false,
    type: DriftSqlType.dateTime,
    requiredDuringInsert: false,
    defaultValue: currentDateAndTime,
  );
  @override
  List<GeneratedColumn> get $columns => [
    id,
    filePath,
    title,
    pageIndex,
    createdAt,
    modifiedAt,
  ];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'bookmarks';
  @override
  VerificationContext validateIntegrity(
    Insertable<Bookmark> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    }
    if (data.containsKey('file_path')) {
      context.handle(
        _filePathMeta,
        filePath.isAcceptableOrUnknown(data['file_path']!, _filePathMeta),
      );
    } else if (isInserting) {
      context.missing(_filePathMeta);
    }
    if (data.containsKey('title')) {
      context.handle(
        _titleMeta,
        title.isAcceptableOrUnknown(data['title']!, _titleMeta),
      );
    } else if (isInserting) {
      context.missing(_titleMeta);
    }
    if (data.containsKey('page_index')) {
      context.handle(
        _pageIndexMeta,
        pageIndex.isAcceptableOrUnknown(data['page_index']!, _pageIndexMeta),
      );
    } else if (isInserting) {
      context.missing(_pageIndexMeta);
    }
    if (data.containsKey('created_at')) {
      context.handle(
        _createdAtMeta,
        createdAt.isAcceptableOrUnknown(data['created_at']!, _createdAtMeta),
      );
    }
    if (data.containsKey('modified_at')) {
      context.handle(
        _modifiedAtMeta,
        modifiedAt.isAcceptableOrUnknown(data['modified_at']!, _modifiedAtMeta),
      );
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  Bookmark map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return Bookmark(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      filePath: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}file_path'],
      )!,
      title: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}title'],
      )!,
      pageIndex: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}page_index'],
      )!,
      createdAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}created_at'],
      )!,
      modifiedAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}modified_at'],
      )!,
    );
  }

  @override
  $BookmarksTable createAlias(String alias) {
    return $BookmarksTable(attachedDatabase, alias);
  }
}

class Bookmark extends DataClass implements Insertable<Bookmark> {
  final int id;
  final String filePath;
  final String title;
  final int pageIndex;
  final DateTime createdAt;
  final DateTime modifiedAt;
  const Bookmark({
    required this.id,
    required this.filePath,
    required this.title,
    required this.pageIndex,
    required this.createdAt,
    required this.modifiedAt,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['file_path'] = Variable<String>(filePath);
    map['title'] = Variable<String>(title);
    map['page_index'] = Variable<int>(pageIndex);
    map['created_at'] = Variable<DateTime>(createdAt);
    map['modified_at'] = Variable<DateTime>(modifiedAt);
    return map;
  }

  BookmarksCompanion toCompanion(bool nullToAbsent) {
    return BookmarksCompanion(
      id: Value(id),
      filePath: Value(filePath),
      title: Value(title),
      pageIndex: Value(pageIndex),
      createdAt: Value(createdAt),
      modifiedAt: Value(modifiedAt),
    );
  }

  factory Bookmark.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return Bookmark(
      id: serializer.fromJson<int>(json['id']),
      filePath: serializer.fromJson<String>(json['filePath']),
      title: serializer.fromJson<String>(json['title']),
      pageIndex: serializer.fromJson<int>(json['pageIndex']),
      createdAt: serializer.fromJson<DateTime>(json['createdAt']),
      modifiedAt: serializer.fromJson<DateTime>(json['modifiedAt']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'filePath': serializer.toJson<String>(filePath),
      'title': serializer.toJson<String>(title),
      'pageIndex': serializer.toJson<int>(pageIndex),
      'createdAt': serializer.toJson<DateTime>(createdAt),
      'modifiedAt': serializer.toJson<DateTime>(modifiedAt),
    };
  }

  Bookmark copyWith({
    int? id,
    String? filePath,
    String? title,
    int? pageIndex,
    DateTime? createdAt,
    DateTime? modifiedAt,
  }) => Bookmark(
    id: id ?? this.id,
    filePath: filePath ?? this.filePath,
    title: title ?? this.title,
    pageIndex: pageIndex ?? this.pageIndex,
    createdAt: createdAt ?? this.createdAt,
    modifiedAt: modifiedAt ?? this.modifiedAt,
  );
  Bookmark copyWithCompanion(BookmarksCompanion data) {
    return Bookmark(
      id: data.id.present ? data.id.value : this.id,
      filePath: data.filePath.present ? data.filePath.value : this.filePath,
      title: data.title.present ? data.title.value : this.title,
      pageIndex: data.pageIndex.present ? data.pageIndex.value : this.pageIndex,
      createdAt: data.createdAt.present ? data.createdAt.value : this.createdAt,
      modifiedAt: data.modifiedAt.present
          ? data.modifiedAt.value
          : this.modifiedAt,
    );
  }

  @override
  String toString() {
    return (StringBuffer('Bookmark(')
          ..write('id: $id, ')
          ..write('filePath: $filePath, ')
          ..write('title: $title, ')
          ..write('pageIndex: $pageIndex, ')
          ..write('createdAt: $createdAt, ')
          ..write('modifiedAt: $modifiedAt')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode =>
      Object.hash(id, filePath, title, pageIndex, createdAt, modifiedAt);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is Bookmark &&
          other.id == this.id &&
          other.filePath == this.filePath &&
          other.title == this.title &&
          other.pageIndex == this.pageIndex &&
          other.createdAt == this.createdAt &&
          other.modifiedAt == this.modifiedAt);
}

class BookmarksCompanion extends UpdateCompanion<Bookmark> {
  final Value<int> id;
  final Value<String> filePath;
  final Value<String> title;
  final Value<int> pageIndex;
  final Value<DateTime> createdAt;
  final Value<DateTime> modifiedAt;
  const BookmarksCompanion({
    this.id = const Value.absent(),
    this.filePath = const Value.absent(),
    this.title = const Value.absent(),
    this.pageIndex = const Value.absent(),
    this.createdAt = const Value.absent(),
    this.modifiedAt = const Value.absent(),
  });
  BookmarksCompanion.insert({
    this.id = const Value.absent(),
    required String filePath,
    required String title,
    required int pageIndex,
    this.createdAt = const Value.absent(),
    this.modifiedAt = const Value.absent(),
  }) : filePath = Value(filePath),
       title = Value(title),
       pageIndex = Value(pageIndex);
  static Insertable<Bookmark> custom({
    Expression<int>? id,
    Expression<String>? filePath,
    Expression<String>? title,
    Expression<int>? pageIndex,
    Expression<DateTime>? createdAt,
    Expression<DateTime>? modifiedAt,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (filePath != null) 'file_path': filePath,
      if (title != null) 'title': title,
      if (pageIndex != null) 'page_index': pageIndex,
      if (createdAt != null) 'created_at': createdAt,
      if (modifiedAt != null) 'modified_at': modifiedAt,
    });
  }

  BookmarksCompanion copyWith({
    Value<int>? id,
    Value<String>? filePath,
    Value<String>? title,
    Value<int>? pageIndex,
    Value<DateTime>? createdAt,
    Value<DateTime>? modifiedAt,
  }) {
    return BookmarksCompanion(
      id: id ?? this.id,
      filePath: filePath ?? this.filePath,
      title: title ?? this.title,
      pageIndex: pageIndex ?? this.pageIndex,
      createdAt: createdAt ?? this.createdAt,
      modifiedAt: modifiedAt ?? this.modifiedAt,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (filePath.present) {
      map['file_path'] = Variable<String>(filePath.value);
    }
    if (title.present) {
      map['title'] = Variable<String>(title.value);
    }
    if (pageIndex.present) {
      map['page_index'] = Variable<int>(pageIndex.value);
    }
    if (createdAt.present) {
      map['created_at'] = Variable<DateTime>(createdAt.value);
    }
    if (modifiedAt.present) {
      map['modified_at'] = Variable<DateTime>(modifiedAt.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('BookmarksCompanion(')
          ..write('id: $id, ')
          ..write('filePath: $filePath, ')
          ..write('title: $title, ')
          ..write('pageIndex: $pageIndex, ')
          ..write('createdAt: $createdAt, ')
          ..write('modifiedAt: $modifiedAt')
          ..write(')'))
        .toString();
  }
}

class $FileSnapshotsTable extends FileSnapshots
    with TableInfo<$FileSnapshotsTable, FileSnapshot> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $FileSnapshotsTable(this.attachedDatabase, [this._alias]);
  static const VerificationMeta _idMeta = const VerificationMeta('id');
  @override
  late final GeneratedColumn<int> id = GeneratedColumn<int>(
    'id',
    aliasedName,
    false,
    hasAutoIncrement: true,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'PRIMARY KEY AUTOINCREMENT',
    ),
  );
  static const VerificationMeta _filePathMeta = const VerificationMeta(
    'filePath',
  );
  @override
  late final GeneratedColumn<String> filePath = GeneratedColumn<String>(
    'file_path',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _lastKnownStateMeta = const VerificationMeta(
    'lastKnownState',
  );
  @override
  late final GeneratedColumn<String> lastKnownState = GeneratedColumn<String>(
    'last_known_state',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _updatedAtMeta = const VerificationMeta(
    'updatedAt',
  );
  @override
  late final GeneratedColumn<DateTime> updatedAt = GeneratedColumn<DateTime>(
    'updated_at',
    aliasedName,
    false,
    type: DriftSqlType.dateTime,
    requiredDuringInsert: false,
    defaultValue: currentDateAndTime,
  );
  @override
  List<GeneratedColumn> get $columns => [
    id,
    filePath,
    lastKnownState,
    updatedAt,
  ];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'file_snapshots';
  @override
  VerificationContext validateIntegrity(
    Insertable<FileSnapshot> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    }
    if (data.containsKey('file_path')) {
      context.handle(
        _filePathMeta,
        filePath.isAcceptableOrUnknown(data['file_path']!, _filePathMeta),
      );
    } else if (isInserting) {
      context.missing(_filePathMeta);
    }
    if (data.containsKey('last_known_state')) {
      context.handle(
        _lastKnownStateMeta,
        lastKnownState.isAcceptableOrUnknown(
          data['last_known_state']!,
          _lastKnownStateMeta,
        ),
      );
    } else if (isInserting) {
      context.missing(_lastKnownStateMeta);
    }
    if (data.containsKey('updated_at')) {
      context.handle(
        _updatedAtMeta,
        updatedAt.isAcceptableOrUnknown(data['updated_at']!, _updatedAtMeta),
      );
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  FileSnapshot map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return FileSnapshot(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      filePath: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}file_path'],
      )!,
      lastKnownState: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}last_known_state'],
      )!,
      updatedAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}updated_at'],
      )!,
    );
  }

  @override
  $FileSnapshotsTable createAlias(String alias) {
    return $FileSnapshotsTable(attachedDatabase, alias);
  }
}

class FileSnapshot extends DataClass implements Insertable<FileSnapshot> {
  final int id;
  final String filePath;
  final String lastKnownState;
  final DateTime updatedAt;
  const FileSnapshot({
    required this.id,
    required this.filePath,
    required this.lastKnownState,
    required this.updatedAt,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['file_path'] = Variable<String>(filePath);
    map['last_known_state'] = Variable<String>(lastKnownState);
    map['updated_at'] = Variable<DateTime>(updatedAt);
    return map;
  }

  FileSnapshotsCompanion toCompanion(bool nullToAbsent) {
    return FileSnapshotsCompanion(
      id: Value(id),
      filePath: Value(filePath),
      lastKnownState: Value(lastKnownState),
      updatedAt: Value(updatedAt),
    );
  }

  factory FileSnapshot.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return FileSnapshot(
      id: serializer.fromJson<int>(json['id']),
      filePath: serializer.fromJson<String>(json['filePath']),
      lastKnownState: serializer.fromJson<String>(json['lastKnownState']),
      updatedAt: serializer.fromJson<DateTime>(json['updatedAt']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'filePath': serializer.toJson<String>(filePath),
      'lastKnownState': serializer.toJson<String>(lastKnownState),
      'updatedAt': serializer.toJson<DateTime>(updatedAt),
    };
  }

  FileSnapshot copyWith({
    int? id,
    String? filePath,
    String? lastKnownState,
    DateTime? updatedAt,
  }) => FileSnapshot(
    id: id ?? this.id,
    filePath: filePath ?? this.filePath,
    lastKnownState: lastKnownState ?? this.lastKnownState,
    updatedAt: updatedAt ?? this.updatedAt,
  );
  FileSnapshot copyWithCompanion(FileSnapshotsCompanion data) {
    return FileSnapshot(
      id: data.id.present ? data.id.value : this.id,
      filePath: data.filePath.present ? data.filePath.value : this.filePath,
      lastKnownState: data.lastKnownState.present
          ? data.lastKnownState.value
          : this.lastKnownState,
      updatedAt: data.updatedAt.present ? data.updatedAt.value : this.updatedAt,
    );
  }

  @override
  String toString() {
    return (StringBuffer('FileSnapshot(')
          ..write('id: $id, ')
          ..write('filePath: $filePath, ')
          ..write('lastKnownState: $lastKnownState, ')
          ..write('updatedAt: $updatedAt')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(id, filePath, lastKnownState, updatedAt);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is FileSnapshot &&
          other.id == this.id &&
          other.filePath == this.filePath &&
          other.lastKnownState == this.lastKnownState &&
          other.updatedAt == this.updatedAt);
}

class FileSnapshotsCompanion extends UpdateCompanion<FileSnapshot> {
  final Value<int> id;
  final Value<String> filePath;
  final Value<String> lastKnownState;
  final Value<DateTime> updatedAt;
  const FileSnapshotsCompanion({
    this.id = const Value.absent(),
    this.filePath = const Value.absent(),
    this.lastKnownState = const Value.absent(),
    this.updatedAt = const Value.absent(),
  });
  FileSnapshotsCompanion.insert({
    this.id = const Value.absent(),
    required String filePath,
    required String lastKnownState,
    this.updatedAt = const Value.absent(),
  }) : filePath = Value(filePath),
       lastKnownState = Value(lastKnownState);
  static Insertable<FileSnapshot> custom({
    Expression<int>? id,
    Expression<String>? filePath,
    Expression<String>? lastKnownState,
    Expression<DateTime>? updatedAt,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (filePath != null) 'file_path': filePath,
      if (lastKnownState != null) 'last_known_state': lastKnownState,
      if (updatedAt != null) 'updated_at': updatedAt,
    });
  }

  FileSnapshotsCompanion copyWith({
    Value<int>? id,
    Value<String>? filePath,
    Value<String>? lastKnownState,
    Value<DateTime>? updatedAt,
  }) {
    return FileSnapshotsCompanion(
      id: id ?? this.id,
      filePath: filePath ?? this.filePath,
      lastKnownState: lastKnownState ?? this.lastKnownState,
      updatedAt: updatedAt ?? this.updatedAt,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (filePath.present) {
      map['file_path'] = Variable<String>(filePath.value);
    }
    if (lastKnownState.present) {
      map['last_known_state'] = Variable<String>(lastKnownState.value);
    }
    if (updatedAt.present) {
      map['updated_at'] = Variable<DateTime>(updatedAt.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('FileSnapshotsCompanion(')
          ..write('id: $id, ')
          ..write('filePath: $filePath, ')
          ..write('lastKnownState: $lastKnownState, ')
          ..write('updatedAt: $updatedAt')
          ..write(')'))
        .toString();
  }
}

abstract class _$AppDatabase extends GeneratedDatabase {
  _$AppDatabase(QueryExecutor e) : super(e);
  $AppDatabaseManager get managers => $AppDatabaseManager(this);
  late final $BookmarksTable bookmarks = $BookmarksTable(this);
  late final $FileSnapshotsTable fileSnapshots = $FileSnapshotsTable(this);
  @override
  Iterable<TableInfo<Table, Object?>> get allTables =>
      allSchemaEntities.whereType<TableInfo<Table, Object?>>();
  @override
  List<DatabaseSchemaEntity> get allSchemaEntities => [
    bookmarks,
    fileSnapshots,
  ];
}

typedef $$BookmarksTableCreateCompanionBuilder =
    BookmarksCompanion Function({
      Value<int> id,
      required String filePath,
      required String title,
      required int pageIndex,
      Value<DateTime> createdAt,
      Value<DateTime> modifiedAt,
    });
typedef $$BookmarksTableUpdateCompanionBuilder =
    BookmarksCompanion Function({
      Value<int> id,
      Value<String> filePath,
      Value<String> title,
      Value<int> pageIndex,
      Value<DateTime> createdAt,
      Value<DateTime> modifiedAt,
    });

class $$BookmarksTableFilterComposer
    extends Composer<_$AppDatabase, $BookmarksTable> {
  $$BookmarksTableFilterComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  ColumnFilters<int> get id => $composableBuilder(
    column: $table.id,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get filePath => $composableBuilder(
    column: $table.filePath,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get title => $composableBuilder(
    column: $table.title,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get pageIndex => $composableBuilder(
    column: $table.pageIndex,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get createdAt => $composableBuilder(
    column: $table.createdAt,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get modifiedAt => $composableBuilder(
    column: $table.modifiedAt,
    builder: (column) => ColumnFilters(column),
  );
}

class $$BookmarksTableOrderingComposer
    extends Composer<_$AppDatabase, $BookmarksTable> {
  $$BookmarksTableOrderingComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  ColumnOrderings<int> get id => $composableBuilder(
    column: $table.id,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get filePath => $composableBuilder(
    column: $table.filePath,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get title => $composableBuilder(
    column: $table.title,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get pageIndex => $composableBuilder(
    column: $table.pageIndex,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get createdAt => $composableBuilder(
    column: $table.createdAt,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get modifiedAt => $composableBuilder(
    column: $table.modifiedAt,
    builder: (column) => ColumnOrderings(column),
  );
}

class $$BookmarksTableAnnotationComposer
    extends Composer<_$AppDatabase, $BookmarksTable> {
  $$BookmarksTableAnnotationComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  GeneratedColumn<int> get id =>
      $composableBuilder(column: $table.id, builder: (column) => column);

  GeneratedColumn<String> get filePath =>
      $composableBuilder(column: $table.filePath, builder: (column) => column);

  GeneratedColumn<String> get title =>
      $composableBuilder(column: $table.title, builder: (column) => column);

  GeneratedColumn<int> get pageIndex =>
      $composableBuilder(column: $table.pageIndex, builder: (column) => column);

  GeneratedColumn<DateTime> get createdAt =>
      $composableBuilder(column: $table.createdAt, builder: (column) => column);

  GeneratedColumn<DateTime> get modifiedAt => $composableBuilder(
    column: $table.modifiedAt,
    builder: (column) => column,
  );
}

class $$BookmarksTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $BookmarksTable,
          Bookmark,
          $$BookmarksTableFilterComposer,
          $$BookmarksTableOrderingComposer,
          $$BookmarksTableAnnotationComposer,
          $$BookmarksTableCreateCompanionBuilder,
          $$BookmarksTableUpdateCompanionBuilder,
          (Bookmark, BaseReferences<_$AppDatabase, $BookmarksTable, Bookmark>),
          Bookmark,
          PrefetchHooks Function()
        > {
  $$BookmarksTableTableManager(_$AppDatabase db, $BookmarksTable table)
    : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$BookmarksTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$BookmarksTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$BookmarksTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<String> filePath = const Value.absent(),
                Value<String> title = const Value.absent(),
                Value<int> pageIndex = const Value.absent(),
                Value<DateTime> createdAt = const Value.absent(),
                Value<DateTime> modifiedAt = const Value.absent(),
              }) => BookmarksCompanion(
                id: id,
                filePath: filePath,
                title: title,
                pageIndex: pageIndex,
                createdAt: createdAt,
                modifiedAt: modifiedAt,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                required String filePath,
                required String title,
                required int pageIndex,
                Value<DateTime> createdAt = const Value.absent(),
                Value<DateTime> modifiedAt = const Value.absent(),
              }) => BookmarksCompanion.insert(
                id: id,
                filePath: filePath,
                title: title,
                pageIndex: pageIndex,
                createdAt: createdAt,
                modifiedAt: modifiedAt,
              ),
          withReferenceMapper: (p0) => p0
              .map((e) => (e.readTable(table), BaseReferences(db, table, e)))
              .toList(),
          prefetchHooksCallback: null,
        ),
      );
}

typedef $$BookmarksTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $BookmarksTable,
      Bookmark,
      $$BookmarksTableFilterComposer,
      $$BookmarksTableOrderingComposer,
      $$BookmarksTableAnnotationComposer,
      $$BookmarksTableCreateCompanionBuilder,
      $$BookmarksTableUpdateCompanionBuilder,
      (Bookmark, BaseReferences<_$AppDatabase, $BookmarksTable, Bookmark>),
      Bookmark,
      PrefetchHooks Function()
    >;
typedef $$FileSnapshotsTableCreateCompanionBuilder =
    FileSnapshotsCompanion Function({
      Value<int> id,
      required String filePath,
      required String lastKnownState,
      Value<DateTime> updatedAt,
    });
typedef $$FileSnapshotsTableUpdateCompanionBuilder =
    FileSnapshotsCompanion Function({
      Value<int> id,
      Value<String> filePath,
      Value<String> lastKnownState,
      Value<DateTime> updatedAt,
    });

class $$FileSnapshotsTableFilterComposer
    extends Composer<_$AppDatabase, $FileSnapshotsTable> {
  $$FileSnapshotsTableFilterComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  ColumnFilters<int> get id => $composableBuilder(
    column: $table.id,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get filePath => $composableBuilder(
    column: $table.filePath,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get lastKnownState => $composableBuilder(
    column: $table.lastKnownState,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get updatedAt => $composableBuilder(
    column: $table.updatedAt,
    builder: (column) => ColumnFilters(column),
  );
}

class $$FileSnapshotsTableOrderingComposer
    extends Composer<_$AppDatabase, $FileSnapshotsTable> {
  $$FileSnapshotsTableOrderingComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  ColumnOrderings<int> get id => $composableBuilder(
    column: $table.id,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get filePath => $composableBuilder(
    column: $table.filePath,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get lastKnownState => $composableBuilder(
    column: $table.lastKnownState,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get updatedAt => $composableBuilder(
    column: $table.updatedAt,
    builder: (column) => ColumnOrderings(column),
  );
}

class $$FileSnapshotsTableAnnotationComposer
    extends Composer<_$AppDatabase, $FileSnapshotsTable> {
  $$FileSnapshotsTableAnnotationComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  GeneratedColumn<int> get id =>
      $composableBuilder(column: $table.id, builder: (column) => column);

  GeneratedColumn<String> get filePath =>
      $composableBuilder(column: $table.filePath, builder: (column) => column);

  GeneratedColumn<String> get lastKnownState => $composableBuilder(
    column: $table.lastKnownState,
    builder: (column) => column,
  );

  GeneratedColumn<DateTime> get updatedAt =>
      $composableBuilder(column: $table.updatedAt, builder: (column) => column);
}

class $$FileSnapshotsTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $FileSnapshotsTable,
          FileSnapshot,
          $$FileSnapshotsTableFilterComposer,
          $$FileSnapshotsTableOrderingComposer,
          $$FileSnapshotsTableAnnotationComposer,
          $$FileSnapshotsTableCreateCompanionBuilder,
          $$FileSnapshotsTableUpdateCompanionBuilder,
          (
            FileSnapshot,
            BaseReferences<_$AppDatabase, $FileSnapshotsTable, FileSnapshot>,
          ),
          FileSnapshot,
          PrefetchHooks Function()
        > {
  $$FileSnapshotsTableTableManager(_$AppDatabase db, $FileSnapshotsTable table)
    : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$FileSnapshotsTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$FileSnapshotsTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$FileSnapshotsTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<String> filePath = const Value.absent(),
                Value<String> lastKnownState = const Value.absent(),
                Value<DateTime> updatedAt = const Value.absent(),
              }) => FileSnapshotsCompanion(
                id: id,
                filePath: filePath,
                lastKnownState: lastKnownState,
                updatedAt: updatedAt,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                required String filePath,
                required String lastKnownState,
                Value<DateTime> updatedAt = const Value.absent(),
              }) => FileSnapshotsCompanion.insert(
                id: id,
                filePath: filePath,
                lastKnownState: lastKnownState,
                updatedAt: updatedAt,
              ),
          withReferenceMapper: (p0) => p0
              .map((e) => (e.readTable(table), BaseReferences(db, table, e)))
              .toList(),
          prefetchHooksCallback: null,
        ),
      );
}

typedef $$FileSnapshotsTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $FileSnapshotsTable,
      FileSnapshot,
      $$FileSnapshotsTableFilterComposer,
      $$FileSnapshotsTableOrderingComposer,
      $$FileSnapshotsTableAnnotationComposer,
      $$FileSnapshotsTableCreateCompanionBuilder,
      $$FileSnapshotsTableUpdateCompanionBuilder,
      (
        FileSnapshot,
        BaseReferences<_$AppDatabase, $FileSnapshotsTable, FileSnapshot>,
      ),
      FileSnapshot,
      PrefetchHooks Function()
    >;

class $AppDatabaseManager {
  final _$AppDatabase _db;
  $AppDatabaseManager(this._db);
  $$BookmarksTableTableManager get bookmarks =>
      $$BookmarksTableTableManager(_db, _db.bookmarks);
  $$FileSnapshotsTableTableManager get fileSnapshots =>
      $$FileSnapshotsTableTableManager(_db, _db.fileSnapshots);
}
