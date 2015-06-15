set(presentation_SRCS
    applicationmodel.cpp
    artifacteditormodel.cpp
    artifactfilterproxymodel.cpp
    availablepagessortfilterproxymodel.cpp
    availablepagesmodel.cpp
    availablesourcesmodel.cpp
    contextpagemodel.cpp
    datasourcelistmodel.cpp
    errorhandler.cpp
    errorhandlingmodelbase.cpp
    inboxpagemodel.cpp
    metatypes.cpp
    pagemodel.cpp
    projectpagemodel.cpp
    querytreemodelbase.cpp
    tagpagemodel.cpp
    tasklistmodel.cpp
    workdaypagemodel.cpp
)

add_library(presentation STATIC ${presentation_SRCS})
target_link_libraries(presentation Qt5::Core Qt5::Gui domain utils)
