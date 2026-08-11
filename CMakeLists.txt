cmake_minimum_required(VERSION 3.21)

project(
    fcitx5-input-counter
    VERSION 0.2.1
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

find_package(Fcitx5Core 5.1.2 REQUIRED)
find_package(Fcitx5ModuleDBus REQUIRED)
find_package(Gettext REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS DBus Widgets)

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5Macros.cmake")

set(INPUT_COUNTER_VIEWER_PATH
    "${FCITX_INSTALL_BINDIR}/fcitx5-input-counter-viewer")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql" INPUT_COUNTER_SCHEMA_SQL)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql")
configure_file(src/embedded_sql.h.in embedded_sql.h @ONLY)

add_library(inputcounter MODULE
    src/database_manager.cpp
    src/hourly_count_buffer.cpp
    src/input_counter.cpp
    src/input_counter_dbus.cpp
    src/input_counter_settings.cpp
    src/main.cpp
    src/quick_counter.cpp
    src/statistics_backend.cpp
)
target_include_directories(inputcounter PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(inputcounter PRIVATE
    Fcitx5::Core
    Fcitx5::Module::DBus
    SQLite3::SQLite3
)
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
    src/viewer/bar_chart_widget.cpp
    src/viewer/main.cpp
    src/viewer/main_window.cpp
    src/viewer/main_window_ui.cpp
    src/viewer/statistics_client.cpp
    src/viewer/statistics_snapshot.cpp
)
target_include_directories(fcitx5-input-counter-viewer PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(fcitx5-input-counter-viewer PRIVATE
    Qt6::DBus
    Qt6::Widgets
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
    add_executable(hourly-count-buffer-test
        src/database_manager.cpp
        src/hourly_count_buffer.cpp
        tests/hourly_count_buffer_test.cpp
    )
    target_include_directories(hourly-count-buffer-test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        src
    )
    target_link_libraries(hourly-count-buffer-test PRIVATE SQLite3::SQLite3)
    add_test(NAME hourly-count-buffer-test COMMAND hourly-count-buffer-test)

    add_executable(statistics-backend-test
        src/database_manager.cpp
        src/hourly_count_buffer.cpp
        src/statistics_backend.cpp
        tests/statistics_backend_test.cpp
    )
    target_include_directories(statistics-backend-test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        src
    )
    target_link_libraries(statistics-backend-test PRIVATE SQLite3::SQLite3)
    add_test(NAME statistics-backend-test COMMAND statistics-backend-test)

    add_executable(statistics-snapshot-test
        src/viewer/chart_range.cpp
        src/viewer/statistics_snapshot.cpp
        tests/statistics_snapshot_test.cpp
    )
    target_include_directories(statistics-snapshot-test PRIVATE
        src
        src/viewer
    )
    target_link_libraries(statistics-snapshot-test PRIVATE Qt6::Core)
    add_test(NAME statistics-snapshot-test COMMAND statistics-snapshot-test)
endif()

add_subdirectory(po)
