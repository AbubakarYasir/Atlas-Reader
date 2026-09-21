cmake_minimum_required(VERSION 3.28)

project(
    AtlasReaderNative
    VERSION 2.0.0
    DESCRIPTION "Native successor to Atlas Reader"
    LANGUAGES CXX
)

set(ATLAS_PRERELEASE "alpha.0" CACHE STRING "Atlas prerelease identifier")
set(ATLAS_VERSION_STRING "${PROJECT_VERSION}-${ATLAS_PRERELEASE}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Exact Qt patch selection belongs to the reproducible toolchain/CI pin, not to
# application source. The bootstrap currently uses only APIs available in Qt
# 6.10+, while N1 owns convergence on the canonical 6.11.x product patch.
find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2)
qt_standard_project_setup(REQUIRES 6.10)

add_library(atlas_core INTERFACE)
target_include_directories(atlas_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(atlas_core INTERFACE cxx_std_23)

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
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickControls2
)

if(MSVC)
    target_compile_options(atlas_reader PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(atlas_reader PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(atlas_core_smoke tests/core_smoke.cpp)
target_link_libraries(atlas_core_smoke PRIVATE atlas_core)

if(MSVC)
    target_compile_options(atlas_core_smoke PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(atlas_core_smoke PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME atlas_core_smoke COMMAND atlas_core_smoke)
