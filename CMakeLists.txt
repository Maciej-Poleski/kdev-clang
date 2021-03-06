project(kdev-clang)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 90)
set(VERSION_PATCH 90)
# KDevplatform dependency version
set( KDEVPLATFORM_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

cmake_minimum_required(VERSION 2.8.12)
find_package (ECM 0.0.9 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(GenerateExportHeader)
include(ECMAddTests)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

find_package(Qt5 REQUIRED Core Widgets Test)
find_package(KF5 REQUIRED COMPONENTS I18n ItemModels ThreadWeaver TextEditor)
find_package(KDevPlatform ${KDEVPLATFORM_VERSION} REQUIRED)
find_package(KDevelop REQUIRED)

find_package(Clang 3.5)
set_package_properties(Clang PROPERTIES
    TYPE REQUIRED
    PURPOSE "LibClang is a high level C interface to clang"
    URL "http://clang.llvm.org/")

if(LLVM_VERSION MATCHES "^3\\.6\\..*$") # expected version 3.6.*
# NOTE: This checks HOST, not TARGET. It may misbehave when cross-compiling.
# Only because of dependency on ncurses, but this dependency is pulled in by Clang, which works
# somehow on Windows too.
if(CMAKE_HOST_UNIX)
    set(BUILD_REFACTORINGS ON)
endif()
endif()

add_definitions(${LLVM_CFLAGS})
include_directories(
    ${CLANG_INCLUDE_DIR}
)

# Turn off missing-field-initializers warning for GCC to avoid noise from false positives with empty {}
# See discussion: http://mail.kde.org/pipermail/kdevelop-devel/2014-February/046910.html
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h"
  @ONLY
)

enable_testing()

include_directories(
    ${KDEVELOP_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

add_subdirectory(tests)

add_definitions(
    -DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY
)

add_subdirectory(clangsettings)
add_subdirectory(duchain)
add_subdirectory(codecompletion)
add_subdirectory(codegen)
add_subdirectory(util)

if(${BUILD_REFACTORINGS})
    add_definitions(-DBUILD_REFACTORINGS)
    add_subdirectory(refactoring)
endif()

set(kdevclangsupport_SRCS
    clangparsejob.cpp
    clangsupport.cpp
    clanghighlighting.cpp
)

kdevplatform_add_plugin(kdevclangsupport JSON kdevclangsupport.json SOURCES ${kdevclangsupport_SRCS})

set(LINK_LIBS
    kdevclangduchain
    kdevclangcodecompletion
    kdevclangcodegen
    kdevclangutil
    sessionsettings

    KF5::ThreadWeaver
    KF5::TextEditor
    KDev::Util
    KDev::Project)

if(${BUILD_REFACTORINGS})
    list(APPEND LINK_LIBS kdevclangrefactor)
endif()

target_link_libraries(kdevclangsupport
    ${LINK_LIBS}
)

install(FILES kdevclangsupport.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kdevclangsupport)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
