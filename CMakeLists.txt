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

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 Pdf)
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

add_library(atlas_app_support STATIC
    src/app/diagnostics/ShellMetrics.cpp
    src/app/diagnostics/ShellMetrics.h
    src/app/logging/Logging.cpp
    src/app/logging/Logging.h
)
target_include_directories(atlas_app_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(atlas_app_support PUBLIC Qt6::Core Qt6::Gui Qt6::Quick)
atlas_enable_warnings(atlas_app_support)

qt_add_executable(atlas_reader src/app/main.cpp)

qt_add_qml_module(atlas_reader
    URI AtlasReader
    VERSION 1.0
    QML_FILES qml/Main.qml
)

target_compile_definitions(atlas_reader PRIVATE ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}")
target_link_libraries(atlas_reader
    PRIVATE atlas_core atlas_app_support Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::QuickControls2
)
atlas_enable_warnings(atlas_reader)

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

if(ATLAS_ENABLE_PDFIUM_PROBE)
    add_executable(atlas_pdfium_probe tools/pdf/pdfium_probe.cpp)
    target_compile_definitions(atlas_pdfium_probe PRIVATE
        ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}"
        ATLAS_PDFIUM_PIN="chromium/8066"
        ATLAS_PDFIUM_VERSION="156.0.8066.0"
    )
    target_link_libraries(atlas_pdfium_probe PRIVATE Qt6::Core pdfium)
    atlas_enable_warnings(atlas_pdfium_probe)

    if(WIN32)
        add_custom_command(
            TARGET atlas_pdfium_probe
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:pdfium>
                    $<TARGET_FILE_DIR:atlas_pdfium_probe>
            VERBATIM
        )
    endif()
endif()

enable_testing()

add_executable(atlas_core_smoke tests/core_smoke.cpp)
target_link_libraries(atlas_core_smoke PRIVATE atlas_core)
atlas_enable_warnings(atlas_core_smoke)
add_test(NAME atlas_core_smoke COMMAND atlas_core_smoke)

set(ATLAS_N2_PDF_FIXTURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/pdf/generated")

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
endif()
