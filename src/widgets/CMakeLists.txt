set(widgets_SRCS
    applicationcomponents.cpp
    availablepagesview.cpp
    availablesourcesview.cpp
    datasourcecombobox.cpp
    datasourcedelegate.cpp
    editorview.cpp
    filterwidget.cpp
    itemdelegate.cpp
    messagebox.cpp
    messageboxinterface.cpp
    newpagedialog.cpp
    newpagedialoginterface.cpp
    pageview.cpp
    scripteditor.cpp
)

qt5_wrap_ui(widgets_SRCS
    filterwidget.ui
    newpagedialog.ui
)

add_library(widgets STATIC ${widgets_SRCS})
target_link_libraries(widgets Qt5::Widgets Qt5::Script KF5::Completion KF5::KDELibs4Support KF5::WidgetsAddons presentation zanshinkdepimstatic)
