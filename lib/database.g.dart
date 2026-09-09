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
    true,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _descriptionMeta = const VerificationMeta(
    'description',
  );
  @override
  late final GeneratedColumn<String> description = GeneratedColumn<String>(
    'description',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _parentIdMeta = const VerificationMeta(
    'parentId',
  );
  @override
  late final GeneratedColumn<int> parentId = GeneratedColumn<int>(
    'parent_id',
    aliasedName,
    true,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'REFERENCES bookmarks (id) ON DELETE CASCADE',
    ),
  );
  static const VerificationMeta _isFolderMeta = const VerificationMeta(
    'isFolder',
  );
  @override
  late final GeneratedColumn<bool> isFolder = GeneratedColumn<bool>(
    'is_folder',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("is_folder" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
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
    title,
    pageIndex,
    description,
    parentId,
    isFolder,
    createdAt,
    updatedAt,
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
    }
    if (data.containsKey('description')) {
      context.handle(
        _descriptionMeta,
        description.isAcceptableOrUnknown(
          data['description']!,
          _descriptionMeta,
        ),
      );
    }
    if (data.containsKey('parent_id')) {
      context.handle(
        _parentIdMeta,
        parentId.isAcceptableOrUnknown(data['parent_id']!, _parentIdMeta),
      );
    }
    if (data.containsKey('is_folder')) {
      context.handle(
        _isFolderMeta,
        isFolder.isAcceptableOrUnknown(data['is_folder']!, _isFolderMeta),
      );
    }
    if (data.containsKey('created_at')) {
      context.handle(
        _createdAtMeta,
        createdAt.isAcceptableOrUnknown(data['created_at']!, _createdAtMeta),
      );
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
      ),
      description: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}description'],
      ),
      parentId: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}parent_id'],
      ),
      isFolder: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}is_folder'],
      )!,
      createdAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}created_at'],
      )!,
      updatedAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}updated_at'],
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
  final int? pageIndex;
  final String? description;
  final int? parentId;
  final bool isFolder;
  final DateTime createdAt;
  final DateTime updatedAt;
  const Bookmark({
    required this.id,
    required this.filePath,
    required this.title,
    this.pageIndex,
    this.description,
    this.parentId,
    required this.isFolder,
    required this.createdAt,
    required this.updatedAt,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['file_path'] = Variable<String>(filePath);
    map['title'] = Variable<String>(title);
    if (!nullToAbsent || pageIndex != null) {
      map['page_index'] = Variable<int>(pageIndex);
    }
    if (!nullToAbsent || description != null) {
      map['description'] = Variable<String>(description);
    }
    if (!nullToAbsent || parentId != null) {
      map['parent_id'] = Variable<int>(parentId);
    }
    map['is_folder'] = Variable<bool>(isFolder);
    map['created_at'] = Variable<DateTime>(createdAt);
    map['updated_at'] = Variable<DateTime>(updatedAt);
    return map;
  }

  BookmarksCompanion toCompanion(bool nullToAbsent) {
    return BookmarksCompanion(
      id: Value(id),
      filePath: Value(filePath),
      title: Value(title),
      pageIndex: pageIndex == null && nullToAbsent
          ? const Value.absent()
          : Value(pageIndex),
      description: description == null && nullToAbsent
          ? const Value.absent()
          : Value(description),
      parentId: parentId == null && nullToAbsent
          ? const Value.absent()
          : Value(parentId),
      isFolder: Value(isFolder),
      createdAt: Value(createdAt),
      updatedAt: Value(updatedAt),
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
      pageIndex: serializer.fromJson<int?>(json['pageIndex']),
      description: serializer.fromJson<String?>(json['description']),
      parentId: serializer.fromJson<int?>(json['parentId']),
      isFolder: serializer.fromJson<bool>(json['isFolder']),
      createdAt: serializer.fromJson<DateTime>(json['createdAt']),
      updatedAt: serializer.fromJson<DateTime>(json['updatedAt']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'filePath': serializer.toJson<String>(filePath),
      'title': serializer.toJson<String>(title),
      'pageIndex': serializer.toJson<int?>(pageIndex),
      'description': serializer.toJson<String?>(description),
      'parentId': serializer.toJson<int?>(parentId),
      'isFolder': serializer.toJson<bool>(isFolder),
      'createdAt': serializer.toJson<DateTime>(createdAt),
      'updatedAt': serializer.toJson<DateTime>(updatedAt),
    };
  }

  Bookmark copyWith({
    int? id,
    String? filePath,
    String? title,
    Value<int?> pageIndex = const Value.absent(),
    Value<String?> description = const Value.absent(),
    Value<int?> parentId = const Value.absent(),
    bool? isFolder,
    DateTime? createdAt,
    DateTime? updatedAt,
  }) => Bookmark(
    id: id ?? this.id,
    filePath: filePath ?? this.filePath,
    title: title ?? this.title,
    pageIndex: pageIndex.present ? pageIndex.value : this.pageIndex,
    description: description.present ? description.value : this.description,
    parentId: parentId.present ? parentId.value : this.parentId,
    isFolder: isFolder ?? this.isFolder,
    createdAt: createdAt ?? this.createdAt,
    updatedAt: updatedAt ?? this.updatedAt,
  );
  Bookmark copyWithCompanion(BookmarksCompanion data) {
    return Bookmark(
      id: data.id.present ? data.id.value : this.id,
      filePath: data.filePath.present ? data.filePath.value : this.filePath,
      title: data.title.present ? data.title.value : this.title,
      pageIndex: data.pageIndex.present ? data.pageIndex.value : this.pageIndex,
      description: data.description.present
          ? data.description.value
          : this.description,
      parentId: data.parentId.present ? data.parentId.value : this.parentId,
      isFolder: data.isFolder.present ? data.isFolder.value : this.isFolder,
      createdAt: data.createdAt.present ? data.createdAt.value : this.createdAt,
      updatedAt: data.updatedAt.present ? data.updatedAt.value : this.updatedAt,
    );
  }

  @override
  String toString() {
    return (StringBuffer('Bookmark(')
          ..write('id: $id, ')
          ..write('filePath: $filePath, ')
          ..write('title: $title, ')
          ..write('pageIndex: $pageIndex, ')
          ..write('description: $description, ')
          ..write('parentId: $parentId, ')
          ..write('isFolder: $isFolder, ')
          ..write('createdAt: $createdAt, ')
          ..write('updatedAt: $updatedAt')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(
    id,
    filePath,
    title,
    pageIndex,
    description,
    parentId,
    isFolder,
    createdAt,
    updatedAt,
  );
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is Bookmark &&
          other.id == this.id &&
          other.filePath == this.filePath &&
          other.title == this.title &&
          other.pageIndex == this.pageIndex &&
          other.description == this.description &&
          other.parentId == this.parentId &&
          other.isFolder == this.isFolder &&
          other.createdAt == this.createdAt &&
          other.updatedAt == this.updatedAt);
}

class BookmarksCompanion extends UpdateCompanion<Bookmark> {
  final Value<int> id;
  final Value<String> filePath;
  final Value<String> title;
  final Value<int?> pageIndex;
  final Value<String?> description;
  final Value<int?> parentId;
  final Value<bool> isFolder;
  final Value<DateTime> createdAt;
  final Value<DateTime> updatedAt;
  const BookmarksCompanion({
    this.id = const Value.absent(),
    this.filePath = const Value.absent(),
    this.title = const Value.absent(),
    this.pageIndex = const Value.absent(),
    this.description = const Value.absent(),
    this.parentId = const Value.absent(),
    this.isFolder = const Value.absent(),
    this.createdAt = const Value.absent(),
    this.updatedAt = const Value.absent(),
  });
  BookmarksCompanion.insert({
    this.id = const Value.absent(),
    required String filePath,
    required String title,
    this.pageIndex = const Value.absent(),
    this.description = const Value.absent(),
    this.parentId = const Value.absent(),
    this.isFolder = const Value.absent(),
    this.createdAt = const Value.absent(),
    this.updatedAt = const Value.absent(),
  }) : filePath = Value(filePath),
       title = Value(title);
  static Insertable<Bookmark> custom({
    Expression<int>? id,
    Expression<String>? filePath,
    Expression<String>? title,
    Expression<int>? pageIndex,
    Expression<String>? description,
    Expression<int>? parentId,
    Expression<bool>? isFolder,
    Expression<DateTime>? createdAt,
    Expression<DateTime>? updatedAt,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (filePath != null) 'file_path': filePath,
      if (title != null) 'title': title,
      if (pageIndex != null) 'page_index': pageIndex,
      if (description != null) 'description': description,
      if (parentId != null) 'parent_id': parentId,
      if (isFolder != null) 'is_folder': isFolder,
      if (createdAt != null) 'created_at': createdAt,
      if (updatedAt != null) 'updated_at': updatedAt,
    });
  }

  BookmarksCompanion copyWith({
    Value<int>? id,
    Value<String>? filePath,
    Value<String>? title,
    Value<int?>? pageIndex,
    Value<String?>? description,
    Value<int?>? parentId,
    Value<bool>? isFolder,
    Value<DateTime>? createdAt,
    Value<DateTime>? updatedAt,
  }) {
    return BookmarksCompanion(
      id: id ?? this.id,
      filePath: filePath ?? this.filePath,
      title: title ?? this.title,
      pageIndex: pageIndex ?? this.pageIndex,
      description: description ?? this.description,
      parentId: parentId ?? this.parentId,
      isFolder: isFolder ?? this.isFolder,
      createdAt: createdAt ?? this.createdAt,
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
    if (title.present) {
      map['title'] = Variable<String>(title.value);
    }
    if (pageIndex.present) {
      map['page_index'] = Variable<int>(pageIndex.value);
    }
    if (description.present) {
      map['description'] = Variable<String>(description.value);
    }
    if (parentId.present) {
      map['parent_id'] = Variable<int>(parentId.value);
    }
    if (isFolder.present) {
      map['is_folder'] = Variable<bool>(isFolder.value);
    }
    if (createdAt.present) {
      map['created_at'] = Variable<DateTime>(createdAt.value);
    }
    if (updatedAt.present) {
      map['updated_at'] = Variable<DateTime>(updatedAt.value);
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
          ..write('description: $description, ')
          ..write('parentId: $parentId, ')
          ..write('isFolder: $isFolder, ')
          ..write('createdAt: $createdAt, ')
          ..write('updatedAt: $updatedAt')
          ..write(')'))
        .toString();
  }
}

class $TagsTable extends Tags with TableInfo<$TagsTable, Tag> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $TagsTable(this.attachedDatabase, [this._alias]);
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
  static const VerificationMeta _nameMeta = const VerificationMeta('name');
  @override
  late final GeneratedColumn<String> name = GeneratedColumn<String>(
    'name',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
    defaultConstraints: GeneratedColumn.constraintIsAlways('UNIQUE'),
  );
  @override
  List<GeneratedColumn> get $columns => [id, name];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'tags';
  @override
  VerificationContext validateIntegrity(
    Insertable<Tag> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    }
    if (data.containsKey('name')) {
      context.handle(
        _nameMeta,
        name.isAcceptableOrUnknown(data['name']!, _nameMeta),
      );
    } else if (isInserting) {
      context.missing(_nameMeta);
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  Tag map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return Tag(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      name: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}name'],
      )!,
    );
  }

  @override
  $TagsTable createAlias(String alias) {
    return $TagsTable(attachedDatabase, alias);
  }
}

class Tag extends DataClass implements Insertable<Tag> {
  final int id;
  final String name;
  const Tag({required this.id, required this.name});
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['name'] = Variable<String>(name);
    return map;
  }

  TagsCompanion toCompanion(bool nullToAbsent) {
    return TagsCompanion(id: Value(id), name: Value(name));
  }

  factory Tag.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return Tag(
      id: serializer.fromJson<int>(json['id']),
      name: serializer.fromJson<String>(json['name']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'name': serializer.toJson<String>(name),
    };
  }

  Tag copyWith({int? id, String? name}) =>
      Tag(id: id ?? this.id, name: name ?? this.name);
  Tag copyWithCompanion(TagsCompanion data) {
    return Tag(
      id: data.id.present ? data.id.value : this.id,
      name: data.name.present ? data.name.value : this.name,
    );
  }

  @override
  String toString() {
    return (StringBuffer('Tag(')
          ..write('id: $id, ')
          ..write('name: $name')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(id, name);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is Tag && other.id == this.id && other.name == this.name);
}

class TagsCompanion extends UpdateCompanion<Tag> {
  final Value<int> id;
  final Value<String> name;
  const TagsCompanion({
    this.id = const Value.absent(),
    this.name = const Value.absent(),
  });
  TagsCompanion.insert({this.id = const Value.absent(), required String name})
    : name = Value(name);
  static Insertable<Tag> custom({
    Expression<int>? id,
    Expression<String>? name,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (name != null) 'name': name,
    });
  }

  TagsCompanion copyWith({Value<int>? id, Value<String>? name}) {
    return TagsCompanion(id: id ?? this.id, name: name ?? this.name);
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (name.present) {
      map['name'] = Variable<String>(name.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('TagsCompanion(')
          ..write('id: $id, ')
          ..write('name: $name')
          ..write(')'))
        .toString();
  }
}

class $BookmarkTagsTable extends BookmarkTags
    with TableInfo<$BookmarkTagsTable, BookmarkTag> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $BookmarkTagsTable(this.attachedDatabase, [this._alias]);
  static const VerificationMeta _bookmarkIdMeta = const VerificationMeta(
    'bookmarkId',
  );
  @override
  late final GeneratedColumn<int> bookmarkId = GeneratedColumn<int>(
    'bookmark_id',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: true,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'REFERENCES bookmarks (id) ON DELETE CASCADE',
    ),
  );
  static const VerificationMeta _tagIdMeta = const VerificationMeta('tagId');
  @override
  late final GeneratedColumn<int> tagId = GeneratedColumn<int>(
    'tag_id',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: true,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'REFERENCES tags (id) ON DELETE CASCADE',
    ),
  );
  @override
  List<GeneratedColumn> get $columns => [bookmarkId, tagId];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'bookmark_tags';
  @override
  VerificationContext validateIntegrity(
    Insertable<BookmarkTag> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('bookmark_id')) {
      context.handle(
        _bookmarkIdMeta,
        bookmarkId.isAcceptableOrUnknown(data['bookmark_id']!, _bookmarkIdMeta),
      );
    } else if (isInserting) {
      context.missing(_bookmarkIdMeta);
    }
    if (data.containsKey('tag_id')) {
      context.handle(
        _tagIdMeta,
        tagId.isAcceptableOrUnknown(data['tag_id']!, _tagIdMeta),
      );
    } else if (isInserting) {
      context.missing(_tagIdMeta);
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => const {};
  @override
  BookmarkTag map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return BookmarkTag(
      bookmarkId: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}bookmark_id'],
      )!,
      tagId: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}tag_id'],
      )!,
    );
  }

  @override
  $BookmarkTagsTable createAlias(String alias) {
    return $BookmarkTagsTable(attachedDatabase, alias);
  }
}

class BookmarkTag extends DataClass implements Insertable<BookmarkTag> {
  final int bookmarkId;
  final int tagId;
  const BookmarkTag({required this.bookmarkId, required this.tagId});
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['bookmark_id'] = Variable<int>(bookmarkId);
    map['tag_id'] = Variable<int>(tagId);
    return map;
  }

  BookmarkTagsCompanion toCompanion(bool nullToAbsent) {
    return BookmarkTagsCompanion(
      bookmarkId: Value(bookmarkId),
      tagId: Value(tagId),
    );
  }

  factory BookmarkTag.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return BookmarkTag(
      bookmarkId: serializer.fromJson<int>(json['bookmarkId']),
      tagId: serializer.fromJson<int>(json['tagId']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'bookmarkId': serializer.toJson<int>(bookmarkId),
      'tagId': serializer.toJson<int>(tagId),
    };
  }

  BookmarkTag copyWith({int? bookmarkId, int? tagId}) => BookmarkTag(
    bookmarkId: bookmarkId ?? this.bookmarkId,
    tagId: tagId ?? this.tagId,
  );
  BookmarkTag copyWithCompanion(BookmarkTagsCompanion data) {
    return BookmarkTag(
      bookmarkId: data.bookmarkId.present
          ? data.bookmarkId.value
          : this.bookmarkId,
      tagId: data.tagId.present ? data.tagId.value : this.tagId,
    );
  }

  @override
  String toString() {
    return (StringBuffer('BookmarkTag(')
          ..write('bookmarkId: $bookmarkId, ')
          ..write('tagId: $tagId')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(bookmarkId, tagId);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is BookmarkTag &&
          other.bookmarkId == this.bookmarkId &&
          other.tagId == this.tagId);
}

class BookmarkTagsCompanion extends UpdateCompanion<BookmarkTag> {
  final Value<int> bookmarkId;
  final Value<int> tagId;
  final Value<int> rowid;
  const BookmarkTagsCompanion({
    this.bookmarkId = const Value.absent(),
    this.tagId = const Value.absent(),
    this.rowid = const Value.absent(),
  });
  BookmarkTagsCompanion.insert({
    required int bookmarkId,
    required int tagId,
    this.rowid = const Value.absent(),
  }) : bookmarkId = Value(bookmarkId),
       tagId = Value(tagId);
  static Insertable<BookmarkTag> custom({
    Expression<int>? bookmarkId,
    Expression<int>? tagId,
    Expression<int>? rowid,
  }) {
    return RawValuesInsertable({
      if (bookmarkId != null) 'bookmark_id': bookmarkId,
      if (tagId != null) 'tag_id': tagId,
      if (rowid != null) 'rowid': rowid,
    });
  }

  BookmarkTagsCompanion copyWith({
    Value<int>? bookmarkId,
    Value<int>? tagId,
    Value<int>? rowid,
  }) {
    return BookmarkTagsCompanion(
      bookmarkId: bookmarkId ?? this.bookmarkId,
      tagId: tagId ?? this.tagId,
      rowid: rowid ?? this.rowid,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (bookmarkId.present) {
      map['bookmark_id'] = Variable<int>(bookmarkId.value);
    }
    if (tagId.present) {
      map['tag_id'] = Variable<int>(tagId.value);
    }
    if (rowid.present) {
      map['rowid'] = Variable<int>(rowid.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('BookmarkTagsCompanion(')
          ..write('bookmarkId: $bookmarkId, ')
          ..write('tagId: $tagId, ')
          ..write('rowid: $rowid')
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

class $LibraryFoldersTable extends LibraryFolders
    with TableInfo<$LibraryFoldersTable, LibraryFolder> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $LibraryFoldersTable(this.attachedDatabase, [this._alias]);
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
  static const VerificationMeta _pathMeta = const VerificationMeta('path');
  @override
  late final GeneratedColumn<String> path = GeneratedColumn<String>(
    'path',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
    defaultConstraints: GeneratedColumn.constraintIsAlways('UNIQUE'),
  );
  static const VerificationMeta _addedAtMeta = const VerificationMeta(
    'addedAt',
  );
  @override
  late final GeneratedColumn<DateTime> addedAt = GeneratedColumn<DateTime>(
    'added_at',
    aliasedName,
    false,
    type: DriftSqlType.dateTime,
    requiredDuringInsert: false,
    defaultValue: currentDateAndTime,
  );
  @override
  List<GeneratedColumn> get $columns => [id, path, addedAt];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'library_folders';
  @override
  VerificationContext validateIntegrity(
    Insertable<LibraryFolder> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    }
    if (data.containsKey('path')) {
      context.handle(
        _pathMeta,
        path.isAcceptableOrUnknown(data['path']!, _pathMeta),
      );
    } else if (isInserting) {
      context.missing(_pathMeta);
    }
    if (data.containsKey('added_at')) {
      context.handle(
        _addedAtMeta,
        addedAt.isAcceptableOrUnknown(data['added_at']!, _addedAtMeta),
      );
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  LibraryFolder map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return LibraryFolder(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      path: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}path'],
      )!,
      addedAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}added_at'],
      )!,
    );
  }

  @override
  $LibraryFoldersTable createAlias(String alias) {
    return $LibraryFoldersTable(attachedDatabase, alias);
  }
}

class LibraryFolder extends DataClass implements Insertable<LibraryFolder> {
  final int id;
  final String path;
  final DateTime addedAt;
  const LibraryFolder({
    required this.id,
    required this.path,
    required this.addedAt,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['path'] = Variable<String>(path);
    map['added_at'] = Variable<DateTime>(addedAt);
    return map;
  }

  LibraryFoldersCompanion toCompanion(bool nullToAbsent) {
    return LibraryFoldersCompanion(
      id: Value(id),
      path: Value(path),
      addedAt: Value(addedAt),
    );
  }

  factory LibraryFolder.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return LibraryFolder(
      id: serializer.fromJson<int>(json['id']),
      path: serializer.fromJson<String>(json['path']),
      addedAt: serializer.fromJson<DateTime>(json['addedAt']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'path': serializer.toJson<String>(path),
      'addedAt': serializer.toJson<DateTime>(addedAt),
    };
  }

  LibraryFolder copyWith({int? id, String? path, DateTime? addedAt}) =>
      LibraryFolder(
        id: id ?? this.id,
        path: path ?? this.path,
        addedAt: addedAt ?? this.addedAt,
      );
  LibraryFolder copyWithCompanion(LibraryFoldersCompanion data) {
    return LibraryFolder(
      id: data.id.present ? data.id.value : this.id,
      path: data.path.present ? data.path.value : this.path,
      addedAt: data.addedAt.present ? data.addedAt.value : this.addedAt,
    );
  }

  @override
  String toString() {
    return (StringBuffer('LibraryFolder(')
          ..write('id: $id, ')
          ..write('path: $path, ')
          ..write('addedAt: $addedAt')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(id, path, addedAt);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is LibraryFolder &&
          other.id == this.id &&
          other.path == this.path &&
          other.addedAt == this.addedAt);
}

class LibraryFoldersCompanion extends UpdateCompanion<LibraryFolder> {
  final Value<int> id;
  final Value<String> path;
  final Value<DateTime> addedAt;
  const LibraryFoldersCompanion({
    this.id = const Value.absent(),
    this.path = const Value.absent(),
    this.addedAt = const Value.absent(),
  });
  LibraryFoldersCompanion.insert({
    this.id = const Value.absent(),
    required String path,
    this.addedAt = const Value.absent(),
  }) : path = Value(path);
  static Insertable<LibraryFolder> custom({
    Expression<int>? id,
    Expression<String>? path,
    Expression<DateTime>? addedAt,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (path != null) 'path': path,
      if (addedAt != null) 'added_at': addedAt,
    });
  }

  LibraryFoldersCompanion copyWith({
    Value<int>? id,
    Value<String>? path,
    Value<DateTime>? addedAt,
  }) {
    return LibraryFoldersCompanion(
      id: id ?? this.id,
      path: path ?? this.path,
      addedAt: addedAt ?? this.addedAt,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (path.present) {
      map['path'] = Variable<String>(path.value);
    }
    if (addedAt.present) {
      map['added_at'] = Variable<DateTime>(addedAt.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('LibraryFoldersCompanion(')
          ..write('id: $id, ')
          ..write('path: $path, ')
          ..write('addedAt: $addedAt')
          ..write(')'))
        .toString();
  }
}

class $LibraryFilesTable extends LibraryFiles
    with TableInfo<$LibraryFilesTable, LibraryFile> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $LibraryFilesTable(this.attachedDatabase, [this._alias]);
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
  static const VerificationMeta _folderIdMeta = const VerificationMeta(
    'folderId',
  );
  @override
  late final GeneratedColumn<int> folderId = GeneratedColumn<int>(
    'folder_id',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: true,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'REFERENCES library_folders (id) ON DELETE CASCADE',
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
    defaultConstraints: GeneratedColumn.constraintIsAlways('UNIQUE'),
  );
  static const VerificationMeta _fileNameMeta = const VerificationMeta(
    'fileName',
  );
  @override
  late final GeneratedColumn<String> fileName = GeneratedColumn<String>(
    'file_name',
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
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _authorMeta = const VerificationMeta('author');
  @override
  late final GeneratedColumn<String> author = GeneratedColumn<String>(
    'author',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _formatMeta = const VerificationMeta('format');
  @override
  late final GeneratedColumn<String> format = GeneratedColumn<String>(
    'format',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('PDF'),
  );
  static const VerificationMeta _bookmarkCountMeta = const VerificationMeta(
    'bookmarkCount',
  );
  @override
  late final GeneratedColumn<int> bookmarkCount = GeneratedColumn<int>(
    'bookmark_count',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultValue: const Constant(0),
  );
  static const VerificationMeta _pageCountMeta = const VerificationMeta(
    'pageCount',
  );
  @override
  late final GeneratedColumn<int> pageCount = GeneratedColumn<int>(
    'page_count',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultValue: const Constant(0),
  );
  static const VerificationMeta _currentPageMeta = const VerificationMeta(
    'currentPage',
  );
  @override
  late final GeneratedColumn<int> currentPage = GeneratedColumn<int>(
    'current_page',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultValue: const Constant(1),
  );
  static const VerificationMeta _fileSizeBytesMeta = const VerificationMeta(
    'fileSizeBytes',
  );
  @override
  late final GeneratedColumn<int> fileSizeBytes = GeneratedColumn<int>(
    'file_size_bytes',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultValue: const Constant(0),
  );
  static const VerificationMeta _isFavoriteMeta = const VerificationMeta(
    'isFavorite',
  );
  @override
  late final GeneratedColumn<bool> isFavorite = GeneratedColumn<bool>(
    'is_favorite',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("is_favorite" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
  );
  static const VerificationMeta _coverPathMeta = const VerificationMeta(
    'coverPath',
  );
  @override
  late final GeneratedColumn<String> coverPath = GeneratedColumn<String>(
    'cover_path',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _lastOpenedMeta = const VerificationMeta(
    'lastOpened',
  );
  @override
  late final GeneratedColumn<DateTime> lastOpened = GeneratedColumn<DateTime>(
    'last_opened',
    aliasedName,
    true,
    type: DriftSqlType.dateTime,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _seriesMeta = const VerificationMeta('series');
  @override
  late final GeneratedColumn<String> series = GeneratedColumn<String>(
    'series',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _tagsMeta = const VerificationMeta('tags');
  @override
  late final GeneratedColumn<String> tags = GeneratedColumn<String>(
    'tags',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _pageOffsetMeta = const VerificationMeta(
    'pageOffset',
  );
  @override
  late final GeneratedColumn<int> pageOffset = GeneratedColumn<int>(
    'page_offset',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultValue: const Constant(0),
  );
  static const VerificationMeta _lastModifiedMeta = const VerificationMeta(
    'lastModified',
  );
  @override
  late final GeneratedColumn<DateTime> lastModified = GeneratedColumn<DateTime>(
    'last_modified',
    aliasedName,
    false,
    type: DriftSqlType.dateTime,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _lastScannedMeta = const VerificationMeta(
    'lastScanned',
  );
  @override
  late final GeneratedColumn<DateTime> lastScanned = GeneratedColumn<DateTime>(
    'last_scanned',
    aliasedName,
    false,
    type: DriftSqlType.dateTime,
    requiredDuringInsert: false,
    defaultValue: currentDateAndTime,
  );
  @override
  List<GeneratedColumn> get $columns => [
    id,
    folderId,
    filePath,
    fileName,
    title,
    author,
    format,
    bookmarkCount,
    pageCount,
    currentPage,
    fileSizeBytes,
    isFavorite,
    coverPath,
    lastOpened,
    series,
    tags,
    pageOffset,
    lastModified,
    lastScanned,
  ];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'library_files';
  @override
  VerificationContext validateIntegrity(
    Insertable<LibraryFile> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    }
    if (data.containsKey('folder_id')) {
      context.handle(
        _folderIdMeta,
        folderId.isAcceptableOrUnknown(data['folder_id']!, _folderIdMeta),
      );
    } else if (isInserting) {
      context.missing(_folderIdMeta);
    }
    if (data.containsKey('file_path')) {
      context.handle(
        _filePathMeta,
        filePath.isAcceptableOrUnknown(data['file_path']!, _filePathMeta),
      );
    } else if (isInserting) {
      context.missing(_filePathMeta);
    }
    if (data.containsKey('file_name')) {
      context.handle(
        _fileNameMeta,
        fileName.isAcceptableOrUnknown(data['file_name']!, _fileNameMeta),
      );
    } else if (isInserting) {
      context.missing(_fileNameMeta);
    }
    if (data.containsKey('title')) {
      context.handle(
        _titleMeta,
        title.isAcceptableOrUnknown(data['title']!, _titleMeta),
      );
    }
    if (data.containsKey('author')) {
      context.handle(
        _authorMeta,
        author.isAcceptableOrUnknown(data['author']!, _authorMeta),
      );
    }
    if (data.containsKey('format')) {
      context.handle(
        _formatMeta,
        format.isAcceptableOrUnknown(data['format']!, _formatMeta),
      );
    }
    if (data.containsKey('bookmark_count')) {
      context.handle(
        _bookmarkCountMeta,
        bookmarkCount.isAcceptableOrUnknown(
          data['bookmark_count']!,
          _bookmarkCountMeta,
        ),
      );
    }
    if (data.containsKey('page_count')) {
      context.handle(
        _pageCountMeta,
        pageCount.isAcceptableOrUnknown(data['page_count']!, _pageCountMeta),
      );
    }
    if (data.containsKey('current_page')) {
      context.handle(
        _currentPageMeta,
        currentPage.isAcceptableOrUnknown(
          data['current_page']!,
          _currentPageMeta,
        ),
      );
    }
    if (data.containsKey('file_size_bytes')) {
      context.handle(
        _fileSizeBytesMeta,
        fileSizeBytes.isAcceptableOrUnknown(
          data['file_size_bytes']!,
          _fileSizeBytesMeta,
        ),
      );
    }
    if (data.containsKey('is_favorite')) {
      context.handle(
        _isFavoriteMeta,
        isFavorite.isAcceptableOrUnknown(data['is_favorite']!, _isFavoriteMeta),
      );
    }
    if (data.containsKey('cover_path')) {
      context.handle(
        _coverPathMeta,
        coverPath.isAcceptableOrUnknown(data['cover_path']!, _coverPathMeta),
      );
    }
    if (data.containsKey('last_opened')) {
      context.handle(
        _lastOpenedMeta,
        lastOpened.isAcceptableOrUnknown(data['last_opened']!, _lastOpenedMeta),
      );
    }
    if (data.containsKey('series')) {
      context.handle(
        _seriesMeta,
        series.isAcceptableOrUnknown(data['series']!, _seriesMeta),
      );
    }
    if (data.containsKey('tags')) {
      context.handle(
        _tagsMeta,
        tags.isAcceptableOrUnknown(data['tags']!, _tagsMeta),
      );
    }
    if (data.containsKey('page_offset')) {
      context.handle(
        _pageOffsetMeta,
        pageOffset.isAcceptableOrUnknown(data['page_offset']!, _pageOffsetMeta),
      );
    }
    if (data.containsKey('last_modified')) {
      context.handle(
        _lastModifiedMeta,
        lastModified.isAcceptableOrUnknown(
          data['last_modified']!,
          _lastModifiedMeta,
        ),
      );
    } else if (isInserting) {
      context.missing(_lastModifiedMeta);
    }
    if (data.containsKey('last_scanned')) {
      context.handle(
        _lastScannedMeta,
        lastScanned.isAcceptableOrUnknown(
          data['last_scanned']!,
          _lastScannedMeta,
        ),
      );
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  LibraryFile map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return LibraryFile(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      folderId: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}folder_id'],
      )!,
      filePath: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}file_path'],
      )!,
      fileName: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}file_name'],
      )!,
      title: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}title'],
      ),
      author: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}author'],
      ),
      format: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}format'],
      )!,
      bookmarkCount: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}bookmark_count'],
      )!,
      pageCount: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}page_count'],
      )!,
      currentPage: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}current_page'],
      )!,
      fileSizeBytes: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}file_size_bytes'],
      )!,
      isFavorite: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}is_favorite'],
      )!,
      coverPath: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}cover_path'],
      ),
      lastOpened: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}last_opened'],
      ),
      series: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}series'],
      ),
      tags: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}tags'],
      ),
      pageOffset: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}page_offset'],
      )!,
      lastModified: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}last_modified'],
      )!,
      lastScanned: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}last_scanned'],
      )!,
    );
  }

  @override
  $LibraryFilesTable createAlias(String alias) {
    return $LibraryFilesTable(attachedDatabase, alias);
  }
}

class LibraryFile extends DataClass implements Insertable<LibraryFile> {
  final int id;
  final int folderId;
  final String filePath;
  final String fileName;
  final String? title;
  final String? author;
  final String format;
  final int bookmarkCount;
  final int pageCount;
  final int currentPage;
  final int fileSizeBytes;
  final bool isFavorite;
  final String? coverPath;
  final DateTime? lastOpened;
  final String? series;
  final String? tags;
  final int pageOffset;
  final DateTime lastModified;
  final DateTime lastScanned;
  const LibraryFile({
    required this.id,
    required this.folderId,
    required this.filePath,
    required this.fileName,
    this.title,
    this.author,
    required this.format,
    required this.bookmarkCount,
    required this.pageCount,
    required this.currentPage,
    required this.fileSizeBytes,
    required this.isFavorite,
    this.coverPath,
    this.lastOpened,
    this.series,
    this.tags,
    required this.pageOffset,
    required this.lastModified,
    required this.lastScanned,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['folder_id'] = Variable<int>(folderId);
    map['file_path'] = Variable<String>(filePath);
    map['file_name'] = Variable<String>(fileName);
    if (!nullToAbsent || title != null) {
      map['title'] = Variable<String>(title);
    }
    if (!nullToAbsent || author != null) {
      map['author'] = Variable<String>(author);
    }
    map['format'] = Variable<String>(format);
    map['bookmark_count'] = Variable<int>(bookmarkCount);
    map['page_count'] = Variable<int>(pageCount);
    map['current_page'] = Variable<int>(currentPage);
    map['file_size_bytes'] = Variable<int>(fileSizeBytes);
    map['is_favorite'] = Variable<bool>(isFavorite);
    if (!nullToAbsent || coverPath != null) {
      map['cover_path'] = Variable<String>(coverPath);
    }
    if (!nullToAbsent || lastOpened != null) {
      map['last_opened'] = Variable<DateTime>(lastOpened);
    }
    if (!nullToAbsent || series != null) {
      map['series'] = Variable<String>(series);
    }
    if (!nullToAbsent || tags != null) {
      map['tags'] = Variable<String>(tags);
    }
    map['page_offset'] = Variable<int>(pageOffset);
    map['last_modified'] = Variable<DateTime>(lastModified);
    map['last_scanned'] = Variable<DateTime>(lastScanned);
    return map;
  }

  LibraryFilesCompanion toCompanion(bool nullToAbsent) {
    return LibraryFilesCompanion(
      id: Value(id),
      folderId: Value(folderId),
      filePath: Value(filePath),
      fileName: Value(fileName),
      title: title == null && nullToAbsent
          ? const Value.absent()
          : Value(title),
      author: author == null && nullToAbsent
          ? const Value.absent()
          : Value(author),
      format: Value(format),
      bookmarkCount: Value(bookmarkCount),
      pageCount: Value(pageCount),
      currentPage: Value(currentPage),
      fileSizeBytes: Value(fileSizeBytes),
      isFavorite: Value(isFavorite),
      coverPath: coverPath == null && nullToAbsent
          ? const Value.absent()
          : Value(coverPath),
      lastOpened: lastOpened == null && nullToAbsent
          ? const Value.absent()
          : Value(lastOpened),
      series: series == null && nullToAbsent
          ? const Value.absent()
          : Value(series),
      tags: tags == null && nullToAbsent ? const Value.absent() : Value(tags),
      pageOffset: Value(pageOffset),
      lastModified: Value(lastModified),
      lastScanned: Value(lastScanned),
    );
  }

  factory LibraryFile.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return LibraryFile(
      id: serializer.fromJson<int>(json['id']),
      folderId: serializer.fromJson<int>(json['folderId']),
      filePath: serializer.fromJson<String>(json['filePath']),
      fileName: serializer.fromJson<String>(json['fileName']),
      title: serializer.fromJson<String?>(json['title']),
      author: serializer.fromJson<String?>(json['author']),
      format: serializer.fromJson<String>(json['format']),
      bookmarkCount: serializer.fromJson<int>(json['bookmarkCount']),
      pageCount: serializer.fromJson<int>(json['pageCount']),
      currentPage: serializer.fromJson<int>(json['currentPage']),
      fileSizeBytes: serializer.fromJson<int>(json['fileSizeBytes']),
      isFavorite: serializer.fromJson<bool>(json['isFavorite']),
      coverPath: serializer.fromJson<String?>(json['coverPath']),
      lastOpened: serializer.fromJson<DateTime?>(json['lastOpened']),
      series: serializer.fromJson<String?>(json['series']),
      tags: serializer.fromJson<String?>(json['tags']),
      pageOffset: serializer.fromJson<int>(json['pageOffset']),
      lastModified: serializer.fromJson<DateTime>(json['lastModified']),
      lastScanned: serializer.fromJson<DateTime>(json['lastScanned']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'folderId': serializer.toJson<int>(folderId),
      'filePath': serializer.toJson<String>(filePath),
      'fileName': serializer.toJson<String>(fileName),
      'title': serializer.toJson<String?>(title),
      'author': serializer.toJson<String?>(author),
      'format': serializer.toJson<String>(format),
      'bookmarkCount': serializer.toJson<int>(bookmarkCount),
      'pageCount': serializer.toJson<int>(pageCount),
      'currentPage': serializer.toJson<int>(currentPage),
      'fileSizeBytes': serializer.toJson<int>(fileSizeBytes),
      'isFavorite': serializer.toJson<bool>(isFavorite),
      'coverPath': serializer.toJson<String?>(coverPath),
      'lastOpened': serializer.toJson<DateTime?>(lastOpened),
      'series': serializer.toJson<String?>(series),
      'tags': serializer.toJson<String?>(tags),
      'pageOffset': serializer.toJson<int>(pageOffset),
      'lastModified': serializer.toJson<DateTime>(lastModified),
      'lastScanned': serializer.toJson<DateTime>(lastScanned),
    };
  }

  LibraryFile copyWith({
    int? id,
    int? folderId,
    String? filePath,
    String? fileName,
    Value<String?> title = const Value.absent(),
    Value<String?> author = const Value.absent(),
    String? format,
    int? bookmarkCount,
    int? pageCount,
    int? currentPage,
    int? fileSizeBytes,
    bool? isFavorite,
    Value<String?> coverPath = const Value.absent(),
    Value<DateTime?> lastOpened = const Value.absent(),
    Value<String?> series = const Value.absent(),
    Value<String?> tags = const Value.absent(),
    int? pageOffset,
    DateTime? lastModified,
    DateTime? lastScanned,
  }) => LibraryFile(
    id: id ?? this.id,
    folderId: folderId ?? this.folderId,
    filePath: filePath ?? this.filePath,
    fileName: fileName ?? this.fileName,
    title: title.present ? title.value : this.title,
    author: author.present ? author.value : this.author,
    format: format ?? this.format,
    bookmarkCount: bookmarkCount ?? this.bookmarkCount,
    pageCount: pageCount ?? this.pageCount,
    currentPage: currentPage ?? this.currentPage,
    fileSizeBytes: fileSizeBytes ?? this.fileSizeBytes,
    isFavorite: isFavorite ?? this.isFavorite,
    coverPath: coverPath.present ? coverPath.value : this.coverPath,
    lastOpened: lastOpened.present ? lastOpened.value : this.lastOpened,
    series: series.present ? series.value : this.series,
    tags: tags.present ? tags.value : this.tags,
    pageOffset: pageOffset ?? this.pageOffset,
    lastModified: lastModified ?? this.lastModified,
    lastScanned: lastScanned ?? this.lastScanned,
  );
  LibraryFile copyWithCompanion(LibraryFilesCompanion data) {
    return LibraryFile(
      id: data.id.present ? data.id.value : this.id,
      folderId: data.folderId.present ? data.folderId.value : this.folderId,
      filePath: data.filePath.present ? data.filePath.value : this.filePath,
      fileName: data.fileName.present ? data.fileName.value : this.fileName,
      title: data.title.present ? data.title.value : this.title,
      author: data.author.present ? data.author.value : this.author,
      format: data.format.present ? data.format.value : this.format,
      bookmarkCount: data.bookmarkCount.present
          ? data.bookmarkCount.value
          : this.bookmarkCount,
      pageCount: data.pageCount.present ? data.pageCount.value : this.pageCount,
      currentPage: data.currentPage.present
          ? data.currentPage.value
          : this.currentPage,
      fileSizeBytes: data.fileSizeBytes.present
          ? data.fileSizeBytes.value
          : this.fileSizeBytes,
      isFavorite: data.isFavorite.present
          ? data.isFavorite.value
          : this.isFavorite,
      coverPath: data.coverPath.present ? data.coverPath.value : this.coverPath,
      lastOpened: data.lastOpened.present
          ? data.lastOpened.value
          : this.lastOpened,
      series: data.series.present ? data.series.value : this.series,
      tags: data.tags.present ? data.tags.value : this.tags,
      pageOffset: data.pageOffset.present
          ? data.pageOffset.value
          : this.pageOffset,
      lastModified: data.lastModified.present
          ? data.lastModified.value
          : this.lastModified,
      lastScanned: data.lastScanned.present
          ? data.lastScanned.value
          : this.lastScanned,
    );
  }

  @override
  String toString() {
    return (StringBuffer('LibraryFile(')
          ..write('id: $id, ')
          ..write('folderId: $folderId, ')
          ..write('filePath: $filePath, ')
          ..write('fileName: $fileName, ')
          ..write('title: $title, ')
          ..write('author: $author, ')
          ..write('format: $format, ')
          ..write('bookmarkCount: $bookmarkCount, ')
          ..write('pageCount: $pageCount, ')
          ..write('currentPage: $currentPage, ')
          ..write('fileSizeBytes: $fileSizeBytes, ')
          ..write('isFavorite: $isFavorite, ')
          ..write('coverPath: $coverPath, ')
          ..write('lastOpened: $lastOpened, ')
          ..write('series: $series, ')
          ..write('tags: $tags, ')
          ..write('pageOffset: $pageOffset, ')
          ..write('lastModified: $lastModified, ')
          ..write('lastScanned: $lastScanned')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(
    id,
    folderId,
    filePath,
    fileName,
    title,
    author,
    format,
    bookmarkCount,
    pageCount,
    currentPage,
    fileSizeBytes,
    isFavorite,
    coverPath,
    lastOpened,
    series,
    tags,
    pageOffset,
    lastModified,
    lastScanned,
  );
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is LibraryFile &&
          other.id == this.id &&
          other.folderId == this.folderId &&
          other.filePath == this.filePath &&
          other.fileName == this.fileName &&
          other.title == this.title &&
          other.author == this.author &&
          other.format == this.format &&
          other.bookmarkCount == this.bookmarkCount &&
          other.pageCount == this.pageCount &&
          other.currentPage == this.currentPage &&
          other.fileSizeBytes == this.fileSizeBytes &&
          other.isFavorite == this.isFavorite &&
          other.coverPath == this.coverPath &&
          other.lastOpened == this.lastOpened &&
          other.series == this.series &&
          other.tags == this.tags &&
          other.pageOffset == this.pageOffset &&
          other.lastModified == this.lastModified &&
          other.lastScanned == this.lastScanned);
}

class LibraryFilesCompanion extends UpdateCompanion<LibraryFile> {
  final Value<int> id;
  final Value<int> folderId;
  final Value<String> filePath;
  final Value<String> fileName;
  final Value<String?> title;
  final Value<String?> author;
  final Value<String> format;
  final Value<int> bookmarkCount;
  final Value<int> pageCount;
  final Value<int> currentPage;
  final Value<int> fileSizeBytes;
  final Value<bool> isFavorite;
  final Value<String?> coverPath;
  final Value<DateTime?> lastOpened;
  final Value<String?> series;
  final Value<String?> tags;
  final Value<int> pageOffset;
  final Value<DateTime> lastModified;
  final Value<DateTime> lastScanned;
  const LibraryFilesCompanion({
    this.id = const Value.absent(),
    this.folderId = const Value.absent(),
    this.filePath = const Value.absent(),
    this.fileName = const Value.absent(),
    this.title = const Value.absent(),
    this.author = const Value.absent(),
    this.format = const Value.absent(),
    this.bookmarkCount = const Value.absent(),
    this.pageCount = const Value.absent(),
    this.currentPage = const Value.absent(),
    this.fileSizeBytes = const Value.absent(),
    this.isFavorite = const Value.absent(),
    this.coverPath = const Value.absent(),
    this.lastOpened = const Value.absent(),
    this.series = const Value.absent(),
    this.tags = const Value.absent(),
    this.pageOffset = const Value.absent(),
    this.lastModified = const Value.absent(),
    this.lastScanned = const Value.absent(),
  });
  LibraryFilesCompanion.insert({
    this.id = const Value.absent(),
    required int folderId,
    required String filePath,
    required String fileName,
    this.title = const Value.absent(),
    this.author = const Value.absent(),
    this.format = const Value.absent(),
    this.bookmarkCount = const Value.absent(),
    this.pageCount = const Value.absent(),
    this.currentPage = const Value.absent(),
    this.fileSizeBytes = const Value.absent(),
    this.isFavorite = const Value.absent(),
    this.coverPath = const Value.absent(),
    this.lastOpened = const Value.absent(),
    this.series = const Value.absent(),
    this.tags = const Value.absent(),
    this.pageOffset = const Value.absent(),
    required DateTime lastModified,
    this.lastScanned = const Value.absent(),
  }) : folderId = Value(folderId),
       filePath = Value(filePath),
       fileName = Value(fileName),
       lastModified = Value(lastModified);
  static Insertable<LibraryFile> custom({
    Expression<int>? id,
    Expression<int>? folderId,
    Expression<String>? filePath,
    Expression<String>? fileName,
    Expression<String>? title,
    Expression<String>? author,
    Expression<String>? format,
    Expression<int>? bookmarkCount,
    Expression<int>? pageCount,
    Expression<int>? currentPage,
    Expression<int>? fileSizeBytes,
    Expression<bool>? isFavorite,
    Expression<String>? coverPath,
    Expression<DateTime>? lastOpened,
    Expression<String>? series,
    Expression<String>? tags,
    Expression<int>? pageOffset,
    Expression<DateTime>? lastModified,
    Expression<DateTime>? lastScanned,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (folderId != null) 'folder_id': folderId,
      if (filePath != null) 'file_path': filePath,
      if (fileName != null) 'file_name': fileName,
      if (title != null) 'title': title,
      if (author != null) 'author': author,
      if (format != null) 'format': format,
      if (bookmarkCount != null) 'bookmark_count': bookmarkCount,
      if (pageCount != null) 'page_count': pageCount,
      if (currentPage != null) 'current_page': currentPage,
      if (fileSizeBytes != null) 'file_size_bytes': fileSizeBytes,
      if (isFavorite != null) 'is_favorite': isFavorite,
      if (coverPath != null) 'cover_path': coverPath,
      if (lastOpened != null) 'last_opened': lastOpened,
      if (series != null) 'series': series,
      if (tags != null) 'tags': tags,
      if (pageOffset != null) 'page_offset': pageOffset,
      if (lastModified != null) 'last_modified': lastModified,
      if (lastScanned != null) 'last_scanned': lastScanned,
    });
  }

  LibraryFilesCompanion copyWith({
    Value<int>? id,
    Value<int>? folderId,
    Value<String>? filePath,
    Value<String>? fileName,
    Value<String?>? title,
    Value<String?>? author,
    Value<String>? format,
    Value<int>? bookmarkCount,
    Value<int>? pageCount,
    Value<int>? currentPage,
    Value<int>? fileSizeBytes,
    Value<bool>? isFavorite,
    Value<String?>? coverPath,
    Value<DateTime?>? lastOpened,
    Value<String?>? series,
    Value<String?>? tags,
    Value<int>? pageOffset,
    Value<DateTime>? lastModified,
    Value<DateTime>? lastScanned,
  }) {
    return LibraryFilesCompanion(
      id: id ?? this.id,
      folderId: folderId ?? this.folderId,
      filePath: filePath ?? this.filePath,
      fileName: fileName ?? this.fileName,
      title: title ?? this.title,
      author: author ?? this.author,
      format: format ?? this.format,
      bookmarkCount: bookmarkCount ?? this.bookmarkCount,
      pageCount: pageCount ?? this.pageCount,
      currentPage: currentPage ?? this.currentPage,
      fileSizeBytes: fileSizeBytes ?? this.fileSizeBytes,
      isFavorite: isFavorite ?? this.isFavorite,
      coverPath: coverPath ?? this.coverPath,
      lastOpened: lastOpened ?? this.lastOpened,
      series: series ?? this.series,
      tags: tags ?? this.tags,
      pageOffset: pageOffset ?? this.pageOffset,
      lastModified: lastModified ?? this.lastModified,
      lastScanned: lastScanned ?? this.lastScanned,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (folderId.present) {
      map['folder_id'] = Variable<int>(folderId.value);
    }
    if (filePath.present) {
      map['file_path'] = Variable<String>(filePath.value);
    }
    if (fileName.present) {
      map['file_name'] = Variable<String>(fileName.value);
    }
    if (title.present) {
      map['title'] = Variable<String>(title.value);
    }
    if (author.present) {
      map['author'] = Variable<String>(author.value);
    }
    if (format.present) {
      map['format'] = Variable<String>(format.value);
    }
    if (bookmarkCount.present) {
      map['bookmark_count'] = Variable<int>(bookmarkCount.value);
    }
    if (pageCount.present) {
      map['page_count'] = Variable<int>(pageCount.value);
    }
    if (currentPage.present) {
      map['current_page'] = Variable<int>(currentPage.value);
    }
    if (fileSizeBytes.present) {
      map['file_size_bytes'] = Variable<int>(fileSizeBytes.value);
    }
    if (isFavorite.present) {
      map['is_favorite'] = Variable<bool>(isFavorite.value);
    }
    if (coverPath.present) {
      map['cover_path'] = Variable<String>(coverPath.value);
    }
    if (lastOpened.present) {
      map['last_opened'] = Variable<DateTime>(lastOpened.value);
    }
    if (series.present) {
      map['series'] = Variable<String>(series.value);
    }
    if (tags.present) {
      map['tags'] = Variable<String>(tags.value);
    }
    if (pageOffset.present) {
      map['page_offset'] = Variable<int>(pageOffset.value);
    }
    if (lastModified.present) {
      map['last_modified'] = Variable<DateTime>(lastModified.value);
    }
    if (lastScanned.present) {
      map['last_scanned'] = Variable<DateTime>(lastScanned.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('LibraryFilesCompanion(')
          ..write('id: $id, ')
          ..write('folderId: $folderId, ')
          ..write('filePath: $filePath, ')
          ..write('fileName: $fileName, ')
          ..write('title: $title, ')
          ..write('author: $author, ')
          ..write('format: $format, ')
          ..write('bookmarkCount: $bookmarkCount, ')
          ..write('pageCount: $pageCount, ')
          ..write('currentPage: $currentPage, ')
          ..write('fileSizeBytes: $fileSizeBytes, ')
          ..write('isFavorite: $isFavorite, ')
          ..write('coverPath: $coverPath, ')
          ..write('lastOpened: $lastOpened, ')
          ..write('series: $series, ')
          ..write('tags: $tags, ')
          ..write('pageOffset: $pageOffset, ')
          ..write('lastModified: $lastModified, ')
          ..write('lastScanned: $lastScanned')
          ..write(')'))
        .toString();
  }
}

class $AnnotationsTable extends Annotations
    with TableInfo<$AnnotationsTable, DocumentAnnotation> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $AnnotationsTable(this.attachedDatabase, [this._alias]);
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
  static const VerificationMeta _pageNumberMeta = const VerificationMeta(
    'pageNumber',
  );
  @override
  late final GeneratedColumn<int> pageNumber = GeneratedColumn<int>(
    'page_number',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _typeMeta = const VerificationMeta('type');
  @override
  late final GeneratedColumn<String> type = GeneratedColumn<String>(
    'type',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _selectedTextMeta = const VerificationMeta(
    'selectedText',
  );
  @override
  late final GeneratedColumn<String> selectedText = GeneratedColumn<String>(
    'selected_text',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _noteMeta = const VerificationMeta('note');
  @override
  late final GeneratedColumn<String> note = GeneratedColumn<String>(
    'note',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _colorHexMeta = const VerificationMeta(
    'colorHex',
  );
  @override
  late final GeneratedColumn<String> colorHex = GeneratedColumn<String>(
    'color_hex',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('#FFE066'),
  );
  static const VerificationMeta _rectXMeta = const VerificationMeta('rectX');
  @override
  late final GeneratedColumn<double> rectX = GeneratedColumn<double>(
    'rect_x',
    aliasedName,
    false,
    type: DriftSqlType.double,
    requiredDuringInsert: false,
    defaultValue: const Constant(0.0),
  );
  static const VerificationMeta _rectYMeta = const VerificationMeta('rectY');
  @override
  late final GeneratedColumn<double> rectY = GeneratedColumn<double>(
    'rect_y',
    aliasedName,
    false,
    type: DriftSqlType.double,
    requiredDuringInsert: false,
    defaultValue: const Constant(0.0),
  );
  static const VerificationMeta _rectWidthMeta = const VerificationMeta(
    'rectWidth',
  );
  @override
  late final GeneratedColumn<double> rectWidth = GeneratedColumn<double>(
    'rect_width',
    aliasedName,
    false,
    type: DriftSqlType.double,
    requiredDuringInsert: false,
    defaultValue: const Constant(0.0),
  );
  static const VerificationMeta _rectHeightMeta = const VerificationMeta(
    'rectHeight',
  );
  @override
  late final GeneratedColumn<double> rectHeight = GeneratedColumn<double>(
    'rect_height',
    aliasedName,
    false,
    type: DriftSqlType.double,
    requiredDuringInsert: false,
    defaultValue: const Constant(0.0),
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
  @override
  List<GeneratedColumn> get $columns => [
    id,
    filePath,
    pageNumber,
    type,
    selectedText,
    note,
    colorHex,
    rectX,
    rectY,
    rectWidth,
    rectHeight,
    createdAt,
  ];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'annotations';
  @override
  VerificationContext validateIntegrity(
    Insertable<DocumentAnnotation> instance, {
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
    if (data.containsKey('page_number')) {
      context.handle(
        _pageNumberMeta,
        pageNumber.isAcceptableOrUnknown(data['page_number']!, _pageNumberMeta),
      );
    } else if (isInserting) {
      context.missing(_pageNumberMeta);
    }
    if (data.containsKey('type')) {
      context.handle(
        _typeMeta,
        type.isAcceptableOrUnknown(data['type']!, _typeMeta),
      );
    } else if (isInserting) {
      context.missing(_typeMeta);
    }
    if (data.containsKey('selected_text')) {
      context.handle(
        _selectedTextMeta,
        selectedText.isAcceptableOrUnknown(
          data['selected_text']!,
          _selectedTextMeta,
        ),
      );
    }
    if (data.containsKey('note')) {
      context.handle(
        _noteMeta,
        note.isAcceptableOrUnknown(data['note']!, _noteMeta),
      );
    }
    if (data.containsKey('color_hex')) {
      context.handle(
        _colorHexMeta,
        colorHex.isAcceptableOrUnknown(data['color_hex']!, _colorHexMeta),
      );
    }
    if (data.containsKey('rect_x')) {
      context.handle(
        _rectXMeta,
        rectX.isAcceptableOrUnknown(data['rect_x']!, _rectXMeta),
      );
    }
    if (data.containsKey('rect_y')) {
      context.handle(
        _rectYMeta,
        rectY.isAcceptableOrUnknown(data['rect_y']!, _rectYMeta),
      );
    }
    if (data.containsKey('rect_width')) {
      context.handle(
        _rectWidthMeta,
        rectWidth.isAcceptableOrUnknown(data['rect_width']!, _rectWidthMeta),
      );
    }
    if (data.containsKey('rect_height')) {
      context.handle(
        _rectHeightMeta,
        rectHeight.isAcceptableOrUnknown(data['rect_height']!, _rectHeightMeta),
      );
    }
    if (data.containsKey('created_at')) {
      context.handle(
        _createdAtMeta,
        createdAt.isAcceptableOrUnknown(data['created_at']!, _createdAtMeta),
      );
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  DocumentAnnotation map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return DocumentAnnotation(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      filePath: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}file_path'],
      )!,
      pageNumber: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}page_number'],
      )!,
      type: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}type'],
      )!,
      selectedText: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}selected_text'],
      ),
      note: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}note'],
      ),
      colorHex: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}color_hex'],
      )!,
      rectX: attachedDatabase.typeMapping.read(
        DriftSqlType.double,
        data['${effectivePrefix}rect_x'],
      )!,
      rectY: attachedDatabase.typeMapping.read(
        DriftSqlType.double,
        data['${effectivePrefix}rect_y'],
      )!,
      rectWidth: attachedDatabase.typeMapping.read(
        DriftSqlType.double,
        data['${effectivePrefix}rect_width'],
      )!,
      rectHeight: attachedDatabase.typeMapping.read(
        DriftSqlType.double,
        data['${effectivePrefix}rect_height'],
      )!,
      createdAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}created_at'],
      )!,
    );
  }

  @override
  $AnnotationsTable createAlias(String alias) {
    return $AnnotationsTable(attachedDatabase, alias);
  }
}

class DocumentAnnotation extends DataClass
    implements Insertable<DocumentAnnotation> {
  final int id;
  final String filePath;
  final int pageNumber;
  final String type;
  final String? selectedText;
  final String? note;
  final String colorHex;
  final double rectX;
  final double rectY;
  final double rectWidth;
  final double rectHeight;
  final DateTime createdAt;
  const DocumentAnnotation({
    required this.id,
    required this.filePath,
    required this.pageNumber,
    required this.type,
    this.selectedText,
    this.note,
    required this.colorHex,
    required this.rectX,
    required this.rectY,
    required this.rectWidth,
    required this.rectHeight,
    required this.createdAt,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['file_path'] = Variable<String>(filePath);
    map['page_number'] = Variable<int>(pageNumber);
    map['type'] = Variable<String>(type);
    if (!nullToAbsent || selectedText != null) {
      map['selected_text'] = Variable<String>(selectedText);
    }
    if (!nullToAbsent || note != null) {
      map['note'] = Variable<String>(note);
    }
    map['color_hex'] = Variable<String>(colorHex);
    map['rect_x'] = Variable<double>(rectX);
    map['rect_y'] = Variable<double>(rectY);
    map['rect_width'] = Variable<double>(rectWidth);
    map['rect_height'] = Variable<double>(rectHeight);
    map['created_at'] = Variable<DateTime>(createdAt);
    return map;
  }

  AnnotationsCompanion toCompanion(bool nullToAbsent) {
    return AnnotationsCompanion(
      id: Value(id),
      filePath: Value(filePath),
      pageNumber: Value(pageNumber),
      type: Value(type),
      selectedText: selectedText == null && nullToAbsent
          ? const Value.absent()
          : Value(selectedText),
      note: note == null && nullToAbsent ? const Value.absent() : Value(note),
      colorHex: Value(colorHex),
      rectX: Value(rectX),
      rectY: Value(rectY),
      rectWidth: Value(rectWidth),
      rectHeight: Value(rectHeight),
      createdAt: Value(createdAt),
    );
  }

  factory DocumentAnnotation.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return DocumentAnnotation(
      id: serializer.fromJson<int>(json['id']),
      filePath: serializer.fromJson<String>(json['filePath']),
      pageNumber: serializer.fromJson<int>(json['pageNumber']),
      type: serializer.fromJson<String>(json['type']),
      selectedText: serializer.fromJson<String?>(json['selectedText']),
      note: serializer.fromJson<String?>(json['note']),
      colorHex: serializer.fromJson<String>(json['colorHex']),
      rectX: serializer.fromJson<double>(json['rectX']),
      rectY: serializer.fromJson<double>(json['rectY']),
      rectWidth: serializer.fromJson<double>(json['rectWidth']),
      rectHeight: serializer.fromJson<double>(json['rectHeight']),
      createdAt: serializer.fromJson<DateTime>(json['createdAt']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'filePath': serializer.toJson<String>(filePath),
      'pageNumber': serializer.toJson<int>(pageNumber),
      'type': serializer.toJson<String>(type),
      'selectedText': serializer.toJson<String?>(selectedText),
      'note': serializer.toJson<String?>(note),
      'colorHex': serializer.toJson<String>(colorHex),
      'rectX': serializer.toJson<double>(rectX),
      'rectY': serializer.toJson<double>(rectY),
      'rectWidth': serializer.toJson<double>(rectWidth),
      'rectHeight': serializer.toJson<double>(rectHeight),
      'createdAt': serializer.toJson<DateTime>(createdAt),
    };
  }

  DocumentAnnotation copyWith({
    int? id,
    String? filePath,
    int? pageNumber,
    String? type,
    Value<String?> selectedText = const Value.absent(),
    Value<String?> note = const Value.absent(),
    String? colorHex,
    double? rectX,
    double? rectY,
    double? rectWidth,
    double? rectHeight,
    DateTime? createdAt,
  }) => DocumentAnnotation(
    id: id ?? this.id,
    filePath: filePath ?? this.filePath,
    pageNumber: pageNumber ?? this.pageNumber,
    type: type ?? this.type,
    selectedText: selectedText.present ? selectedText.value : this.selectedText,
    note: note.present ? note.value : this.note,
    colorHex: colorHex ?? this.colorHex,
    rectX: rectX ?? this.rectX,
    rectY: rectY ?? this.rectY,
    rectWidth: rectWidth ?? this.rectWidth,
    rectHeight: rectHeight ?? this.rectHeight,
    createdAt: createdAt ?? this.createdAt,
  );
  DocumentAnnotation copyWithCompanion(AnnotationsCompanion data) {
    return DocumentAnnotation(
      id: data.id.present ? data.id.value : this.id,
      filePath: data.filePath.present ? data.filePath.value : this.filePath,
      pageNumber: data.pageNumber.present
          ? data.pageNumber.value
          : this.pageNumber,
      type: data.type.present ? data.type.value : this.type,
      selectedText: data.selectedText.present
          ? data.selectedText.value
          : this.selectedText,
      note: data.note.present ? data.note.value : this.note,
      colorHex: data.colorHex.present ? data.colorHex.value : this.colorHex,
      rectX: data.rectX.present ? data.rectX.value : this.rectX,
      rectY: data.rectY.present ? data.rectY.value : this.rectY,
      rectWidth: data.rectWidth.present ? data.rectWidth.value : this.rectWidth,
      rectHeight: data.rectHeight.present
          ? data.rectHeight.value
          : this.rectHeight,
      createdAt: data.createdAt.present ? data.createdAt.value : this.createdAt,
    );
  }

  @override
  String toString() {
    return (StringBuffer('DocumentAnnotation(')
          ..write('id: $id, ')
          ..write('filePath: $filePath, ')
          ..write('pageNumber: $pageNumber, ')
          ..write('type: $type, ')
          ..write('selectedText: $selectedText, ')
          ..write('note: $note, ')
          ..write('colorHex: $colorHex, ')
          ..write('rectX: $rectX, ')
          ..write('rectY: $rectY, ')
          ..write('rectWidth: $rectWidth, ')
          ..write('rectHeight: $rectHeight, ')
          ..write('createdAt: $createdAt')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(
    id,
    filePath,
    pageNumber,
    type,
    selectedText,
    note,
    colorHex,
    rectX,
    rectY,
    rectWidth,
    rectHeight,
    createdAt,
  );
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is DocumentAnnotation &&
          other.id == this.id &&
          other.filePath == this.filePath &&
          other.pageNumber == this.pageNumber &&
          other.type == this.type &&
          other.selectedText == this.selectedText &&
          other.note == this.note &&
          other.colorHex == this.colorHex &&
          other.rectX == this.rectX &&
          other.rectY == this.rectY &&
          other.rectWidth == this.rectWidth &&
          other.rectHeight == this.rectHeight &&
          other.createdAt == this.createdAt);
}

class AnnotationsCompanion extends UpdateCompanion<DocumentAnnotation> {
  final Value<int> id;
  final Value<String> filePath;
  final Value<int> pageNumber;
  final Value<String> type;
  final Value<String?> selectedText;
  final Value<String?> note;
  final Value<String> colorHex;
  final Value<double> rectX;
  final Value<double> rectY;
  final Value<double> rectWidth;
  final Value<double> rectHeight;
  final Value<DateTime> createdAt;
  const AnnotationsCompanion({
    this.id = const Value.absent(),
    this.filePath = const Value.absent(),
    this.pageNumber = const Value.absent(),
    this.type = const Value.absent(),
    this.selectedText = const Value.absent(),
    this.note = const Value.absent(),
    this.colorHex = const Value.absent(),
    this.rectX = const Value.absent(),
    this.rectY = const Value.absent(),
    this.rectWidth = const Value.absent(),
    this.rectHeight = const Value.absent(),
    this.createdAt = const Value.absent(),
  });
  AnnotationsCompanion.insert({
    this.id = const Value.absent(),
    required String filePath,
    required int pageNumber,
    required String type,
    this.selectedText = const Value.absent(),
    this.note = const Value.absent(),
    this.colorHex = const Value.absent(),
    this.rectX = const Value.absent(),
    this.rectY = const Value.absent(),
    this.rectWidth = const Value.absent(),
    this.rectHeight = const Value.absent(),
    this.createdAt = const Value.absent(),
  }) : filePath = Value(filePath),
       pageNumber = Value(pageNumber),
       type = Value(type);
  static Insertable<DocumentAnnotation> custom({
    Expression<int>? id,
    Expression<String>? filePath,
    Expression<int>? pageNumber,
    Expression<String>? type,
    Expression<String>? selectedText,
    Expression<String>? note,
    Expression<String>? colorHex,
    Expression<double>? rectX,
    Expression<double>? rectY,
    Expression<double>? rectWidth,
    Expression<double>? rectHeight,
    Expression<DateTime>? createdAt,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (filePath != null) 'file_path': filePath,
      if (pageNumber != null) 'page_number': pageNumber,
      if (type != null) 'type': type,
      if (selectedText != null) 'selected_text': selectedText,
      if (note != null) 'note': note,
      if (colorHex != null) 'color_hex': colorHex,
      if (rectX != null) 'rect_x': rectX,
      if (rectY != null) 'rect_y': rectY,
      if (rectWidth != null) 'rect_width': rectWidth,
      if (rectHeight != null) 'rect_height': rectHeight,
      if (createdAt != null) 'created_at': createdAt,
    });
  }

  AnnotationsCompanion copyWith({
    Value<int>? id,
    Value<String>? filePath,
    Value<int>? pageNumber,
    Value<String>? type,
    Value<String?>? selectedText,
    Value<String?>? note,
    Value<String>? colorHex,
    Value<double>? rectX,
    Value<double>? rectY,
    Value<double>? rectWidth,
    Value<double>? rectHeight,
    Value<DateTime>? createdAt,
  }) {
    return AnnotationsCompanion(
      id: id ?? this.id,
      filePath: filePath ?? this.filePath,
      pageNumber: pageNumber ?? this.pageNumber,
      type: type ?? this.type,
      selectedText: selectedText ?? this.selectedText,
      note: note ?? this.note,
      colorHex: colorHex ?? this.colorHex,
      rectX: rectX ?? this.rectX,
      rectY: rectY ?? this.rectY,
      rectWidth: rectWidth ?? this.rectWidth,
      rectHeight: rectHeight ?? this.rectHeight,
      createdAt: createdAt ?? this.createdAt,
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
    if (pageNumber.present) {
      map['page_number'] = Variable<int>(pageNumber.value);
    }
    if (type.present) {
      map['type'] = Variable<String>(type.value);
    }
    if (selectedText.present) {
      map['selected_text'] = Variable<String>(selectedText.value);
    }
    if (note.present) {
      map['note'] = Variable<String>(note.value);
    }
    if (colorHex.present) {
      map['color_hex'] = Variable<String>(colorHex.value);
    }
    if (rectX.present) {
      map['rect_x'] = Variable<double>(rectX.value);
    }
    if (rectY.present) {
      map['rect_y'] = Variable<double>(rectY.value);
    }
    if (rectWidth.present) {
      map['rect_width'] = Variable<double>(rectWidth.value);
    }
    if (rectHeight.present) {
      map['rect_height'] = Variable<double>(rectHeight.value);
    }
    if (createdAt.present) {
      map['created_at'] = Variable<DateTime>(createdAt.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('AnnotationsCompanion(')
          ..write('id: $id, ')
          ..write('filePath: $filePath, ')
          ..write('pageNumber: $pageNumber, ')
          ..write('type: $type, ')
          ..write('selectedText: $selectedText, ')
          ..write('note: $note, ')
          ..write('colorHex: $colorHex, ')
          ..write('rectX: $rectX, ')
          ..write('rectY: $rectY, ')
          ..write('rectWidth: $rectWidth, ')
          ..write('rectHeight: $rectHeight, ')
          ..write('createdAt: $createdAt')
          ..write(')'))
        .toString();
  }
}

abstract class _$AppDatabase extends GeneratedDatabase {
  _$AppDatabase(QueryExecutor e) : super(e);
  $AppDatabaseManager get managers => $AppDatabaseManager(this);
  late final $BookmarksTable bookmarks = $BookmarksTable(this);
  late final $TagsTable tags = $TagsTable(this);
  late final $BookmarkTagsTable bookmarkTags = $BookmarkTagsTable(this);
  late final $FileSnapshotsTable fileSnapshots = $FileSnapshotsTable(this);
  late final $LibraryFoldersTable libraryFolders = $LibraryFoldersTable(this);
  late final $LibraryFilesTable libraryFiles = $LibraryFilesTable(this);
  late final $AnnotationsTable annotations = $AnnotationsTable(this);
  @override
  Iterable<TableInfo<Table, Object?>> get allTables =>
      allSchemaEntities.whereType<TableInfo<Table, Object?>>();
  @override
  List<DatabaseSchemaEntity> get allSchemaEntities => [
    bookmarks,
    tags,
    bookmarkTags,
    fileSnapshots,
    libraryFolders,
    libraryFiles,
    annotations,
  ];
  @override
  StreamQueryUpdateRules get streamUpdateRules => const StreamQueryUpdateRules([
    WritePropagation(
      on: TableUpdateQuery.onTableName(
        'bookmarks',
        limitUpdateKind: UpdateKind.delete,
      ),
      result: [TableUpdate('bookmarks', kind: UpdateKind.delete)],
    ),
    WritePropagation(
      on: TableUpdateQuery.onTableName(
        'bookmarks',
        limitUpdateKind: UpdateKind.delete,
      ),
      result: [TableUpdate('bookmark_tags', kind: UpdateKind.delete)],
    ),
    WritePropagation(
      on: TableUpdateQuery.onTableName(
        'tags',
        limitUpdateKind: UpdateKind.delete,
      ),
      result: [TableUpdate('bookmark_tags', kind: UpdateKind.delete)],
    ),
    WritePropagation(
      on: TableUpdateQuery.onTableName(
        'library_folders',
        limitUpdateKind: UpdateKind.delete,
      ),
      result: [TableUpdate('library_files', kind: UpdateKind.delete)],
    ),
  ]);
}

typedef $$BookmarksTableCreateCompanionBuilder =
    BookmarksCompanion Function({
      Value<int> id,
      required String filePath,
      required String title,
      Value<int?> pageIndex,
      Value<String?> description,
      Value<int?> parentId,
      Value<bool> isFolder,
      Value<DateTime> createdAt,
      Value<DateTime> updatedAt,
    });
typedef $$BookmarksTableUpdateCompanionBuilder =
    BookmarksCompanion Function({
      Value<int> id,
      Value<String> filePath,
      Value<String> title,
      Value<int?> pageIndex,
      Value<String?> description,
      Value<int?> parentId,
      Value<bool> isFolder,
      Value<DateTime> createdAt,
      Value<DateTime> updatedAt,
    });

final class $$BookmarksTableReferences
    extends BaseReferences<_$AppDatabase, $BookmarksTable, Bookmark> {
  $$BookmarksTableReferences(super.$_db, super.$_table, super.$_typedResult);

  static $BookmarksTable _parentIdTable(_$AppDatabase db) =>
      db.bookmarks.createAlias(
        $_aliasNameGenerator(db.bookmarks.parentId, db.bookmarks.id),
      );

  $$BookmarksTableProcessedTableManager? get parentId {
    final $_column = $_itemColumn<int>('parent_id');
    if ($_column == null) return null;
    final manager = $$BookmarksTableTableManager(
      $_db,
      $_db.bookmarks,
    ).filter((f) => f.id.sqlEquals($_column));
    final item = $_typedResult.readTableOrNull(_parentIdTable($_db));
    if (item == null) return manager;
    return ProcessedTableManager(
      manager.$state.copyWith(prefetchedData: [item]),
    );
  }

  static MultiTypedResultKey<$BookmarkTagsTable, List<BookmarkTag>>
  _bookmarkTagsRefsTable(_$AppDatabase db) => MultiTypedResultKey.fromTable(
    db.bookmarkTags,
    aliasName: $_aliasNameGenerator(
      db.bookmarks.id,
      db.bookmarkTags.bookmarkId,
    ),
  );

  $$BookmarkTagsTableProcessedTableManager get bookmarkTagsRefs {
    final manager = $$BookmarkTagsTableTableManager(
      $_db,
      $_db.bookmarkTags,
    ).filter((f) => f.bookmarkId.id.sqlEquals($_itemColumn<int>('id')!));

    final cache = $_typedResult.readTableOrNull(_bookmarkTagsRefsTable($_db));
    return ProcessedTableManager(
      manager.$state.copyWith(prefetchedData: cache),
    );
  }
}

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

  ColumnFilters<String> get description => $composableBuilder(
    column: $table.description,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get isFolder => $composableBuilder(
    column: $table.isFolder,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get createdAt => $composableBuilder(
    column: $table.createdAt,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get updatedAt => $composableBuilder(
    column: $table.updatedAt,
    builder: (column) => ColumnFilters(column),
  );

  $$BookmarksTableFilterComposer get parentId {
    final $$BookmarksTableFilterComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.parentId,
      referencedTable: $db.bookmarks,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarksTableFilterComposer(
            $db: $db,
            $table: $db.bookmarks,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }

  Expression<bool> bookmarkTagsRefs(
    Expression<bool> Function($$BookmarkTagsTableFilterComposer f) f,
  ) {
    final $$BookmarkTagsTableFilterComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.id,
      referencedTable: $db.bookmarkTags,
      getReferencedColumn: (t) => t.bookmarkId,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarkTagsTableFilterComposer(
            $db: $db,
            $table: $db.bookmarkTags,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return f(composer);
  }
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

  ColumnOrderings<String> get description => $composableBuilder(
    column: $table.description,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get isFolder => $composableBuilder(
    column: $table.isFolder,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get createdAt => $composableBuilder(
    column: $table.createdAt,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get updatedAt => $composableBuilder(
    column: $table.updatedAt,
    builder: (column) => ColumnOrderings(column),
  );

  $$BookmarksTableOrderingComposer get parentId {
    final $$BookmarksTableOrderingComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.parentId,
      referencedTable: $db.bookmarks,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarksTableOrderingComposer(
            $db: $db,
            $table: $db.bookmarks,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
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

  GeneratedColumn<String> get description => $composableBuilder(
    column: $table.description,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get isFolder =>
      $composableBuilder(column: $table.isFolder, builder: (column) => column);

  GeneratedColumn<DateTime> get createdAt =>
      $composableBuilder(column: $table.createdAt, builder: (column) => column);

  GeneratedColumn<DateTime> get updatedAt =>
      $composableBuilder(column: $table.updatedAt, builder: (column) => column);

  $$BookmarksTableAnnotationComposer get parentId {
    final $$BookmarksTableAnnotationComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.parentId,
      referencedTable: $db.bookmarks,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarksTableAnnotationComposer(
            $db: $db,
            $table: $db.bookmarks,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }

  Expression<T> bookmarkTagsRefs<T extends Object>(
    Expression<T> Function($$BookmarkTagsTableAnnotationComposer a) f,
  ) {
    final $$BookmarkTagsTableAnnotationComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.id,
      referencedTable: $db.bookmarkTags,
      getReferencedColumn: (t) => t.bookmarkId,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarkTagsTableAnnotationComposer(
            $db: $db,
            $table: $db.bookmarkTags,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return f(composer);
  }
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
          (Bookmark, $$BookmarksTableReferences),
          Bookmark,
          PrefetchHooks Function({bool parentId, bool bookmarkTagsRefs})
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
                Value<int?> pageIndex = const Value.absent(),
                Value<String?> description = const Value.absent(),
                Value<int?> parentId = const Value.absent(),
                Value<bool> isFolder = const Value.absent(),
                Value<DateTime> createdAt = const Value.absent(),
                Value<DateTime> updatedAt = const Value.absent(),
              }) => BookmarksCompanion(
                id: id,
                filePath: filePath,
                title: title,
                pageIndex: pageIndex,
                description: description,
                parentId: parentId,
                isFolder: isFolder,
                createdAt: createdAt,
                updatedAt: updatedAt,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                required String filePath,
                required String title,
                Value<int?> pageIndex = const Value.absent(),
                Value<String?> description = const Value.absent(),
                Value<int?> parentId = const Value.absent(),
                Value<bool> isFolder = const Value.absent(),
                Value<DateTime> createdAt = const Value.absent(),
                Value<DateTime> updatedAt = const Value.absent(),
              }) => BookmarksCompanion.insert(
                id: id,
                filePath: filePath,
                title: title,
                pageIndex: pageIndex,
                description: description,
                parentId: parentId,
                isFolder: isFolder,
                createdAt: createdAt,
                updatedAt: updatedAt,
              ),
          withReferenceMapper: (p0) => p0
              .map(
                (e) => (
                  e.readTable(table),
                  $$BookmarksTableReferences(db, table, e),
                ),
              )
              .toList(),
          prefetchHooksCallback:
              ({parentId = false, bookmarkTagsRefs = false}) {
                return PrefetchHooks(
                  db: db,
                  explicitlyWatchedTables: [
                    if (bookmarkTagsRefs) db.bookmarkTags,
                  ],
                  addJoins:
                      <
                        T extends TableManagerState<
                          dynamic,
                          dynamic,
                          dynamic,
                          dynamic,
                          dynamic,
                          dynamic,
                          dynamic,
                          dynamic,
                          dynamic,
                          dynamic,
                          dynamic
                        >
                      >(state) {
                        if (parentId) {
                          state =
                              state.withJoin(
                                    currentTable: table,
                                    currentColumn: table.parentId,
                                    referencedTable: $$BookmarksTableReferences
                                        ._parentIdTable(db),
                                    referencedColumn: $$BookmarksTableReferences
                                        ._parentIdTable(db)
                                        .id,
                                  )
                                  as T;
                        }

                        return state;
                      },
                  getPrefetchedDataCallback: (items) async {
                    return [
                      if (bookmarkTagsRefs)
                        await $_getPrefetchedData<
                          Bookmark,
                          $BookmarksTable,
                          BookmarkTag
                        >(
                          currentTable: table,
                          referencedTable: $$BookmarksTableReferences
                              ._bookmarkTagsRefsTable(db),
                          managerFromTypedResult: (p0) =>
                              $$BookmarksTableReferences(
                                db,
                                table,
                                p0,
                              ).bookmarkTagsRefs,
                          referencedItemsForCurrentItem:
                              (item, referencedItems) => referencedItems.where(
                                (e) => e.bookmarkId == item.id,
                              ),
                          typedResults: items,
                        ),
                    ];
                  },
                );
              },
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
      (Bookmark, $$BookmarksTableReferences),
      Bookmark,
      PrefetchHooks Function({bool parentId, bool bookmarkTagsRefs})
    >;
typedef $$TagsTableCreateCompanionBuilder =
    TagsCompanion Function({Value<int> id, required String name});
typedef $$TagsTableUpdateCompanionBuilder =
    TagsCompanion Function({Value<int> id, Value<String> name});

final class $$TagsTableReferences
    extends BaseReferences<_$AppDatabase, $TagsTable, Tag> {
  $$TagsTableReferences(super.$_db, super.$_table, super.$_typedResult);

  static MultiTypedResultKey<$BookmarkTagsTable, List<BookmarkTag>>
  _bookmarkTagsRefsTable(_$AppDatabase db) => MultiTypedResultKey.fromTable(
    db.bookmarkTags,
    aliasName: $_aliasNameGenerator(db.tags.id, db.bookmarkTags.tagId),
  );

  $$BookmarkTagsTableProcessedTableManager get bookmarkTagsRefs {
    final manager = $$BookmarkTagsTableTableManager(
      $_db,
      $_db.bookmarkTags,
    ).filter((f) => f.tagId.id.sqlEquals($_itemColumn<int>('id')!));

    final cache = $_typedResult.readTableOrNull(_bookmarkTagsRefsTable($_db));
    return ProcessedTableManager(
      manager.$state.copyWith(prefetchedData: cache),
    );
  }
}

class $$TagsTableFilterComposer extends Composer<_$AppDatabase, $TagsTable> {
  $$TagsTableFilterComposer({
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

  ColumnFilters<String> get name => $composableBuilder(
    column: $table.name,
    builder: (column) => ColumnFilters(column),
  );

  Expression<bool> bookmarkTagsRefs(
    Expression<bool> Function($$BookmarkTagsTableFilterComposer f) f,
  ) {
    final $$BookmarkTagsTableFilterComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.id,
      referencedTable: $db.bookmarkTags,
      getReferencedColumn: (t) => t.tagId,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarkTagsTableFilterComposer(
            $db: $db,
            $table: $db.bookmarkTags,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return f(composer);
  }
}

class $$TagsTableOrderingComposer extends Composer<_$AppDatabase, $TagsTable> {
  $$TagsTableOrderingComposer({
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

  ColumnOrderings<String> get name => $composableBuilder(
    column: $table.name,
    builder: (column) => ColumnOrderings(column),
  );
}

class $$TagsTableAnnotationComposer
    extends Composer<_$AppDatabase, $TagsTable> {
  $$TagsTableAnnotationComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  GeneratedColumn<int> get id =>
      $composableBuilder(column: $table.id, builder: (column) => column);

  GeneratedColumn<String> get name =>
      $composableBuilder(column: $table.name, builder: (column) => column);

  Expression<T> bookmarkTagsRefs<T extends Object>(
    Expression<T> Function($$BookmarkTagsTableAnnotationComposer a) f,
  ) {
    final $$BookmarkTagsTableAnnotationComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.id,
      referencedTable: $db.bookmarkTags,
      getReferencedColumn: (t) => t.tagId,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarkTagsTableAnnotationComposer(
            $db: $db,
            $table: $db.bookmarkTags,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return f(composer);
  }
}

class $$TagsTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $TagsTable,
          Tag,
          $$TagsTableFilterComposer,
          $$TagsTableOrderingComposer,
          $$TagsTableAnnotationComposer,
          $$TagsTableCreateCompanionBuilder,
          $$TagsTableUpdateCompanionBuilder,
          (Tag, $$TagsTableReferences),
          Tag,
          PrefetchHooks Function({bool bookmarkTagsRefs})
        > {
  $$TagsTableTableManager(_$AppDatabase db, $TagsTable table)
    : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$TagsTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$TagsTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$TagsTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<String> name = const Value.absent(),
              }) => TagsCompanion(id: id, name: name),
          createCompanionCallback:
              ({Value<int> id = const Value.absent(), required String name}) =>
                  TagsCompanion.insert(id: id, name: name),
          withReferenceMapper: (p0) => p0
              .map(
                (e) =>
                    (e.readTable(table), $$TagsTableReferences(db, table, e)),
              )
              .toList(),
          prefetchHooksCallback: ({bookmarkTagsRefs = false}) {
            return PrefetchHooks(
              db: db,
              explicitlyWatchedTables: [if (bookmarkTagsRefs) db.bookmarkTags],
              addJoins: null,
              getPrefetchedDataCallback: (items) async {
                return [
                  if (bookmarkTagsRefs)
                    await $_getPrefetchedData<Tag, $TagsTable, BookmarkTag>(
                      currentTable: table,
                      referencedTable: $$TagsTableReferences
                          ._bookmarkTagsRefsTable(db),
                      managerFromTypedResult: (p0) =>
                          $$TagsTableReferences(db, table, p0).bookmarkTagsRefs,
                      referencedItemsForCurrentItem: (item, referencedItems) =>
                          referencedItems.where((e) => e.tagId == item.id),
                      typedResults: items,
                    ),
                ];
              },
            );
          },
        ),
      );
}

typedef $$TagsTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $TagsTable,
      Tag,
      $$TagsTableFilterComposer,
      $$TagsTableOrderingComposer,
      $$TagsTableAnnotationComposer,
      $$TagsTableCreateCompanionBuilder,
      $$TagsTableUpdateCompanionBuilder,
      (Tag, $$TagsTableReferences),
      Tag,
      PrefetchHooks Function({bool bookmarkTagsRefs})
    >;
typedef $$BookmarkTagsTableCreateCompanionBuilder =
    BookmarkTagsCompanion Function({
      required int bookmarkId,
      required int tagId,
      Value<int> rowid,
    });
typedef $$BookmarkTagsTableUpdateCompanionBuilder =
    BookmarkTagsCompanion Function({
      Value<int> bookmarkId,
      Value<int> tagId,
      Value<int> rowid,
    });

final class $$BookmarkTagsTableReferences
    extends BaseReferences<_$AppDatabase, $BookmarkTagsTable, BookmarkTag> {
  $$BookmarkTagsTableReferences(super.$_db, super.$_table, super.$_typedResult);

  static $BookmarksTable _bookmarkIdTable(_$AppDatabase db) =>
      db.bookmarks.createAlias(
        $_aliasNameGenerator(db.bookmarkTags.bookmarkId, db.bookmarks.id),
      );

  $$BookmarksTableProcessedTableManager get bookmarkId {
    final $_column = $_itemColumn<int>('bookmark_id')!;

    final manager = $$BookmarksTableTableManager(
      $_db,
      $_db.bookmarks,
    ).filter((f) => f.id.sqlEquals($_column));
    final item = $_typedResult.readTableOrNull(_bookmarkIdTable($_db));
    if (item == null) return manager;
    return ProcessedTableManager(
      manager.$state.copyWith(prefetchedData: [item]),
    );
  }

  static $TagsTable _tagIdTable(_$AppDatabase db) => db.tags.createAlias(
    $_aliasNameGenerator(db.bookmarkTags.tagId, db.tags.id),
  );

  $$TagsTableProcessedTableManager get tagId {
    final $_column = $_itemColumn<int>('tag_id')!;

    final manager = $$TagsTableTableManager(
      $_db,
      $_db.tags,
    ).filter((f) => f.id.sqlEquals($_column));
    final item = $_typedResult.readTableOrNull(_tagIdTable($_db));
    if (item == null) return manager;
    return ProcessedTableManager(
      manager.$state.copyWith(prefetchedData: [item]),
    );
  }
}

class $$BookmarkTagsTableFilterComposer
    extends Composer<_$AppDatabase, $BookmarkTagsTable> {
  $$BookmarkTagsTableFilterComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  $$BookmarksTableFilterComposer get bookmarkId {
    final $$BookmarksTableFilterComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.bookmarkId,
      referencedTable: $db.bookmarks,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarksTableFilterComposer(
            $db: $db,
            $table: $db.bookmarks,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }

  $$TagsTableFilterComposer get tagId {
    final $$TagsTableFilterComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.tagId,
      referencedTable: $db.tags,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$TagsTableFilterComposer(
            $db: $db,
            $table: $db.tags,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
}

class $$BookmarkTagsTableOrderingComposer
    extends Composer<_$AppDatabase, $BookmarkTagsTable> {
  $$BookmarkTagsTableOrderingComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  $$BookmarksTableOrderingComposer get bookmarkId {
    final $$BookmarksTableOrderingComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.bookmarkId,
      referencedTable: $db.bookmarks,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarksTableOrderingComposer(
            $db: $db,
            $table: $db.bookmarks,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }

  $$TagsTableOrderingComposer get tagId {
    final $$TagsTableOrderingComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.tagId,
      referencedTable: $db.tags,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$TagsTableOrderingComposer(
            $db: $db,
            $table: $db.tags,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
}

class $$BookmarkTagsTableAnnotationComposer
    extends Composer<_$AppDatabase, $BookmarkTagsTable> {
  $$BookmarkTagsTableAnnotationComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  $$BookmarksTableAnnotationComposer get bookmarkId {
    final $$BookmarksTableAnnotationComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.bookmarkId,
      referencedTable: $db.bookmarks,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$BookmarksTableAnnotationComposer(
            $db: $db,
            $table: $db.bookmarks,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }

  $$TagsTableAnnotationComposer get tagId {
    final $$TagsTableAnnotationComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.tagId,
      referencedTable: $db.tags,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$TagsTableAnnotationComposer(
            $db: $db,
            $table: $db.tags,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
}

class $$BookmarkTagsTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $BookmarkTagsTable,
          BookmarkTag,
          $$BookmarkTagsTableFilterComposer,
          $$BookmarkTagsTableOrderingComposer,
          $$BookmarkTagsTableAnnotationComposer,
          $$BookmarkTagsTableCreateCompanionBuilder,
          $$BookmarkTagsTableUpdateCompanionBuilder,
          (BookmarkTag, $$BookmarkTagsTableReferences),
          BookmarkTag,
          PrefetchHooks Function({bool bookmarkId, bool tagId})
        > {
  $$BookmarkTagsTableTableManager(_$AppDatabase db, $BookmarkTagsTable table)
    : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$BookmarkTagsTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$BookmarkTagsTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$BookmarkTagsTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> bookmarkId = const Value.absent(),
                Value<int> tagId = const Value.absent(),
                Value<int> rowid = const Value.absent(),
              }) => BookmarkTagsCompanion(
                bookmarkId: bookmarkId,
                tagId: tagId,
                rowid: rowid,
              ),
          createCompanionCallback:
              ({
                required int bookmarkId,
                required int tagId,
                Value<int> rowid = const Value.absent(),
              }) => BookmarkTagsCompanion.insert(
                bookmarkId: bookmarkId,
                tagId: tagId,
                rowid: rowid,
              ),
          withReferenceMapper: (p0) => p0
              .map(
                (e) => (
                  e.readTable(table),
                  $$BookmarkTagsTableReferences(db, table, e),
                ),
              )
              .toList(),
          prefetchHooksCallback: ({bookmarkId = false, tagId = false}) {
            return PrefetchHooks(
              db: db,
              explicitlyWatchedTables: [],
              addJoins:
                  <
                    T extends TableManagerState<
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic
                    >
                  >(state) {
                    if (bookmarkId) {
                      state =
                          state.withJoin(
                                currentTable: table,
                                currentColumn: table.bookmarkId,
                                referencedTable: $$BookmarkTagsTableReferences
                                    ._bookmarkIdTable(db),
                                referencedColumn: $$BookmarkTagsTableReferences
                                    ._bookmarkIdTable(db)
                                    .id,
                              )
                              as T;
                    }
                    if (tagId) {
                      state =
                          state.withJoin(
                                currentTable: table,
                                currentColumn: table.tagId,
                                referencedTable: $$BookmarkTagsTableReferences
                                    ._tagIdTable(db),
                                referencedColumn: $$BookmarkTagsTableReferences
                                    ._tagIdTable(db)
                                    .id,
                              )
                              as T;
                    }

                    return state;
                  },
              getPrefetchedDataCallback: (items) async {
                return [];
              },
            );
          },
        ),
      );
}

typedef $$BookmarkTagsTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $BookmarkTagsTable,
      BookmarkTag,
      $$BookmarkTagsTableFilterComposer,
      $$BookmarkTagsTableOrderingComposer,
      $$BookmarkTagsTableAnnotationComposer,
      $$BookmarkTagsTableCreateCompanionBuilder,
      $$BookmarkTagsTableUpdateCompanionBuilder,
      (BookmarkTag, $$BookmarkTagsTableReferences),
      BookmarkTag,
      PrefetchHooks Function({bool bookmarkId, bool tagId})
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
typedef $$LibraryFoldersTableCreateCompanionBuilder =
    LibraryFoldersCompanion Function({
      Value<int> id,
      required String path,
      Value<DateTime> addedAt,
    });
typedef $$LibraryFoldersTableUpdateCompanionBuilder =
    LibraryFoldersCompanion Function({
      Value<int> id,
      Value<String> path,
      Value<DateTime> addedAt,
    });

final class $$LibraryFoldersTableReferences
    extends BaseReferences<_$AppDatabase, $LibraryFoldersTable, LibraryFolder> {
  $$LibraryFoldersTableReferences(
    super.$_db,
    super.$_table,
    super.$_typedResult,
  );

  static MultiTypedResultKey<$LibraryFilesTable, List<LibraryFile>>
  _libraryFilesRefsTable(_$AppDatabase db) => MultiTypedResultKey.fromTable(
    db.libraryFiles,
    aliasName: $_aliasNameGenerator(
      db.libraryFolders.id,
      db.libraryFiles.folderId,
    ),
  );

  $$LibraryFilesTableProcessedTableManager get libraryFilesRefs {
    final manager = $$LibraryFilesTableTableManager(
      $_db,
      $_db.libraryFiles,
    ).filter((f) => f.folderId.id.sqlEquals($_itemColumn<int>('id')!));

    final cache = $_typedResult.readTableOrNull(_libraryFilesRefsTable($_db));
    return ProcessedTableManager(
      manager.$state.copyWith(prefetchedData: cache),
    );
  }
}

class $$LibraryFoldersTableFilterComposer
    extends Composer<_$AppDatabase, $LibraryFoldersTable> {
  $$LibraryFoldersTableFilterComposer({
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

  ColumnFilters<String> get path => $composableBuilder(
    column: $table.path,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get addedAt => $composableBuilder(
    column: $table.addedAt,
    builder: (column) => ColumnFilters(column),
  );

  Expression<bool> libraryFilesRefs(
    Expression<bool> Function($$LibraryFilesTableFilterComposer f) f,
  ) {
    final $$LibraryFilesTableFilterComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.id,
      referencedTable: $db.libraryFiles,
      getReferencedColumn: (t) => t.folderId,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$LibraryFilesTableFilterComposer(
            $db: $db,
            $table: $db.libraryFiles,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return f(composer);
  }
}

class $$LibraryFoldersTableOrderingComposer
    extends Composer<_$AppDatabase, $LibraryFoldersTable> {
  $$LibraryFoldersTableOrderingComposer({
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

  ColumnOrderings<String> get path => $composableBuilder(
    column: $table.path,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get addedAt => $composableBuilder(
    column: $table.addedAt,
    builder: (column) => ColumnOrderings(column),
  );
}

class $$LibraryFoldersTableAnnotationComposer
    extends Composer<_$AppDatabase, $LibraryFoldersTable> {
  $$LibraryFoldersTableAnnotationComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  GeneratedColumn<int> get id =>
      $composableBuilder(column: $table.id, builder: (column) => column);

  GeneratedColumn<String> get path =>
      $composableBuilder(column: $table.path, builder: (column) => column);

  GeneratedColumn<DateTime> get addedAt =>
      $composableBuilder(column: $table.addedAt, builder: (column) => column);

  Expression<T> libraryFilesRefs<T extends Object>(
    Expression<T> Function($$LibraryFilesTableAnnotationComposer a) f,
  ) {
    final $$LibraryFilesTableAnnotationComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.id,
      referencedTable: $db.libraryFiles,
      getReferencedColumn: (t) => t.folderId,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$LibraryFilesTableAnnotationComposer(
            $db: $db,
            $table: $db.libraryFiles,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return f(composer);
  }
}

class $$LibraryFoldersTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $LibraryFoldersTable,
          LibraryFolder,
          $$LibraryFoldersTableFilterComposer,
          $$LibraryFoldersTableOrderingComposer,
          $$LibraryFoldersTableAnnotationComposer,
          $$LibraryFoldersTableCreateCompanionBuilder,
          $$LibraryFoldersTableUpdateCompanionBuilder,
          (LibraryFolder, $$LibraryFoldersTableReferences),
          LibraryFolder,
          PrefetchHooks Function({bool libraryFilesRefs})
        > {
  $$LibraryFoldersTableTableManager(
    _$AppDatabase db,
    $LibraryFoldersTable table,
  ) : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$LibraryFoldersTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$LibraryFoldersTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$LibraryFoldersTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<String> path = const Value.absent(),
                Value<DateTime> addedAt = const Value.absent(),
              }) =>
                  LibraryFoldersCompanion(id: id, path: path, addedAt: addedAt),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                required String path,
                Value<DateTime> addedAt = const Value.absent(),
              }) => LibraryFoldersCompanion.insert(
                id: id,
                path: path,
                addedAt: addedAt,
              ),
          withReferenceMapper: (p0) => p0
              .map(
                (e) => (
                  e.readTable(table),
                  $$LibraryFoldersTableReferences(db, table, e),
                ),
              )
              .toList(),
          prefetchHooksCallback: ({libraryFilesRefs = false}) {
            return PrefetchHooks(
              db: db,
              explicitlyWatchedTables: [if (libraryFilesRefs) db.libraryFiles],
              addJoins: null,
              getPrefetchedDataCallback: (items) async {
                return [
                  if (libraryFilesRefs)
                    await $_getPrefetchedData<
                      LibraryFolder,
                      $LibraryFoldersTable,
                      LibraryFile
                    >(
                      currentTable: table,
                      referencedTable: $$LibraryFoldersTableReferences
                          ._libraryFilesRefsTable(db),
                      managerFromTypedResult: (p0) =>
                          $$LibraryFoldersTableReferences(
                            db,
                            table,
                            p0,
                          ).libraryFilesRefs,
                      referencedItemsForCurrentItem: (item, referencedItems) =>
                          referencedItems.where((e) => e.folderId == item.id),
                      typedResults: items,
                    ),
                ];
              },
            );
          },
        ),
      );
}

typedef $$LibraryFoldersTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $LibraryFoldersTable,
      LibraryFolder,
      $$LibraryFoldersTableFilterComposer,
      $$LibraryFoldersTableOrderingComposer,
      $$LibraryFoldersTableAnnotationComposer,
      $$LibraryFoldersTableCreateCompanionBuilder,
      $$LibraryFoldersTableUpdateCompanionBuilder,
      (LibraryFolder, $$LibraryFoldersTableReferences),
      LibraryFolder,
      PrefetchHooks Function({bool libraryFilesRefs})
    >;
typedef $$LibraryFilesTableCreateCompanionBuilder =
    LibraryFilesCompanion Function({
      Value<int> id,
      required int folderId,
      required String filePath,
      required String fileName,
      Value<String?> title,
      Value<String?> author,
      Value<String> format,
      Value<int> bookmarkCount,
      Value<int> pageCount,
      Value<int> currentPage,
      Value<int> fileSizeBytes,
      Value<bool> isFavorite,
      Value<String?> coverPath,
      Value<DateTime?> lastOpened,
      Value<String?> series,
      Value<String?> tags,
      Value<int> pageOffset,
      required DateTime lastModified,
      Value<DateTime> lastScanned,
    });
typedef $$LibraryFilesTableUpdateCompanionBuilder =
    LibraryFilesCompanion Function({
      Value<int> id,
      Value<int> folderId,
      Value<String> filePath,
      Value<String> fileName,
      Value<String?> title,
      Value<String?> author,
      Value<String> format,
      Value<int> bookmarkCount,
      Value<int> pageCount,
      Value<int> currentPage,
      Value<int> fileSizeBytes,
      Value<bool> isFavorite,
      Value<String?> coverPath,
      Value<DateTime?> lastOpened,
      Value<String?> series,
      Value<String?> tags,
      Value<int> pageOffset,
      Value<DateTime> lastModified,
      Value<DateTime> lastScanned,
    });

final class $$LibraryFilesTableReferences
    extends BaseReferences<_$AppDatabase, $LibraryFilesTable, LibraryFile> {
  $$LibraryFilesTableReferences(super.$_db, super.$_table, super.$_typedResult);

  static $LibraryFoldersTable _folderIdTable(_$AppDatabase db) =>
      db.libraryFolders.createAlias(
        $_aliasNameGenerator(db.libraryFiles.folderId, db.libraryFolders.id),
      );

  $$LibraryFoldersTableProcessedTableManager get folderId {
    final $_column = $_itemColumn<int>('folder_id')!;

    final manager = $$LibraryFoldersTableTableManager(
      $_db,
      $_db.libraryFolders,
    ).filter((f) => f.id.sqlEquals($_column));
    final item = $_typedResult.readTableOrNull(_folderIdTable($_db));
    if (item == null) return manager;
    return ProcessedTableManager(
      manager.$state.copyWith(prefetchedData: [item]),
    );
  }
}

class $$LibraryFilesTableFilterComposer
    extends Composer<_$AppDatabase, $LibraryFilesTable> {
  $$LibraryFilesTableFilterComposer({
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

  ColumnFilters<String> get fileName => $composableBuilder(
    column: $table.fileName,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get title => $composableBuilder(
    column: $table.title,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get author => $composableBuilder(
    column: $table.author,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get format => $composableBuilder(
    column: $table.format,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get bookmarkCount => $composableBuilder(
    column: $table.bookmarkCount,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get pageCount => $composableBuilder(
    column: $table.pageCount,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get currentPage => $composableBuilder(
    column: $table.currentPage,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get fileSizeBytes => $composableBuilder(
    column: $table.fileSizeBytes,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get isFavorite => $composableBuilder(
    column: $table.isFavorite,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get coverPath => $composableBuilder(
    column: $table.coverPath,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get lastOpened => $composableBuilder(
    column: $table.lastOpened,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get series => $composableBuilder(
    column: $table.series,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get tags => $composableBuilder(
    column: $table.tags,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get pageOffset => $composableBuilder(
    column: $table.pageOffset,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get lastModified => $composableBuilder(
    column: $table.lastModified,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get lastScanned => $composableBuilder(
    column: $table.lastScanned,
    builder: (column) => ColumnFilters(column),
  );

  $$LibraryFoldersTableFilterComposer get folderId {
    final $$LibraryFoldersTableFilterComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.folderId,
      referencedTable: $db.libraryFolders,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$LibraryFoldersTableFilterComposer(
            $db: $db,
            $table: $db.libraryFolders,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
}

class $$LibraryFilesTableOrderingComposer
    extends Composer<_$AppDatabase, $LibraryFilesTable> {
  $$LibraryFilesTableOrderingComposer({
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

  ColumnOrderings<String> get fileName => $composableBuilder(
    column: $table.fileName,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get title => $composableBuilder(
    column: $table.title,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get author => $composableBuilder(
    column: $table.author,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get format => $composableBuilder(
    column: $table.format,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get bookmarkCount => $composableBuilder(
    column: $table.bookmarkCount,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get pageCount => $composableBuilder(
    column: $table.pageCount,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get currentPage => $composableBuilder(
    column: $table.currentPage,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get fileSizeBytes => $composableBuilder(
    column: $table.fileSizeBytes,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get isFavorite => $composableBuilder(
    column: $table.isFavorite,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get coverPath => $composableBuilder(
    column: $table.coverPath,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get lastOpened => $composableBuilder(
    column: $table.lastOpened,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get series => $composableBuilder(
    column: $table.series,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get tags => $composableBuilder(
    column: $table.tags,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get pageOffset => $composableBuilder(
    column: $table.pageOffset,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get lastModified => $composableBuilder(
    column: $table.lastModified,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get lastScanned => $composableBuilder(
    column: $table.lastScanned,
    builder: (column) => ColumnOrderings(column),
  );

  $$LibraryFoldersTableOrderingComposer get folderId {
    final $$LibraryFoldersTableOrderingComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.folderId,
      referencedTable: $db.libraryFolders,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$LibraryFoldersTableOrderingComposer(
            $db: $db,
            $table: $db.libraryFolders,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
}

class $$LibraryFilesTableAnnotationComposer
    extends Composer<_$AppDatabase, $LibraryFilesTable> {
  $$LibraryFilesTableAnnotationComposer({
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

  GeneratedColumn<String> get fileName =>
      $composableBuilder(column: $table.fileName, builder: (column) => column);

  GeneratedColumn<String> get title =>
      $composableBuilder(column: $table.title, builder: (column) => column);

  GeneratedColumn<String> get author =>
      $composableBuilder(column: $table.author, builder: (column) => column);

  GeneratedColumn<String> get format =>
      $composableBuilder(column: $table.format, builder: (column) => column);

  GeneratedColumn<int> get bookmarkCount => $composableBuilder(
    column: $table.bookmarkCount,
    builder: (column) => column,
  );

  GeneratedColumn<int> get pageCount =>
      $composableBuilder(column: $table.pageCount, builder: (column) => column);

  GeneratedColumn<int> get currentPage => $composableBuilder(
    column: $table.currentPage,
    builder: (column) => column,
  );

  GeneratedColumn<int> get fileSizeBytes => $composableBuilder(
    column: $table.fileSizeBytes,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get isFavorite => $composableBuilder(
    column: $table.isFavorite,
    builder: (column) => column,
  );

  GeneratedColumn<String> get coverPath =>
      $composableBuilder(column: $table.coverPath, builder: (column) => column);

  GeneratedColumn<DateTime> get lastOpened => $composableBuilder(
    column: $table.lastOpened,
    builder: (column) => column,
  );

  GeneratedColumn<String> get series =>
      $composableBuilder(column: $table.series, builder: (column) => column);

  GeneratedColumn<String> get tags =>
      $composableBuilder(column: $table.tags, builder: (column) => column);

  GeneratedColumn<int> get pageOffset => $composableBuilder(
    column: $table.pageOffset,
    builder: (column) => column,
  );

  GeneratedColumn<DateTime> get lastModified => $composableBuilder(
    column: $table.lastModified,
    builder: (column) => column,
  );

  GeneratedColumn<DateTime> get lastScanned => $composableBuilder(
    column: $table.lastScanned,
    builder: (column) => column,
  );

  $$LibraryFoldersTableAnnotationComposer get folderId {
    final $$LibraryFoldersTableAnnotationComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.folderId,
      referencedTable: $db.libraryFolders,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$LibraryFoldersTableAnnotationComposer(
            $db: $db,
            $table: $db.libraryFolders,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
}

class $$LibraryFilesTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $LibraryFilesTable,
          LibraryFile,
          $$LibraryFilesTableFilterComposer,
          $$LibraryFilesTableOrderingComposer,
          $$LibraryFilesTableAnnotationComposer,
          $$LibraryFilesTableCreateCompanionBuilder,
          $$LibraryFilesTableUpdateCompanionBuilder,
          (LibraryFile, $$LibraryFilesTableReferences),
          LibraryFile,
          PrefetchHooks Function({bool folderId})
        > {
  $$LibraryFilesTableTableManager(_$AppDatabase db, $LibraryFilesTable table)
    : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$LibraryFilesTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$LibraryFilesTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$LibraryFilesTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<int> folderId = const Value.absent(),
                Value<String> filePath = const Value.absent(),
                Value<String> fileName = const Value.absent(),
                Value<String?> title = const Value.absent(),
                Value<String?> author = const Value.absent(),
                Value<String> format = const Value.absent(),
                Value<int> bookmarkCount = const Value.absent(),
                Value<int> pageCount = const Value.absent(),
                Value<int> currentPage = const Value.absent(),
                Value<int> fileSizeBytes = const Value.absent(),
                Value<bool> isFavorite = const Value.absent(),
                Value<String?> coverPath = const Value.absent(),
                Value<DateTime?> lastOpened = const Value.absent(),
                Value<String?> series = const Value.absent(),
                Value<String?> tags = const Value.absent(),
                Value<int> pageOffset = const Value.absent(),
                Value<DateTime> lastModified = const Value.absent(),
                Value<DateTime> lastScanned = const Value.absent(),
              }) => LibraryFilesCompanion(
                id: id,
                folderId: folderId,
                filePath: filePath,
                fileName: fileName,
                title: title,
                author: author,
                format: format,
                bookmarkCount: bookmarkCount,
                pageCount: pageCount,
                currentPage: currentPage,
                fileSizeBytes: fileSizeBytes,
                isFavorite: isFavorite,
                coverPath: coverPath,
                lastOpened: lastOpened,
                series: series,
                tags: tags,
                pageOffset: pageOffset,
                lastModified: lastModified,
                lastScanned: lastScanned,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                required int folderId,
                required String filePath,
                required String fileName,
                Value<String?> title = const Value.absent(),
                Value<String?> author = const Value.absent(),
                Value<String> format = const Value.absent(),
                Value<int> bookmarkCount = const Value.absent(),
                Value<int> pageCount = const Value.absent(),
                Value<int> currentPage = const Value.absent(),
                Value<int> fileSizeBytes = const Value.absent(),
                Value<bool> isFavorite = const Value.absent(),
                Value<String?> coverPath = const Value.absent(),
                Value<DateTime?> lastOpened = const Value.absent(),
                Value<String?> series = const Value.absent(),
                Value<String?> tags = const Value.absent(),
                Value<int> pageOffset = const Value.absent(),
                required DateTime lastModified,
                Value<DateTime> lastScanned = const Value.absent(),
              }) => LibraryFilesCompanion.insert(
                id: id,
                folderId: folderId,
                filePath: filePath,
                fileName: fileName,
                title: title,
                author: author,
                format: format,
                bookmarkCount: bookmarkCount,
                pageCount: pageCount,
                currentPage: currentPage,
                fileSizeBytes: fileSizeBytes,
                isFavorite: isFavorite,
                coverPath: coverPath,
                lastOpened: lastOpened,
                series: series,
                tags: tags,
                pageOffset: pageOffset,
                lastModified: lastModified,
                lastScanned: lastScanned,
              ),
          withReferenceMapper: (p0) => p0
              .map(
                (e) => (
                  e.readTable(table),
                  $$LibraryFilesTableReferences(db, table, e),
                ),
              )
              .toList(),
          prefetchHooksCallback: ({folderId = false}) {
            return PrefetchHooks(
              db: db,
              explicitlyWatchedTables: [],
              addJoins:
                  <
                    T extends TableManagerState<
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic
                    >
                  >(state) {
                    if (folderId) {
                      state =
                          state.withJoin(
                                currentTable: table,
                                currentColumn: table.folderId,
                                referencedTable: $$LibraryFilesTableReferences
                                    ._folderIdTable(db),
                                referencedColumn: $$LibraryFilesTableReferences
                                    ._folderIdTable(db)
                                    .id,
                              )
                              as T;
                    }

                    return state;
                  },
              getPrefetchedDataCallback: (items) async {
                return [];
              },
            );
          },
        ),
      );
}

typedef $$LibraryFilesTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $LibraryFilesTable,
      LibraryFile,
      $$LibraryFilesTableFilterComposer,
      $$LibraryFilesTableOrderingComposer,
      $$LibraryFilesTableAnnotationComposer,
      $$LibraryFilesTableCreateCompanionBuilder,
      $$LibraryFilesTableUpdateCompanionBuilder,
      (LibraryFile, $$LibraryFilesTableReferences),
      LibraryFile,
      PrefetchHooks Function({bool folderId})
    >;
typedef $$AnnotationsTableCreateCompanionBuilder =
    AnnotationsCompanion Function({
      Value<int> id,
      required String filePath,
      required int pageNumber,
      required String type,
      Value<String?> selectedText,
      Value<String?> note,
      Value<String> colorHex,
      Value<double> rectX,
      Value<double> rectY,
      Value<double> rectWidth,
      Value<double> rectHeight,
      Value<DateTime> createdAt,
    });
typedef $$AnnotationsTableUpdateCompanionBuilder =
    AnnotationsCompanion Function({
      Value<int> id,
      Value<String> filePath,
      Value<int> pageNumber,
      Value<String> type,
      Value<String?> selectedText,
      Value<String?> note,
      Value<String> colorHex,
      Value<double> rectX,
      Value<double> rectY,
      Value<double> rectWidth,
      Value<double> rectHeight,
      Value<DateTime> createdAt,
    });

class $$AnnotationsTableFilterComposer
    extends Composer<_$AppDatabase, $AnnotationsTable> {
  $$AnnotationsTableFilterComposer({
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

  ColumnFilters<int> get pageNumber => $composableBuilder(
    column: $table.pageNumber,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get type => $composableBuilder(
    column: $table.type,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get selectedText => $composableBuilder(
    column: $table.selectedText,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get note => $composableBuilder(
    column: $table.note,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get colorHex => $composableBuilder(
    column: $table.colorHex,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<double> get rectX => $composableBuilder(
    column: $table.rectX,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<double> get rectY => $composableBuilder(
    column: $table.rectY,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<double> get rectWidth => $composableBuilder(
    column: $table.rectWidth,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<double> get rectHeight => $composableBuilder(
    column: $table.rectHeight,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get createdAt => $composableBuilder(
    column: $table.createdAt,
    builder: (column) => ColumnFilters(column),
  );
}

class $$AnnotationsTableOrderingComposer
    extends Composer<_$AppDatabase, $AnnotationsTable> {
  $$AnnotationsTableOrderingComposer({
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

  ColumnOrderings<int> get pageNumber => $composableBuilder(
    column: $table.pageNumber,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get type => $composableBuilder(
    column: $table.type,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get selectedText => $composableBuilder(
    column: $table.selectedText,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get note => $composableBuilder(
    column: $table.note,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get colorHex => $composableBuilder(
    column: $table.colorHex,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<double> get rectX => $composableBuilder(
    column: $table.rectX,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<double> get rectY => $composableBuilder(
    column: $table.rectY,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<double> get rectWidth => $composableBuilder(
    column: $table.rectWidth,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<double> get rectHeight => $composableBuilder(
    column: $table.rectHeight,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get createdAt => $composableBuilder(
    column: $table.createdAt,
    builder: (column) => ColumnOrderings(column),
  );
}

class $$AnnotationsTableAnnotationComposer
    extends Composer<_$AppDatabase, $AnnotationsTable> {
  $$AnnotationsTableAnnotationComposer({
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

  GeneratedColumn<int> get pageNumber => $composableBuilder(
    column: $table.pageNumber,
    builder: (column) => column,
  );

  GeneratedColumn<String> get type =>
      $composableBuilder(column: $table.type, builder: (column) => column);

  GeneratedColumn<String> get selectedText => $composableBuilder(
    column: $table.selectedText,
    builder: (column) => column,
  );

  GeneratedColumn<String> get note =>
      $composableBuilder(column: $table.note, builder: (column) => column);

  GeneratedColumn<String> get colorHex =>
      $composableBuilder(column: $table.colorHex, builder: (column) => column);

  GeneratedColumn<double> get rectX =>
      $composableBuilder(column: $table.rectX, builder: (column) => column);

  GeneratedColumn<double> get rectY =>
      $composableBuilder(column: $table.rectY, builder: (column) => column);

  GeneratedColumn<double> get rectWidth =>
      $composableBuilder(column: $table.rectWidth, builder: (column) => column);

  GeneratedColumn<double> get rectHeight => $composableBuilder(
    column: $table.rectHeight,
    builder: (column) => column,
  );

  GeneratedColumn<DateTime> get createdAt =>
      $composableBuilder(column: $table.createdAt, builder: (column) => column);
}

class $$AnnotationsTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $AnnotationsTable,
          DocumentAnnotation,
          $$AnnotationsTableFilterComposer,
          $$AnnotationsTableOrderingComposer,
          $$AnnotationsTableAnnotationComposer,
          $$AnnotationsTableCreateCompanionBuilder,
          $$AnnotationsTableUpdateCompanionBuilder,
          (
            DocumentAnnotation,
            BaseReferences<
              _$AppDatabase,
              $AnnotationsTable,
              DocumentAnnotation
            >,
          ),
          DocumentAnnotation,
          PrefetchHooks Function()
        > {
  $$AnnotationsTableTableManager(_$AppDatabase db, $AnnotationsTable table)
    : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$AnnotationsTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$AnnotationsTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$AnnotationsTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<String> filePath = const Value.absent(),
                Value<int> pageNumber = const Value.absent(),
                Value<String> type = const Value.absent(),
                Value<String?> selectedText = const Value.absent(),
                Value<String?> note = const Value.absent(),
                Value<String> colorHex = const Value.absent(),
                Value<double> rectX = const Value.absent(),
                Value<double> rectY = const Value.absent(),
                Value<double> rectWidth = const Value.absent(),
                Value<double> rectHeight = const Value.absent(),
                Value<DateTime> createdAt = const Value.absent(),
              }) => AnnotationsCompanion(
                id: id,
                filePath: filePath,
                pageNumber: pageNumber,
                type: type,
                selectedText: selectedText,
                note: note,
                colorHex: colorHex,
                rectX: rectX,
                rectY: rectY,
                rectWidth: rectWidth,
                rectHeight: rectHeight,
                createdAt: createdAt,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                required String filePath,
                required int pageNumber,
                required String type,
                Value<String?> selectedText = const Value.absent(),
                Value<String?> note = const Value.absent(),
                Value<String> colorHex = const Value.absent(),
                Value<double> rectX = const Value.absent(),
                Value<double> rectY = const Value.absent(),
                Value<double> rectWidth = const Value.absent(),
                Value<double> rectHeight = const Value.absent(),
                Value<DateTime> createdAt = const Value.absent(),
              }) => AnnotationsCompanion.insert(
                id: id,
                filePath: filePath,
                pageNumber: pageNumber,
                type: type,
                selectedText: selectedText,
                note: note,
                colorHex: colorHex,
                rectX: rectX,
                rectY: rectY,
                rectWidth: rectWidth,
                rectHeight: rectHeight,
                createdAt: createdAt,
              ),
          withReferenceMapper: (p0) => p0
              .map((e) => (e.readTable(table), BaseReferences(db, table, e)))
              .toList(),
          prefetchHooksCallback: null,
        ),
      );
}

typedef $$AnnotationsTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $AnnotationsTable,
      DocumentAnnotation,
      $$AnnotationsTableFilterComposer,
      $$AnnotationsTableOrderingComposer,
      $$AnnotationsTableAnnotationComposer,
      $$AnnotationsTableCreateCompanionBuilder,
      $$AnnotationsTableUpdateCompanionBuilder,
      (
        DocumentAnnotation,
        BaseReferences<_$AppDatabase, $AnnotationsTable, DocumentAnnotation>,
      ),
      DocumentAnnotation,
      PrefetchHooks Function()
    >;

class $AppDatabaseManager {
  final _$AppDatabase _db;
  $AppDatabaseManager(this._db);
  $$BookmarksTableTableManager get bookmarks =>
      $$BookmarksTableTableManager(_db, _db.bookmarks);
  $$TagsTableTableManager get tags => $$TagsTableTableManager(_db, _db.tags);
  $$BookmarkTagsTableTableManager get bookmarkTags =>
      $$BookmarkTagsTableTableManager(_db, _db.bookmarkTags);
  $$FileSnapshotsTableTableManager get fileSnapshots =>
      $$FileSnapshotsTableTableManager(_db, _db.fileSnapshots);
  $$LibraryFoldersTableTableManager get libraryFolders =>
      $$LibraryFoldersTableTableManager(_db, _db.libraryFolders);
  $$LibraryFilesTableTableManager get libraryFiles =>
      $$LibraryFilesTableTableManager(_db, _db.libraryFiles);
  $$AnnotationsTableTableManager get annotations =>
      $$AnnotationsTableTableManager(_db, _db.annotations);
}
