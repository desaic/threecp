PROJECT (Viewer)
cmake_minimum_required(VERSION 2.8.11)

#Qt 
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON) 
find_package(Qt5Widgets)

include_directories(include)
include_directories(ui)

#for ui_mainwindow.h
#INCLUDE_DIRECTORIES(build-Debug)
FILE(GLOB EXE_SRC "ui/*.cpp" "ui/*.c")
add_executable(viewer ${EXE_SRC} )

target_link_libraries(viewer Qt5::Widgets)
