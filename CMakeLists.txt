# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CCACHE_ENABLE 1)
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(OS)
