cmake_minimum_required(VERSION 2.6)
project(webapp)
find_package(PkgConfig)

set(CMAKE_INSTALL_PREFIX   "${OUTPUT_DIR}")

add_definitions(
  --std=gnu++0x
  -Wunused-parameter
  -Wnon-virtual-dtor
  -Wignored-qualifiers
  -Wmissing-declarations
  -Wsign-compare
  -Wreturn-type
)

include_directories("back-end")

add_subdirectory("back-end")
add_subdirectory("front-end")
add_subdirectory("server")
add_subdirectory("test")
add_subdirectory("third-party")
