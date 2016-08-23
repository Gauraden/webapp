add_executable(webapp_server
  webapp_server.cpp
)

target_link_libraries (webapp_server
  webapp_lib
)

install_targets(/ webapp_server)

