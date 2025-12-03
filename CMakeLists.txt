cmake_minimum_required(VERSION 3.16)
project(HypWorkCut LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

add_executable(${PROJECT_NAME} hypworkcut.cpp)

target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets)