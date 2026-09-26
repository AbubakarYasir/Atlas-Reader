cmake_minimum_required(VERSION 3.28)
project(AtlasN2PdfFidelity LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Pdf)
find_package(PDFium REQUIRED)

add_executable(atlas_qt_pdf_fidelity_probe qt_pdf_fidelity_probe.cpp)
target_link_libraries(atlas_qt_pdf_fidelity_probe PRIVATE Qt6::Core Qt6::Gui Qt6::Pdf)

add_executable(atlas_pdfium_fidelity_probe pdfium_fidelity_probe.cpp)
target_link_libraries(atlas_pdfium_fidelity_probe PRIVATE Qt6::Core pdfium)

if(MSVC)
    target_compile_options(atlas_qt_pdf_fidelity_probe PRIVATE /W4 /permissive- /Zc:__cplusplus /WX)
    target_compile_options(atlas_pdfium_fidelity_probe PRIVATE /W4 /permissive- /Zc:__cplusplus /WX)
else()
    target_compile_options(atlas_qt_pdf_fidelity_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(atlas_pdfium_fidelity_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(WIN32)
    add_custom_command(
        TARGET atlas_pdfium_fidelity_probe
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:pdfium>
                $<TARGET_FILE_DIR:atlas_pdfium_fidelity_probe>
        VERBATIM
    )
endif()
