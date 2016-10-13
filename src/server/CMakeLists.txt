add_executable(webapp_server
  webapp_server.cpp
)

target_link_libraries (webapp_server
  webapp_lib
)

if ("${INSTALL_SOURCE}" STREQUAL "demo")
  install_targets(/demo webapp_server)
endif ()

