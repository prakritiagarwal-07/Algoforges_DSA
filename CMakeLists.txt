cmake_minimum_required(VERSION 3.10)
project(leetcode_oracle)

set(CMAKE_CXX_STANDARD 17)

# 1. Tell CMake where headers are
include_directories(${CMAKE_SOURCE_DIR}/include)

# 2. Link your external GLFW (Path from your screenshot)
set(GLFW_DIR "C:/Users/prakr/coding_installations/glfw-3.4.bin.WIN64")
include_directories("${GLFW_DIR}/include")
link_directories("${GLFW_DIR}/lib-mingw-w64")

# 3. Add all your source files
add_executable(${PROJECT_NAME} 
    main.cpp 
    glad.c 
    imgui.cpp 
    imgui_draw.cpp 
    imgui_tables.cpp 
    imgui_widgets.cpp 
    imgui_impl_glfw.cpp 
    imgui_impl_opengl3.cpp
)

# 4. Link Libraries
target_link_libraries(${PROJECT_NAME} glfw3 opengl32 gdi32)
# This ensures that when imgui_widgets.cpp looks for "imgui.h", 
# it checks your include folder.
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)