PROJECT (Util)
cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 20)
FILE(GLOB LIB_SRC "*.cpp" "*.c" EXCLUDE "*/main.cpp")
include_directories("../Math/inc")
include_directories("../Mesh/inc")
if (NOT TARGET Mesh)
  message(STATUS "add mesh lib")
  add_subdirectory("../Mesh" "Mesh")
endif()

message("Util lib src ${LIB_SRC}")
add_library("Util" STATIC ${LIB_SRC})
