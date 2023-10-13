cmake_minimum_required(VERSION 3.0)
project(rtsp_server_sample)

# Find the required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_RTSP_SERVER REQUIRED gstreamer-rtsp-server-1.0)

# Set the source files
set(SOURCE test-rtsp-video-server.cpp)

# Set the include directories
include_directories(${GST_INCLUDE_DIRS} ${GST_RTSP_SERVER_INCLUDE_DIRS})

# Set the compiler flags
add_definitions(${GST_CFLAGS_OTHER} ${GST_RTSP_SERVER_CFLAGS_OTHER})

# Set the linker flags
link_directories(${GST_LIBRARY_DIRS} ${GST_RTSP_SERVER_LIBRARY_DIRS})
link_libraries(${GST_LIBRARIES} ${GST_RTSP_SERVER_LIBRARIES})

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCE})

# Link the required libraries
target_link_libraries(${PROJECT_NAME} ${GST_LIBRARIES} ${GST_RTSP_SERVER_LIBRARIES})

