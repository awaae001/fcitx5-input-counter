cmake_minimum_required(VERSION 3.21)

project(
    fcitx5-input-counter
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Fcitx5Core 5.1.2 REQUIRED)
find_package(Gettext REQUIRED)

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5Macros.cmake")

add_library(inputcounter MODULE
    src/input_counter.cpp
    src/main.cpp
)
target_link_libraries(inputcounter PRIVATE Fcitx5::Core)
target_compile_definitions(inputcounter PRIVATE
    FCITX_GETTEXT_DOMAIN="fcitx5-input-counter"
    FCITX_INSTALL_LOCALEDIR="${FCITX_INSTALL_LOCALEDIR}"
)
set_target_properties(inputcounter PROPERTIES PREFIX "")

configure_file(inputcounter.conf.in inputcounter.conf @ONLY)

install(TARGETS inputcounter DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/inputcounter.conf"
    DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
)

add_subdirectory(po)
