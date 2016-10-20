add_library(webapp_lib
  webapp_lib.cpp
  webapp_proto.cpp
  webapp_proto_http.cpp
  webapp_com.cpp
  webapp_com_ctl.cpp
  webapp_com_http.cpp
)

target_link_libraries (webapp_lib
  pthread
  boost_thread
  boost_system
  boost_filesystem
  boost_date_time
  boost_regex
)

if ("${INSTALL_SOURCE}" STREQUAL "back-end")
  install_targets(/ webapp_lib)
endif ()
