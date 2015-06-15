include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(app)
add_subdirectory(core)
add_subdirectory(gui)
add_subdirectory(icons)
add_subdirectory(reparentingmodel)
add_subdirectory(utils)

########### zanshin core ###############

set(zanshin_core_SRCS
    todohelpers.cpp
    ${core_SRCS}
    ${gui_SRCS}
    ${reparentingmodel_SRCS}
)

set(zanshin_core_LIBS
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDEPIMLIBS_KCALCORE_LIBS}
    ${KDEPIMLIBS_KPIMTEXTEDIT_LIBS}
    ${KDE4_KIO_LIBS}
    ${QT_QTXML_LIBRARY}
    akonadi
    domain
    oldutils
    utils
)

########### zanshin gui ###############

set(zanshin_gui_SRCS
    ${gui_SRCS}
)

qt5_wrap_ui(zanshin_gui_SRCS gui/itemeditor/itemview.ui)
qt5_wrap_ui(zanshin_gui_SRCS gui/itemeditor/properties.ui)

########### zanshin app ###############

set(zanshin_SRCS
    ${app_SRCS}
    ${mainApp_SRCS}
    ${zanshin_core_SRCS}
    ${zanshin_gui_SRCS}
)


ecm_install_icons(ICONS "${CMAKE_CURRENT_SOURCE_DIR}/icons/hi*-apps-zanshin.png" DESTINATION ${KDE_INSTALL_ICONDIR})
add_executable(zanshin ${zanshin_SRCS})
target_link_libraries(zanshin ${KDE4_KDEUI_LIBS} ${zanshin_core_LIBS})

#install(TARGETS zanshin DESTINATION ${KDE_INSTALL_BINDIR})
#install(FILES app/zanshinui.rc DESTINATION ${KDE_INSTALL_DATADIR}/zanshin)
#install(FILES gui/itemeditor/editorui.rc DESTINATION ${KDE_INSTALL_DATADIR}/zanshin)
#install(PROGRAMS app/zanshin.desktop DESTINATION ${KDE_INSTALL_APPDIR})

########### zanshin part ###############

set(zanshin_part_SRCS
    ${app_SRCS}
    ${partApp_SRCS}
    ${zanshin_core_SRCS}
    ${zanshin_gui_SRCS}
)

kde4_add_plugin(zanshin_part ${zanshin_part_SRCS})
target_link_libraries(zanshin_part ${KDE4_KPARTS_LIBS} ${zanshin_core_LIBS})

install(TARGETS zanshin_part DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES app/zanshin_part.rc DESTINATION ${KDE_INSTALL_DATADIR}/zanshin)
install(FILES app/zanshin_part.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### kontact zanshin plugin ###############

set(kontact_zanshinplugin_SRCS
    app/kontact_plugin.cpp
)

kde4_add_plugin(kontact_zanshinplugin ${kontact_zanshinplugin_SRCS})
target_link_libraries(kontact_zanshinplugin ${KDE4_KPARTS_LIBS} ${KDEPIMLIBS_KONTACTINTERFACE_LIBS})

install(TARGETS kontact_zanshinplugin DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES app/zanshin_plugin.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kontact)

########### zanshin static version ###############

if(KDE4_BUILD_TESTS)
   kde4_add_library(zanshin_static STATIC ${zanshin_core_SRCS})
   target_link_libraries(zanshin_static ${KDE4_KDEUI_LIBS} ${zanshin_core_LIBS})
   add_dependencies(zanshin_static zanshin)
endif()
