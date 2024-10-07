cmake_minimum_required(VERSION 3.5)

set(IDF_TARGET "esp32s3")  # Add this line

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(netstick-test)

# Add components directory
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/components")
