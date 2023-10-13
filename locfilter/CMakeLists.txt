cmake_minimum_required(VERSION 3.11)

project(gstlocfilter)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 17) 

include_directories(${GST_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})

add_library(${PROJECT_NAME} SHARED gstlocfilter.cpp)

target_link_libraries(${PROJECT_NAME} ${GST_LIBRARIES} ${OpenCV_LIBS} gstbase-1.0 gstvideo-1.0)
