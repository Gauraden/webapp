cmake_minimum_required(VERSION 2.6)
project(webapp)
find_package(PkgConfig)

set(CMAKE_INSTALL_PREFIX   "${OUTPUT_DIR}")
add_definitions(
  --std=gnu++0x
)

include_directories("back-end")

add_subdirectory("back-end")
add_subdirectory("server")
add_subdirectory("test")