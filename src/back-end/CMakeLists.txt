add_library(vbr_webapp_lib
  vbr_webapp_lib.cpp
)

target_link_libraries (vbr_webapp_lib
  boost_system
  boost_filesystem
  boost_date_time
)

install_targets(/ vbr_webapp_lib)