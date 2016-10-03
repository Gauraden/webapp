add_executable (webapp_units_tests
  units_tests.cpp
  proto_test.cpp
  com_test.cpp
  com_http_test.cpp
  com_ctl_test.cpp
)

target_link_libraries (webapp_units_tests
  boost_unit_test_framework
  webapp_lib
)

install_targets(/ webapp_units_tests)
