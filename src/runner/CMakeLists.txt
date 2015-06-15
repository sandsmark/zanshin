set(krunner_zanshin_SRCS
    zanshinrunner.cpp
)

add_library(krunner_zanshin MODULE ${krunner_zanshin_SRCS})
target_link_libraries(krunner_zanshin
    KF5::Plasma
    KF5::Runner
    akonadi
    domain
    utils
)

install(TARGETS krunner_zanshin DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-runner-zanshin.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

