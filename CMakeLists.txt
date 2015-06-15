project(zanshin)
cmake_minimum_required(VERSION 2.8.12)

find_package(ECM REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

find_package(Boost REQUIRED)
include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
include(FeatureSummary)
include(ECMInstallIcons)
include(ECMMarkAsTest)

find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Core Gui Widgets Script Test)


#macro_log_feature(KDEPIMLIBS_FOUND "kdepimlibs" "KDE PIM libraries" "The KDE PIM libs are required to build Zanshin" "http://www.kde.org")

macro(assert_min_ver version)
    set(error_msg "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION} not supported")
    if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "${version}")
        message(FATAL_ERROR "${msg}")
    endif()
endmacro()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang >= 3.3
    assert_min_ver("3.3")
    add_definitions(-DQ_DECL_OVERRIDE=override)
    add_definitions(-DQ_DECL_FINAL=final)
    add_definitions(-DQ_NULLPTR=nullptr)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC >= 4.8
    assert_min_ver("4.8")
    add_definitions(-DQ_DECL_OVERRIDE=override)
    add_definitions(-DQ_DECL_FINAL=final)
    add_definitions(-DQ_NULLPTR=nullptr)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # currently Intel compiler does not support all required C++11 features in zanshin
    # http://software.intel.com/en-us/articles/c0x-features-supported-by-intel-c-compiler
    assert_min_ver("15.0")
    add_definitions(-DQ_DECL_OVERRIDE=)
    add_definitions(-DQ_DECL_FINAL=)
    add_definitions(-DQ_NULLPTR=0)
else()
    add_definitions(-DQ_DECL_OVERRIDE=)
    add_definitions(-DQ_DECL_FINAL=)
    add_definitions(-DQ_NULLPTR=0)
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS} -fPIC")

option(ZANSHIN_BUILD_COVERAGE "Build Zanshin with gcov support" OFF)
if(ZANSHIN_BUILD_COVERAGE AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
include_directories (
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    
    ${KDEPIMLIBS_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
    3rdparty/kdepim/
    3rdparty/kdepim/libkdepim/
)
#find_package(KF5AkonadiCalendar CONFIG REQUIRED)

find_package(KF5 REQUIRED COMPONENTS
    Akonadi
    AkonadiCalendar
    AkonadiNotes
    AkonadiSearch
    CalendarCore
    Completion
    Contacts
    CoreAddons
    DocTools
    I18n
    IconThemes
    IdentityManagement
    KCMUtils
    KDELibs4Support
    KHtml
    KIO
    KontactInterface
    Ldap
    Mime
    Parts
    Parts
    PimTextEdit
    Plasma
    Runner
    WidgetsAddons
    )

add_subdirectory(3rdparty)
#add_subdirectory(attic)
add_subdirectory(src)
add_subdirectory(tests)

