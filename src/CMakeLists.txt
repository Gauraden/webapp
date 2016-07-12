cmake_minimum_required(VERSION 2.6)
project(vbr_webapp)
find_package(PkgConfig)

set(CMAKE_INSTALL_PREFIX   "${OUTPUT_DIR}")

include_directories("back-end")

add_subdirectory("back-end")
add_subdirectory("test")