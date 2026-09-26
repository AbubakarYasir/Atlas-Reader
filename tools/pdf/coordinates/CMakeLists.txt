cmake_minimum_required(VERSION 3.28)
project(AtlasN2PdfCoordinates LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Pdf)
find_package(PDFium REQUIRED)

add_executable(atlas_qt_pdf_coordinate_probe qt_pdf_coordinate_probe.cpp)
target_link_libraries(atlas_qt_pdf_coordinate_probe PRIVATE Qt6::Core Qt6::Pdf)

add_executable(atlas_pdfium_coordinate_probe pdfium_coordinate_probe.cpp)
target_compile_definitions(atlas_pdfium_coordinate_probe PRIVATE
    ATLAS_PDFIUM_PIN="chromium/8066"
    ATLAS_PDFIUM_VERSION="156.0.8066.0"
)
target_link_libraries(atlas_pdfium_coordinate_probe PRIVATE Qt6::Core pdfium)

if(MSVC)
    target_compile_options(atlas_qt_pdf_coordinate_probe PRIVATE /W4 /permissive- /Zc:__cplusplus /WX)
    target_compile_options(atlas_pdfium_coordinate_probe PRIVATE /W4 /permissive- /Zc:__cplusplus /WX)
else()
    target_compile_options(atlas_qt_pdf_coordinate_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(atlas_pdfium_coordinate_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(WIN32)
    add_custom_command(
        TARGET atlas_pdfium_coordinate_probe
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:pdfium>
                $<TARGET_FILE_DIR:atlas_pdfium_coordinate_probe>
        VERBATIM
    )
endif()
