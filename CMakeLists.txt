cmake_minimum_required(VERSION 3.10)
project(rhombus)

find_package(OpenCV REQUIRED)

add_executable(rhombus src/rhombus.cpp)
target_include_directories(rhombus PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(rhombus PRIVATE ${OpenCV_LIBS})

