add_library(webapp_lib
  webapp_lib.cpp
)

target_link_libraries (webapp_lib
  boost_system
  boost_filesystem
  boost_date_time
  pthread
)

install_targets(/ webapp_lib)