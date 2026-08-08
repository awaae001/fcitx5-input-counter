cmake_minimum_required(VERSION 3.21)

project(
    fcitx5-input-counter
    VERSION 0.2.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

find_package(Fcitx5Core 5.1.2 REQUIRED)
find_package(Gettext REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5Macros.cmake")

set(INPUT_COUNTER_VIEWER_PATH
    "${FCITX_INSTALL_BINDIR}/fcitx5-input-counter-viewer")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql" INPUT_COUNTER_SCHEMA_SQL)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql")
configure_file(src/embedded_sql.h.in embedded_sql.h @ONLY)

add_library(inputcounter MODULE
    src/input_counter.cpp
    src/main.cpp
    src/stats_db.cpp
)
target_include_directories(inputcounter PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(inputcounter PRIVATE Fcitx5::Core SQLite3::SQLite3)
target_compile_definitions(inputcounter PRIVATE
    FCITX_GETTEXT_DOMAIN="fcitx5-input-counter"
    FCITX_INSTALL_LOCALEDIR="${FCITX_INSTALL_LOCALEDIR}"
    INPUT_COUNTER_VIEWER_PATH="${INPUT_COUNTER_VIEWER_PATH}"
)
set_target_properties(inputcounter PROPERTIES PREFIX "")

add_executable(fcitx5-input-counter-viewer
    src/viewer/bar_chart_renderer.cpp
    src/viewer/chart_range.cpp
    src/viewer/custom_range_dialog.cpp
    src/stats_db.cpp
    src/viewer/bar_chart_widget.cpp
    src/viewer/main.cpp
    src/viewer/main_window.cpp
    src/viewer/main_window_ui.cpp
    src/viewer/statistics_snapshot.cpp
)
target_include_directories(fcitx5-input-counter-viewer PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(fcitx5-input-counter-viewer PRIVATE
    Qt6::Widgets
    SQLite3::SQLite3
)
target_compile_definitions(fcitx5-input-counter-viewer PRIVATE
    INPUT_COUNTER_LOCALEDIR="${FCITX_INSTALL_LOCALEDIR}"
)

configure_file(inputcounter.conf.in inputcounter.conf @ONLY)

install(TARGETS inputcounter DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")
install(TARGETS fcitx5-input-counter-viewer
    DESTINATION "${FCITX_INSTALL_BINDIR}"
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/inputcounter.conf"
    DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
)

# Desktop entry + icon so Wayland compositors can resolve the viewer's
# window icon via its app id (fcitx5-input-counter-viewer).
install(
    FILES data/fcitx5-input-counter-viewer.desktop
    DESTINATION "${FCITX_INSTALL_DATADIR}/applications"
)
install(
    FILES data/fcitx5-input-counter.svg
    DESTINATION "${FCITX_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(chart-range-test
        src/viewer/chart_range.cpp
        tests/chart_range_test.cpp
    )
    target_include_directories(chart-range-test PRIVATE src/viewer)
    target_link_libraries(chart-range-test PRIVATE Qt6::Core)
    add_test(NAME chart-range-test COMMAND chart-range-test)
endif()

add_subdirectory(po)
