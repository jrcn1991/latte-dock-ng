add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.latte.plasmoid\")

plasma_install_package(package org.kde.latte.plasmoid)

find_package(PlasmaActivitiesStats REQUIRED)

set(tasks_SRCS
    plugin/windowviewbackend.cpp
    plugin/windowviewbackend.h
    plugin/contextmenuactionsbackend.cpp
    plugin/contextmenuactionsbackend.h
    plugin/types.cpp
    plugin/types.h
)

# Task types and backends are registered declaratively via
# QML_NAMED_ELEMENT; qt_add_qml_module generates the plugin class, the
# registration code and the qmldir installed below.
set(LATTE_TASKS_QML_BUILD_DIR ${CMAKE_BINARY_DIR}/qml/org/kde/latte/private/tasks)

qt_add_qml_module(lattetasksplugin
    URI org.kde.latte.private.tasks
    VERSION 0.1
    CLASS_NAME LatteTasksPlugin
    PLUGIN_TARGET lattetasksplugin
    NO_PLUGIN_OPTIONAL
    OUTPUT_DIRECTORY ${LATTE_TASKS_QML_BUILD_DIR}
    SOURCES ${tasks_SRCS}
)

# QML's Unix plugin loader expects the conventional lib-prefixed filename.
set_target_properties(lattetasksplugin PROPERTIES
    PREFIX "lib"
)

target_link_libraries(lattetasksplugin PRIVATE
                      Qt6::DBus
                      Qt6::Core
                      Qt6::Gui
                      Qt6::Qml
                      Qt6::Widgets
                      KF6::I18n
                      KF6::KIOCore
                      KF6::KIOFileWidgets
                      KF6::KIOWidgets
                      Plasma::ActivitiesStats
                      Plasma::Plasma
                      Plasma::PlasmaQuick)

# The generated qmltyperegistrations.cpp resolves the module headers via
# __has_include(<types.h>), so the plugin/ header directory must be on the
# target include path.
target_include_directories(lattetasksplugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/plugin)

install(TARGETS lattetasksplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks)
install(FILES ${LATTE_TASKS_QML_BUILD_DIR}/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks)
# Tooling data for qmllint/Qt Creator; generated together with the qmldir.
install(FILES ${LATTE_TASKS_QML_BUILD_DIR}/lattetasksplugin.qmltypes
        DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/latte/private/tasks
        OPTIONAL)

if(LATTE_QT6_COMPAT_QMLDIR)
    install(TARGETS lattetasksplugin DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/tasks)
    install(FILES ${LATTE_TASKS_QML_BUILD_DIR}/qmldir DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/tasks)
    install(FILES ${LATTE_TASKS_QML_BUILD_DIR}/lattetasksplugin.qmltypes
            DESTINATION ${LATTE_QT6_COMPAT_QMLDIR}/org/kde/latte/private/tasks
            OPTIONAL)
endif()
