add_executable(vbr_webapp_server
  vbr_webapp_server.cpp
)

target_link_libraries (vbr_webapp_server
  vbr_webapp_lib
)

install_targets(/ vbr_webapp_server)

