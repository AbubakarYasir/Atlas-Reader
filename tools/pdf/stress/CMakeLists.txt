cmake_minimum_required(VERSION 3.28)
project(AtlasN2PdfStress LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Pdf)
find_package(PDFium REQUIRED)

add_executable(atlas_qt_pdf_stress qt_pdf_stress.cpp)
target_link_libraries(atlas_qt_pdf_stress PRIVATE Qt6::Core Qt6::Gui Qt6::Pdf Psapi)

add_executable(atlas_pdfium_stress pdfium_stress.cpp)
target_compile_definitions(atlas_pdfium_stress PRIVATE
    ATLAS_PDFIUM_PIN="chromium/8066"
    ATLAS_PDFIUM_VERSION="156.0.8066.0"
)
target_link_libraries(atlas_pdfium_stress PRIVATE Qt6::Core pdfium Psapi)

if(MSVC)
    target_compile_options(atlas_qt_pdf_stress PRIVATE /W4 /permissive- /Zc:__cplusplus /WX)
    target_compile_options(atlas_pdfium_stress PRIVATE /W4 /permissive- /Zc:__cplusplus /WX)
else()
    target_compile_options(atlas_qt_pdf_stress PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(atlas_pdfium_stress PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(WIN32)
    add_custom_command(
        TARGET atlas_pdfium_stress
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:pdfium>
                $<TARGET_FILE_DIR:atlas_pdfium_stress>
        VERBATIM
    )
endif()
