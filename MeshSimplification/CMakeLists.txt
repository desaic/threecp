file(GLOB_RECURSE SRC "*.cpp" "*.h")
set(CMAKE_CXX_STANDARD 20)
include_directories("../Math/inc")
add_library(MeshSimplification ${SRC})
