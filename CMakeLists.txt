cmake_minimum_required(VERSION 3.20)
project(sotto VERSION 2.0.0 DESCRIPTION "Voice typing for Linux: GigaAM, Parakeet and Whisper, live as you speak" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(GNUInstallDirs)

option(SOTTO_WERROR "Treat warnings as errors" OFF)
option(SOTTO_BUILD_TESTS "Build the tests (ctest)" OFF)
option(SOTTO_REQUIRE_SHERPA "Fail when sherpa-onnx is not found instead of building without it" OFF)
# Packages that bundle their own whisper.cpp (Debian, Fedora) keep its ggml backends in a private folder.
set(SOTTO_GGML_BACKEND_DIR "" CACHE PATH "Folder with the ggml backend libraries (empty: <prefix>/lib/ggml or ggml's default)")
# sherpa-onnx (and the ONNX Runtime it brings) as packaging/fetch-sherpa.sh unpacks it: include/ and lib/.
set(SOTTO_SHERPA_DIR "" CACHE PATH "Folder with sherpa-onnx (include/sherpa-onnx/c-api/c-api.h, lib/libsherpa-onnx-c-api.so)")

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Multimedia Network DBus)
find_package(whisper REQUIRED)
find_package(LayerShellQt QUIET)
find_path(SHERPA_ONNX_INCLUDE_DIR sherpa-onnx/c-api/c-api.h HINTS "${SOTTO_SHERPA_DIR}/include")
find_library(SHERPA_ONNX_LIBRARY sherpa-onnx-c-api HINTS "${SOTTO_SHERPA_DIR}/lib")

set(SOTTO_SOURCES
    src/main.cpp
    src/cli.cpp
    src/core/config.cpp
    src/core/endpointer.cpp
    src/core/engine.cpp
    src/core/gpu.cpp
    src/core/i18n.cpp
    src/core/listener.cpp
    src/core/models.cpp
    src/core/recorder.cpp
    src/core/segmenter.cpp
    src/core/transcriber.cpp
    src/core/whisperengine.cpp
    src/gui/app.cpp
    src/gui/overlay.cpp
    src/gui/settingsdialog.cpp
    src/gui/uninstaller.cpp
    src/system/desktop.cpp
    src/system/output.cpp
    src/system/portal.cpp
    src/system/shortcut.cpp
    resources/sotto.qrc
)

add_executable(sotto ${SOTTO_SOURCES})
target_include_directories(sotto PRIVATE src)

target_compile_definitions(sotto PRIVATE SOTTO_VERSION="${PROJECT_VERSION}" QT_NO_CAST_FROM_BYTEARRAY)
target_compile_options(sotto PRIVATE -Wall -Wextra -Wpedantic -fstack-protector-strong $<$<BOOL:${SOTTO_WERROR}>:-Werror>)
# Distribution flags (Fedora, Arch) usually bring their own _FORTIFY_SOURCE.
if(NOT CMAKE_CXX_FLAGS MATCHES "_FORTIFY_SOURCE")
    target_compile_options(sotto PRIVATE $<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=3>)
endif()
target_link_options(sotto PRIVATE -Wl,-z,relro,-z,now -Wl,-z,noexecstack)
if(SOTTO_GGML_BACKEND_DIR)
    target_compile_definitions(sotto PRIVATE SOTTO_GGML_BACKEND_DIR="${SOTTO_GGML_BACKEND_DIR}")
endif()

target_link_libraries(sotto PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::Network Qt6::DBus whisper)

if(SHERPA_ONNX_INCLUDE_DIR AND SHERPA_ONNX_LIBRARY)
    target_sources(sotto PRIVATE src/core/sherpaengine.cpp)
    target_include_directories(sotto PRIVATE "${SHERPA_ONNX_INCLUDE_DIR}")
    target_link_libraries(sotto PRIVATE "${SHERPA_ONNX_LIBRARY}")
    target_compile_definitions(sotto PRIVATE SOTTO_HAVE_SHERPA)
    message(STATUS "sherpa-onnx found: GigaAM and Parakeet models (${SHERPA_ONNX_LIBRARY})")
elseif(SOTTO_REQUIRE_SHERPA)
    message(FATAL_ERROR "sherpa-onnx not found: run packaging/fetch-sherpa.sh and pass -DSOTTO_SHERPA_DIR=<its folder>")
else()
    message(STATUS "sherpa-onnx not found: Whisper models only (packaging/fetch-sherpa.sh gets it)")
endif()

if(LayerShellQt_FOUND)
    target_link_libraries(sotto PRIVATE LayerShellQt::Interface)
    target_compile_definitions(sotto PRIVATE SOTTO_HAVE_LAYERSHELL)
    message(STATUS "LayerShellQt found: overlay will use wlr-layer-shell on Wayland")
else()
    message(STATUS "LayerShellQt not found: overlay falls back to a regular window")
endif()

install(TARGETS sotto RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/io.github.trendorin.Sotto.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/io.github.trendorin.Sotto.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES packaging/io.github.trendorin.Sotto.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/sotto)

if(SOTTO_BUILD_TESTS)
    enable_testing()
    add_executable(sotto-test-endpointer tests/test_endpointer.cpp src/core/endpointer.cpp)
    target_include_directories(sotto-test-endpointer PRIVATE src)
    target_link_libraries(sotto-test-endpointer PRIVATE Qt6::Core)
    set_target_properties(sotto-test-endpointer PROPERTIES AUTOMOC OFF AUTORCC OFF)
    add_test(NAME endpointer COMMAND sotto-test-endpointer)
    add_test(NAME cli-version COMMAND sotto --version)
    set_tests_properties(cli-version PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(NAME translations COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/check-translations.py ${CMAKE_SOURCE_DIR}/src)
    endif()
endif()
