cmake_minimum_required(VERSION 3.28)

project(AtlasN2PdfBench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ATLAS_VERSION_STRING "2.0.0-alpha.2" CACHE STRING "Atlas version string recorded by benchmark probes")
set(ATLAS_PDFIUM_PIN "chromium/8066" CACHE STRING "PDFium qualification pin")
set(ATLAS_PDFIUM_VERSION "156.0.8066.0" CACHE STRING "PDFium qualification version")

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Pdf)
find_package(PDFium REQUIRED)

function(atlas_bench_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

add_executable(atlas_qt_pdf_bench ../qt_pdf_bench.cpp ../bench_common.h)
target_include_directories(atlas_qt_pdf_bench PRIVATE ..)
target_compile_definitions(atlas_qt_pdf_bench PRIVATE ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}")
target_link_libraries(atlas_qt_pdf_bench PRIVATE Qt6::Core Qt6::Gui Qt6::Pdf)
atlas_bench_warnings(atlas_qt_pdf_bench)

add_executable(atlas_pdfium_bench ../pdfium_bench.cpp ../bench_common.h)
target_include_directories(atlas_pdfium_bench PRIVATE ..)
target_compile_definitions(atlas_pdfium_bench PRIVATE
    ATLAS_VERSION_STRING="${ATLAS_VERSION_STRING}"
    ATLAS_PDFIUM_PIN="${ATLAS_PDFIUM_PIN}"
    ATLAS_PDFIUM_VERSION="${ATLAS_PDFIUM_VERSION}"
)
target_link_libraries(atlas_pdfium_bench PRIVATE Qt6::Core pdfium)
atlas_bench_warnings(atlas_pdfium_bench)

if(WIN32)
    target_link_libraries(atlas_qt_pdf_bench PRIVATE Psapi)
    target_link_libraries(atlas_pdfium_bench PRIVATE Psapi)

    add_custom_command(
        TARGET atlas_pdfium_bench
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:pdfium>
                $<TARGET_FILE_DIR:atlas_pdfium_bench>
        VERBATIM
    )
endif()
