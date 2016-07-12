add_executable(vbr_webapp_test
    vbr_webapp_test.cpp
)

target_link_libraries (vbr_webapp_test
  vbr_webapp_lib
)

install_targets(/ vbr_webapp_test)

