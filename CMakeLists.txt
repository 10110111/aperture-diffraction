cmake_minimum_required(VERSION 3.10.0)
project(ApertureDiffraction VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type -Wall -Wextra")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 5.9 REQUIRED Core OpenGL Concurrent)

include(CheckIncludeFileCXX)
check_include_file_cxx("glm/glm.hpp" HAVE_GLM)
if(NOT HAVE_GLM)
    unset(HAVE_GLM CACHE)
    message(FATAL_ERROR "GLM was not found")
endif()

add_executable(aperdiff
                main.cpp
                Canvas.cpp
                common.cpp
                ToolsWidget.cpp
                Manipulator.cpp
                ApertureOutline.cpp
              )
target_link_libraries(aperdiff Qt5::Core Qt5::OpenGL Qt5::Concurrent)
