add_executable (units_tests
  units_tests.cpp
  protocol_test.cpp
)

target_link_libraries (units_tests
  boost_unit_test_framework
  vbr_webapp_lib
)

install_targets(/ units_tests)
