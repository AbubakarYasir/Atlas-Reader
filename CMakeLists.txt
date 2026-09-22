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

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# N1's accepted canonical public baseline remains Qt 6.10.3 / MSVC 2022.
# N2 inherits that toolchain. Qt PDF is currently linked only to the focused
# N2 qualification probe; atlas_reader and atlas_core do not depend on Qt PDF.
find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 Pdf)
qt_standard_project_setup(REQUIRES 6.10)

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
target_link_libraries(atlas_app_support
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
)
atlas_enable_warnings(atlas_app_support)

qt_add_executable(atlas_reader
    src/app/main.cpp
)

qt_add_qml_module(atlas_reader
    URI AtlasReader
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_compile_definitions(atlas_reader PRIVATE
    ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}"
)

target_link_libraries(atlas_reader
    PRIVATE
        atlas_core
        atlas_app_support
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickControls2
)
atlas_enable_warnings(atlas_reader)

# N1 Windows decision carried forward into N2: keep the measured GDI
# font-backend configuration next to every built product executable. N2
# PDF-engine probes must not casually reopen the accepted N1 shell decision.
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

# N2.1: focused Qt PDF qualification executable. This target is intentionally
# separate from the product shell and portable core so probe adoption cannot be
# mistaken for a production engine decision.
add_executable(atlas_qt_pdf_probe tools/pdf/qt_pdf_probe.cpp)
target_compile_definitions(atlas_qt_pdf_probe PRIVATE
    ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}"
)
target_link_libraries(atlas_qt_pdf_probe
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Pdf
)
atlas_enable_warnings(atlas_qt_pdf_probe)

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
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A002_geometry_labels.pdf"
)

add_test(
    NAME n2_qt_pdf_a003_outlines_links_open
    COMMAND atlas_qt_pdf_probe
        --fixture-id A003
        --expect-pages 3
        --expect-labels 1,2,3
        --expect-text-contains "Atlas N2 Fixture A003 page 1"
        "${ATLAS_N2_PDF_FIXTURE_DIR}/A003_outlines_links.pdf"
)
