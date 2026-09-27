cmake_minimum_required(VERSION 3.28)

project(
    AtlasReaderNative
    VERSION 2.0.0
    DESCRIPTION "Native successor to Atlas Reader"
    LANGUAGES CXX
)

set(ATLAS_PRERELEASE "alpha.2" CACHE STRING "Atlas prerelease identifier")
set(ATLAS_VERSION_STRING "${PROJECT_VERSION}-${ATLAS_PRERELEASE}")
option(ATLAS_WARNINGS_AS_ERRORS "Treat Atlas compiler warnings as errors" OFF)
option(ATLAS_ENABLE_PDFIUM_PROBE "Build the N2 PDFium qualification probe" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Concurrent Gui Quick Qml QuickControls2 Pdf)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
qt_standard_project_setup(REQUIRES 6.10)

if(ATLAS_ENABLE_PDFIUM_PROBE)
    find_package(PDFium REQUIRED)
endif()

function(atlas_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
        if(ATLAS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(ATLAS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

add_library(atlas_core INTERFACE)
target_include_directories(atlas_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(atlas_core INTERFACE cxx_std_23)

add_library(atlas_document_identity STATIC
    src/core/index/DocumentIdentityReconciler.cpp
    src/core/index/DocumentIdentityReconciler.h
)
target_include_directories(atlas_document_identity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_document_identity PUBLIC atlas_core)
atlas_enable_warnings(atlas_document_identity)

add_library(atlas_file_identity STATIC
    src/app/index/IFileIdentityProvider.h
    src/app/index/NativeFileIdentityProvider.cpp
    src/app/index/NativeFileIdentityProvider.h
)
target_include_directories(atlas_file_identity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_file_identity PUBLIC atlas_core)
atlas_enable_warnings(atlas_file_identity)

add_library(atlas_library_index STATIC
    src/app/index/SqliteLibraryIndex.cpp
    src/app/index/SqliteLibraryIndex.h
)
target_include_directories(atlas_library_index PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_library_index PUBLIC atlas_core atlas_document_identity unofficial::sqlite3::sqlite3)
atlas_enable_warnings(atlas_library_index)

add_library(atlas_library_scanner STATIC
    src/app/index/LibraryScanner.cpp
    src/app/index/LibraryScanner.h
    src/app/index/LibraryPathKey.cpp
    src/app/index/LibraryPathKey.h
    src/app/index/NativeLibraryEnumerator.cpp
    src/app/index/NativeLibraryEnumerator.h
    src/app/index/ILibraryEnumerator.h
)
target_include_directories(atlas_library_scanner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_library_scanner PUBLIC atlas_core)
atlas_enable_warnings(atlas_library_scanner)

add_library(atlas_library_scan_service STATIC
    src/app/index/LibraryScanService.cpp
    src/app/index/LibraryScanService.h
)
target_include_directories(atlas_library_scan_service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_library_scan_service PUBLIC atlas_library_scanner atlas_library_index atlas_file_identity)
atlas_enable_warnings(atlas_library_scan_service)

add_library(atlas_library_document_inspector STATIC
    src/app/index/ILibraryDocumentInspector.h
    src/app/index/QtPdfDocumentInspector.cpp
    src/app/index/QtPdfDocumentInspector.h
)
target_include_directories(atlas_library_document_inspector PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_library_document_inspector PUBLIC atlas_core Qt6::Core Qt6::Pdf)
atlas_enable_warnings(atlas_library_document_inspector)

add_library(atlas_library_ingestion STATIC
    src/app/index/LibraryIngestionService.cpp
    src/app/index/LibraryIngestionService.h
)
target_include_directories(atlas_library_ingestion PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_library_ingestion PUBLIC
    atlas_library_scan_service atlas_library_document_inspector atlas_library_index
)
atlas_enable_warnings(atlas_library_ingestion)

add_library(atlas_app_support STATIC
    src/app/diagnostics/ShellMetrics.cpp
    src/app/diagnostics/ShellMetrics.h
    src/app/logging/Logging.cpp
    src/app/logging/Logging.h
)
target_include_directories(atlas_app_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_app_support PUBLIC Qt6::Core Qt6::Gui Qt6::Quick)
atlas_enable_warnings(atlas_app_support)

add_library(atlas_library_ui STATIC
    src/app/library/LibraryController.cpp
    src/app/library/LibraryController.h
)
target_include_directories(atlas_library_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_library_ui PUBLIC
    atlas_library_ingestion atlas_file_identity atlas_library_scanner
    Qt6::Core Qt6::Concurrent Qt6::Gui
)
atlas_enable_warnings(atlas_library_ui)

qt_add_executable(atlas_reader src/app/main.cpp)

qt_add_qml_module(atlas_reader
    URI AtlasReader
    VERSION 1.0
    QML_FILES qml/AtlasButton.qml qml/Main.qml
)

target_compile_definitions(atlas_reader PRIVATE ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}")
target_link_libraries(atlas_reader
    PRIVATE atlas_core atlas_app_support atlas_library_ui
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::QuickControls2
)
atlas_enable_warnings(atlas_reader)

add_executable(atlas_n3_library_benchmark tools/bench/n3_library_benchmark.cpp)
target_link_libraries(atlas_n3_library_benchmark PRIVATE atlas_library_scanner atlas_library_index)
atlas_enable_warnings(atlas_n3_library_benchmark)

if(WIN32)
    add_custom_command(
        TARGET atlas_reader
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/qt.conf
                $<TARGET_FILE_DIR:atlas_reader>/qt.conf
        VERBATIM
    )
endif()

add_executable(atlas_qt_pdf_probe tools/pdf/qt_pdf_probe.cpp)
target_compile_definitions(atlas_qt_pdf_probe PRIVATE ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}")
target_link_libraries(atlas_qt_pdf_probe PRIVATE Qt6::Core Qt6::Gui Qt6::Pdf)
atlas_enable_warnings(atlas_qt_pdf_probe)

add_executable(atlas_qt_pdf_search_probe tools/pdf/qt_pdf_search_probe.cpp)
target_compile_definitions(atlas_qt_pdf_search_probe PRIVATE ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}")
target_link_libraries(atlas_qt_pdf_search_probe PRIVATE Qt6::Core Qt6::Pdf)
atlas_enable_warnings(atlas_qt_pdf_search_probe)

add_executable(atlas_qt_pdf_security_probe tools/pdf/qt_pdf_security_probe.cpp)
target_compile_definitions(atlas_qt_pdf_security_probe PRIVATE ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}")
target_link_libraries(atlas_qt_pdf_security_probe PRIVATE Qt6::Core Qt6::Pdf)
atlas_enable_warnings(atlas_qt_pdf_security_probe)

if(ATLAS_ENABLE_PDFIUM_PROBE)
    add_executable(atlas_pdfium_probe tools/pdf/pdfium_probe.cpp)
    target_compile_definitions(atlas_pdfium_probe PRIVATE
        ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}"
        ATLAS_PDFIUM_PIN="chromium/8066"
        ATLAS_PDFIUM_VERSION="156.0.8066.0"
    )
    target_link_libraries(atlas_pdfium_probe PRIVATE Qt6::Core pdfium)
    atlas_enable_warnings(atlas_pdfium_probe)

    add_executable(atlas_pdfium_search_probe tools/pdf/pdfium_search_probe.cpp)
    target_compile_definitions(atlas_pdfium_search_probe PRIVATE
        ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}"
        ATLAS_PDFIUM_PIN="chromium/8066"
        ATLAS_PDFIUM_VERSION="156.0.8066.0"
    )
    target_link_libraries(atlas_pdfium_search_probe PRIVATE Qt6::Core pdfium)
    atlas_enable_warnings(atlas_pdfium_search_probe)

    add_executable(atlas_pdfium_security_probe tools/pdf/pdfium_security_probe.cpp)
    target_compile_definitions(atlas_pdfium_security_probe PRIVATE
        ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}"
        ATLAS_PDFIUM_PIN="chromium/8066"
        ATLAS_PDFIUM_VERSION="156.0.8066.0"
    )
    target_link_libraries(atlas_pdfium_security_probe PRIVATE Qt6::Core pdfium)
    atlas_enable_warnings(atlas_pdfium_security_probe)

    if(WIN32)
        add_custom_command(
            TARGET atlas_pdfium_probe
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:pdfium>
                    $<TARGET_FILE_DIR:atlas_pdfium_probe>
            VERBATIM
        )
        add_custom_command(
            TARGET atlas_pdfium_search_probe
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:pdfium>
                    $<TARGET_FILE_DIR:atlas_pdfium_search_probe>
            VERBATIM
        )
        add_custom_command(
            TARGET atlas_pdfium_security_probe
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:pdfium>
                    $<TARGET_FILE_DIR:atlas_pdfium_security_probe>
            VERBATIM
        )
    endif()
endif()

enable_testing()

add_executable(atlas_core_smoke tests/core_smoke.cpp)
target_link_libraries(atlas_core_smoke PRIVATE atlas_core)
atlas_enable_warnings(atlas_core_smoke)
add_test(NAME atlas_core_smoke COMMAND atlas_core_smoke)

add_executable(atlas_document_identity_tests tests/index/document_identity_reconciler_tests.cpp)
target_link_libraries(atlas_document_identity_tests PRIVATE atlas_document_identity)
atlas_enable_warnings(atlas_document_identity_tests)
add_test(NAME atlas_document_identity COMMAND atlas_document_identity_tests)

add_executable(atlas_native_file_identity_tests tests/index/native_file_identity_tests.cpp)
target_link_libraries(atlas_native_file_identity_tests PRIVATE atlas_file_identity)
atlas_enable_warnings(atlas_native_file_identity_tests)
add_test(NAME atlas_native_file_identity COMMAND atlas_native_file_identity_tests)

add_executable(atlas_library_index_tests tests/index/library_index_tests.cpp)
target_link_libraries(atlas_library_index_tests PRIVATE atlas_library_index)
atlas_enable_warnings(atlas_library_index_tests)
add_test(NAME atlas_library_index COMMAND atlas_library_index_tests)

add_executable(atlas_library_scanner_tests tests/index/library_scanner_tests.cpp)
target_link_libraries(atlas_library_scanner_tests PRIVATE atlas_library_scanner atlas_library_scan_service)
atlas_enable_warnings(atlas_library_scanner_tests)
add_test(NAME atlas_library_scanner COMMAND atlas_library_scanner_tests)

add_executable(atlas_library_ingestion_tests tests/index/library_ingestion_tests.cpp)
target_link_libraries(atlas_library_ingestion_tests PRIVATE
    atlas_library_ingestion atlas_file_identity atlas_library_scanner
)
atlas_enable_warnings(atlas_library_ingestion_tests)
add_test(NAME atlas_library_ingestion COMMAND atlas_library_ingestion_tests)

add_executable(atlas_library_document_inspector_tests tests/index/library_document_inspector_tests.cpp)
target_link_libraries(atlas_library_document_inspector_tests PRIVATE atlas_library_document_inspector)
atlas_enable_warnings(atlas_library_document_inspector_tests)

set(ATLAS_N2_PDF_FIXTURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/pdf/generated")

add_test(
    NAME atlas_library_document_inspector
    COMMAND atlas_library_document_inspector_tests
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A001_simple_text.pdf"
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A007_malformed_truncated.pdf"
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A008_password_rc4_40.pdf"
)

add_test(
    NAME n2_qt_pdf_a001_simple_text
    COMMAND atlas_qt_pdf_probe
        --fixture-id A001
        --expect-pages 1
        --expect-labels 1
        --expect-text-contains "Atlas N2 Fixture A001"
        --expect-outline-count 0
        --expect-link-count 0
        --render-page 0
        --render-width 612
        --render-height 792
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A001_simple_text.pdf"
)

add_test(
    NAME n2_qt_pdf_a002_geometry_labels
    COMMAND atlas_qt_pdf_probe
        --fixture-id A002
        --expect-pages 3
        --expect-labels i,ii,1
        --expect-text-contains "Atlas N2 Fixture A002 page 1"
        --expect-outline-count 0
        --expect-link-count 0
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A002_geometry_labels.pdf"
)

add_test(
    NAME n2_qt_pdf_a003_outlines_links
    COMMAND atlas_qt_pdf_probe
        --fixture-id A003
        --expect-pages 3
        --expect-labels 1,2,3
        --expect-text-contains "Atlas N2 Fixture A003 page 1"
        --expect-outline-count 3
        --expect-outline-titles "Chapter 1|Section 1.1|Chapter 2"
        --expect-outline-depths 0,1,0
        --expect-outline-pages 0,1,2
        --expect-link-count 3
        --expect-internal-link-page 2
        --expect-external-uri "https://example.com/atlas-n2-fixture"
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A003_outlines_links.pdf"
)

add_test(
    NAME n2_qt_pdf_a004_outlines_only
    COMMAND atlas_qt_pdf_probe
        --fixture-id A004
        --expect-pages 3
        --expect-labels 1,2,3
        --expect-text-contains "Atlas N2 Fixture A004 page 1"
        --expect-outline-count 3
        --expect-outline-titles "Chapter 1|Section 1.1|Chapter 2"
        --expect-outline-depths 0,1,0
        --expect-outline-pages 0,1,2
        --expect-link-count 0
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A004_outlines_only.pdf"
)

add_test(
    NAME n2_qt_pdf_a005_links_only
    COMMAND atlas_qt_pdf_probe
        --fixture-id A005
        --expect-pages 3
        --expect-labels 1,2,3
        --expect-text-contains "Atlas N2 Fixture A005 page 1"
        --expect-outline-count 0
        --expect-link-count 3
        --expect-internal-link-page 2
        --expect-external-uri "https://example.com/atlas-n2-fixture"
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A005_links_only.pdf"
)

# A006 is regenerated from checked-in source before CI validation/tests. Its
# Type3 glyphs are synthetic; these tests qualify logical Unicode extraction
# and outline semantics only, never Arabic/Urdu shaping or raster fidelity.
add_test(
    NAME n2_qt_pdf_a006_arabic_outline
    COMMAND atlas_qt_pdf_probe
        --fixture-id A006
        --expect-pages 4
        --expect-labels 1,2,3,4
        --expect-text-contains "مرحبا بالعالم"
        --expect-outline-count 4
        --expect-outline-titles "العربية|مُشَكَّل|Mixed العربية English|اردو"
        --expect-outline-depths 0,1,0,0
        --expect-outline-pages 0,1,2,3
        --expect-link-count 0
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
)

# Both read engines preserve every vocalized code point but reverse the order
# of combining marks within a base-letter cluster. This full raw line has
# SHA-256 376cdb244082d82602d5f60ab1edff730450a55d53b5b9bcb2eaf2755ce76cb0
# and is NFD/canonically equivalent to the source fixture string.
add_test(
    NAME n2_qt_pdf_a006_vocalized_arabic
    COMMAND atlas_qt_pdf_probe
        --fixture-id A006
        --expect-pages 4
        --expect-text-contains "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ"
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
)

add_test(
    NAME n2_qt_pdf_a006_urdu
    COMMAND atlas_qt_pdf_probe
        --fixture-id A006
        --expect-pages 4
        --expect-text-contains "یہ اردو متن ہے"
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
)

# Search probes normalize only document-order page/ordinal identity. Qt's
# IndexOnPage and PDFium's character start/count remain engine-native evidence.
add_test(
    NAME n2_qt_pdf_search_a001_english
    COMMAND atlas_qt_pdf_search_probe
        --fixture-id A001
        --query "Atlas N2 Fixture A001"
        --expect-count 1
        --expect-first-page 0
        --expect-first-ordinal 0
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A001_simple_text.pdf"
)

add_test(
    NAME n2_qt_pdf_search_a006_arabic
    COMMAND atlas_qt_pdf_search_probe
        --fixture-id A006
        --query "مرحبا بالعالم"
        --expect-count 2
        --expect-first-page 0
        --expect-first-ordinal 0
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
)

add_test(
    NAME n2_qt_pdf_search_a006_mixed_english
    COMMAND atlas_qt_pdf_search_probe
        --fixture-id A006
        --query "Atlas PDF 123"
        --expect-count 1
        --expect-first-page 2
        --expect-first-ordinal 0
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
)

add_test(
    NAME n2_qt_pdf_search_a006_urdu
    COMMAND atlas_qt_pdf_search_probe
        --fixture-id A006
        --query "یہ اردو متن ہے"
        --expect-count 1
        --expect-first-page 3
        --expect-first-ordinal 0
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
)

add_test(
    NAME n2_qt_pdf_search_a006_vocalized_raw
    COMMAND atlas_qt_pdf_search_probe
        --fixture-id A006
        --query "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ"
        --expect-count 1
        --expect-first-page 1
        --expect-first-ordinal 0
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
)

# A007/A008 are deterministic security fixtures regenerated before CI tests.
# Password values are public test data; probes only emit password_supplied.
add_test(
    NAME n2_qt_pdf_security_a007_malformed
    COMMAND atlas_qt_pdf_security_probe
        --fixture-id A007-malformed
        --expect-load-error invalid-file-format
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A007_malformed_truncated.pdf"
)

add_test(
    NAME n2_qt_pdf_security_a008_no_password
    COMMAND atlas_qt_pdf_security_probe
        --fixture-id A008-no-password
        --expect-load-error incorrect-password
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A008_password_rc4_40.pdf"
)

add_test(
    NAME n2_qt_pdf_security_a008_wrong_password
    COMMAND atlas_qt_pdf_security_probe
        --fixture-id A008-wrong-password
        --password atlas-wrong
        --expect-load-error incorrect-password
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A008_password_rc4_40.pdf"
)

add_test(
    NAME n2_qt_pdf_security_a008_correct_password
    COMMAND atlas_qt_pdf_security_probe
        --fixture-id A008-correct-password
        --password atlas-user
        --expect-load-error none
        --expect-pages 1
        --expect-labels 1
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A008_password_rc4_40.pdf"
)

if(ATLAS_ENABLE_PDFIUM_PROBE)
    add_test(
        NAME n2_pdfium_a001_simple_text
        COMMAND atlas_pdfium_probe
            --fixture-id A001
            --expect-pages 1
            --expect-labels 1
            --expect-text-contains "Atlas N2 Fixture A001"
            --expect-outline-count 0
            --expect-link-count 0
            --render-page 0
            --render-width 612
            --render-height 792
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A001_simple_text.pdf"
    )

    add_test(
        NAME n2_pdfium_a002_geometry_labels
        COMMAND atlas_pdfium_probe
            --fixture-id A002
            --expect-pages 3
            --expect-labels i,ii,1
            --expect-text-contains "Atlas N2 Fixture A002 page 1"
            --expect-outline-count 0
            --expect-link-count 0
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A002_geometry_labels.pdf"
    )

    add_test(
        NAME n2_pdfium_a003_outlines_links
        COMMAND atlas_pdfium_probe
            --fixture-id A003
            --expect-pages 3
            --expect-labels 1,2,3
            --expect-text-contains "Atlas N2 Fixture A003 page 1"
            --expect-outline-count 3
            --expect-outline-titles "Chapter 1|Section 1.1|Chapter 2"
            --expect-outline-depths 0,1,0
            --expect-outline-pages 0,1,2
            --expect-link-count 2
            --expect-internal-link-page 2
            --expect-external-uri "https://example.com/atlas-n2-fixture"
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A003_outlines_links.pdf"
    )

    add_test(
        NAME n2_pdfium_a004_outlines_only
        COMMAND atlas_pdfium_probe
            --fixture-id A004
            --expect-pages 3
            --expect-labels 1,2,3
            --expect-text-contains "Atlas N2 Fixture A004 page 1"
            --expect-outline-count 3
            --expect-outline-titles "Chapter 1|Section 1.1|Chapter 2"
            --expect-outline-depths 0,1,0
            --expect-outline-pages 0,1,2
            --expect-link-count 0
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A004_outlines_only.pdf"
    )

    add_test(
        NAME n2_pdfium_a005_links_only
        COMMAND atlas_pdfium_probe
            --fixture-id A005
            --expect-pages 3
            --expect-labels 1,2,3
            --expect-text-contains "Atlas N2 Fixture A005 page 1"
            --expect-outline-count 0
            --expect-link-count 2
            --expect-internal-link-page 2
            --expect-external-uri "https://example.com/atlas-n2-fixture"
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A005_links_only.pdf"
    )

    add_test(
        NAME n2_pdfium_a006_arabic_outline
        COMMAND atlas_pdfium_probe
            --fixture-id A006
            --expect-pages 4
            --expect-labels 1,2,3,4
            --expect-text-contains "مرحبا بالعالم"
            --expect-outline-count 4
            --expect-outline-titles "العربية|مُشَكَّل|Mixed العربية English|اردو"
            --expect-outline-depths 0,1,0,0
            --expect-outline-pages 0,1,2,3
            --expect-link-count 0
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
    )

    add_test(
        NAME n2_pdfium_a006_vocalized_arabic
        COMMAND atlas_pdfium_probe
            --fixture-id A006
            --expect-pages 4
            --expect-text-contains "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ"
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
    )

    add_test(
        NAME n2_pdfium_a006_urdu
        COMMAND atlas_pdfium_probe
            --fixture-id A006
            --expect-pages 4
            --expect-text-contains "یہ اردو متن ہے"
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
    )

    add_test(
        NAME n2_pdfium_search_a001_english
        COMMAND atlas_pdfium_search_probe
            --fixture-id A001
            --query "Atlas N2 Fixture A001"
            --expect-count 1
            --expect-first-page 0
            --expect-first-ordinal 0
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A001_simple_text.pdf"
    )

    add_test(
        NAME n2_pdfium_search_a006_arabic
        COMMAND atlas_pdfium_search_probe
            --fixture-id A006
            --query "مرحبا بالعالم"
            --expect-count 2
            --expect-first-page 0
            --expect-first-ordinal 0
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
    )

    add_test(
        NAME n2_pdfium_search_a006_mixed_english
        COMMAND atlas_pdfium_search_probe
            --fixture-id A006
            --query "Atlas PDF 123"
            --expect-count 1
            --expect-first-page 2
            --expect-first-ordinal 0
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
    )

    add_test(
        NAME n2_pdfium_search_a006_urdu
        COMMAND atlas_pdfium_search_probe
            --fixture-id A006
            --query "یہ اردو متن ہے"
            --expect-count 1
            --expect-first-page 3
            --expect-first-ordinal 0
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
    )

    add_test(
        NAME n2_pdfium_search_a006_vocalized_raw
        COMMAND atlas_pdfium_search_probe
            --fixture-id A006
            --query "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ"
            --expect-count 1
            --expect-first-page 1
            --expect-first-ordinal 0
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A006_unicode_semantics.pdf"
    )

    add_test(
        NAME n2_pdfium_security_a007_malformed
        COMMAND atlas_pdfium_security_probe
            --fixture-id A007-malformed
            --expect-load-error format
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A007_malformed_truncated.pdf"
    )

    add_test(
        NAME n2_pdfium_security_a008_no_password
        COMMAND atlas_pdfium_security_probe
            --fixture-id A008-no-password
            --expect-load-error password
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A008_password_rc4_40.pdf"
    )

    add_test(
        NAME n2_pdfium_security_a008_wrong_password
        COMMAND atlas_pdfium_security_probe
            --fixture-id A008-wrong-password
            --password atlas-wrong
            --expect-load-error password
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A008_password_rc4_40.pdf"
    )

    add_test(
        NAME n2_pdfium_security_a008_correct_password
        COMMAND atlas_pdfium_security_probe
            --fixture-id A008-correct-password
            --password atlas-user
            --expect-load-error success
            --expect-pages 1
            --expect-labels 1
            "${ATLAS_N2_PDF_FIXTURE_DIR}/A008_password_rc4_40.pdf"
    )
endif()
