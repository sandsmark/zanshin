set(testlib_SRCS
   akonadidebug.cpp
   akonadifakedata.cpp
   akonadifakejobs.cpp
   akonadifakemonitor.cpp
   fakejob.cpp
   gencollection.cpp
   gennote.cpp
   gentag.cpp
   gentodo.cpp
   modeltest.cpp
   monitorspy.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/tests ${CMAKE_SOURCE_DIR}/src)
add_library(testlib STATIC ${testlib_SRCS})
target_link_libraries(testlib
    KF5::CalendarCore
    KF5::Mime
    KF5::AkonadiCore
    Qt5::Test)

