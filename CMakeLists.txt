cmake_minimum_required(VERSION 3.20)
project(sotto VERSION 1.2.1 DESCRIPTION "Voice typing for Linux with whisper.cpp" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(GNUInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Multimedia Network DBus)
find_package(whisper REQUIRED)
find_package(LayerShellQt QUIET)

add_executable(sotto
    src/main.cpp
    src/app.cpp
    src/config.cpp
    src/i18n.cpp
    src/models.cpp
    src/output.cpp
    src/overlay.cpp
    src/portal.cpp
    src/recorder.cpp
    src/settingsdialog.cpp
    src/shortcut.cpp
    src/transcriber.cpp
    src/uninstaller.cpp
    data/resources.qrc
)

target_compile_definitions(sotto PRIVATE SOTTO_VERSION="${PROJECT_VERSION}" QT_NO_CAST_FROM_BYTEARRAY)
option(SOTTO_WERROR "Treat warnings as errors" OFF)
target_compile_options(sotto PRIVATE -Wall -Wextra -Wpedantic -fstack-protector-strong $<$<BOOL:${SOTTO_WERROR}>:-Werror>)
# Distribution flags (Fedora, Arch) usually bring their own _FORTIFY_SOURCE.
if(NOT CMAKE_CXX_FLAGS MATCHES "_FORTIFY_SOURCE")
    target_compile_options(sotto PRIVATE $<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=3>)
endif()
target_link_options(sotto PRIVATE -Wl,-z,relro,-z,now -Wl,-z,noexecstack)

# Packages that bundle their own whisper.cpp (Debian, Fedora) keep its ggml
# backends in a private folder: -DSOTTO_GGML_BACKEND_DIR=/usr/lib/sotto/ggml
set(SOTTO_GGML_BACKEND_DIR "" CACHE PATH "Folder with the ggml backend libraries (empty: <prefix>/lib/ggml or ggml's default)")
if(SOTTO_GGML_BACKEND_DIR)
    target_compile_definitions(sotto PRIVATE SOTTO_GGML_BACKEND_DIR="${SOTTO_GGML_BACKEND_DIR}")
endif()

target_link_libraries(sotto PRIVATE
    Qt6::Widgets Qt6::Multimedia Qt6::Network Qt6::DBus
    whisper
)

if(LayerShellQt_FOUND)
    target_link_libraries(sotto PRIVATE LayerShellQt::Interface)
    target_compile_definitions(sotto PRIVATE SOTTO_HAVE_LAYERSHELL)
    message(STATUS "LayerShellQt found: overlay will use wlr-layer-shell on Wayland")
else()
    message(STATUS "LayerShellQt not found: overlay falls back to a regular window")
endif()

install(TARGETS sotto RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/io.github.trendorin.Sotto.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/io.github.trendorin.Sotto.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES data/io.github.trendorin.Sotto.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/sotto)
