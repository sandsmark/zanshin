set(part_SRCS
    ../app/aboutdata.cpp
    ../app/dependencies.cpp
    part.cpp
)

add_library(zanshin-next_part ${part_SRCS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
target_link_libraries(zanshin-next_part
    KF5::Parts
    akonadi
    domain
    presentation
    scripting
    utils
    widgets
)

install(TARGETS zanshin-next_part DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES zanshin-next_part.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

##########################

set(kontact_SRCS
    kontact_plugin.cpp
)

add_library(kontact_zanshinnextplugin ${kontact_SRCS})
target_link_libraries(kontact_zanshinnextplugin
    KF5::Parts
    KF5::KontactInterface
    )

install(TARGETS kontact_zanshinnextplugin DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES zanshin-next_plugin.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kontact)
install(FILES zanshin-next_part.rc DESTINATION ${KDE_INSTALL_DATADIR}/zanshin)

