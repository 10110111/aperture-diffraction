cmake_minimum_required(VERSION 3.10.0)
project(ApertureDiffraction VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type -Wall -Wextra")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(ENABLE_QT6 1 CACHE BOOL "Whether to try using Qt6. If Qt6 isn't found, Qt5 will be used.")
if(ENABLE_QT6)
    find_package(Qt6 COMPONENTS Core OpenGL Concurrent Widgets OpenGLWidgets QUIET)
endif()
if(Qt6_FOUND)
    if(NOT DEFINED QT_VERSION_MAJOR)
        set(QT_VERSION_MAJOR 6)
    endif()
    set(QT_EXTRA_LIBS Qt${QT_VERSION_MAJOR}::OpenGLWidgets)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core OpenGL Concurrent Widgets)
    if(NOT DEFINED QT_VERSION_MAJOR)
        set(QT_VERSION_MAJOR 5)
    endif()
endif()
get_target_property(QMAKE_LOCATION Qt${QT_VERSION_MAJOR}::qmake LOCATION)
if(NOT DEFINED QT_VERSION)
    execute_process(COMMAND ${QMAKE_LOCATION} -query QT_VERSION OUTPUT_VARIABLE QT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
message(STATUS "Qt version: ${QT_VERSION}")

include(CheckIncludeFileCXX)
check_include_file_cxx("glm/glm.hpp" HAVE_GLM)
if(NOT HAVE_GLM)
    unset(HAVE_GLM CACHE)
    message(FATAL_ERROR "GLM was not found")
endif()

if(QT_VERSION_MAJOR EQUAL 5)
    qt5_add_resources(RES_SOURCES resources.qrc)
else()
    qt_add_resources(RES_SOURCES resources.qrc)
endif()
add_executable(aperdiff
                ${RES_SOURCES}
                main.cpp
                Canvas.cpp
                common.cpp
                ToolsWidget.cpp
                Manipulator.cpp
                ApertureOutline.cpp
                GLSLCosineQualityChecker.cpp
              )
target_link_libraries(aperdiff
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    ${QT_EXTRA_LIBS})
