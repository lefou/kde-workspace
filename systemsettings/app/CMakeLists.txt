
include_directories( ToolTips )

set( systemsettings_SRCS
     SystemSettingsApp.cpp
     SettingsBase.cpp
     ToolTips/ktooltip.cpp
     ToolTips/ktooltipwindow.cpp
     ToolTips/tooltipmanager.cpp
     main.cpp )

kde4_add_app_icon( systemsettings_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/categories/preferences-system.png" )
kde4_add_ui_files( systemsettings_SRCS configDialog.ui )
kconfig_add_kcfg_files( systemsettings_SRCS BaseConfig.kcfgc )
qt5_add_dbus_adaptor( systemsettings_SRCS org.kde.systemsettings.xml SystemSettingsApp.h SystemSettingsApp  )

add_executable( systemsettings ${systemsettings_SRCS})
target_compile_definitions(systemsettings PRIVATE -DSYSTEMSETTINGS_STRING_VERSION="${SYSTEMSETTINGS_STRING_VERSION}")

target_link_libraries( systemsettings systemsettingsview
    KF5::ItemViews
    KF5::KCMUtils
    KF5::I18n
    KF5::IconThemes
    KF5::KIOWidgets
    KF5::Service
    KF5::WindowSystem
    KF5::XmlGui
    KF5::DBusAddons
    KF5::ConfigGui
)

install( TARGETS systemsettings ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES systemsettingsui.rc systemsettings.kcfg DESTINATION ${DATA_INSTALL_DIR}/systemsettings )
install( PROGRAMS kdesystemsettings.desktop systemsettings.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
