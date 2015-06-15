set(app_SRCS
    aboutdata.cpp
    dependencies.cpp
    main.cpp
)

add_executable(zanshin-next ${app_SRCS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ECM_ENABLE_EXCEPTIONS}")
target_link_libraries(zanshin-next
    KF5::KDELibs4Support
    KF5::I18n
    KF5::KontactInterface

    akonadi
    domain
    presentation
    scripting
    utils
    widgets
)

install(TARGETS zanshin-next DESTINATION ${KDE_INSTALL_BINDIR})
install(PROGRAMS zanshin-next.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES zanshin.appdata.xml DESTINATION share/appdata)
