if ("${INSTALL_SOURCE}" STREQUAL "demo")
  install(FILES core.js demo_webapp.html DESTINATION "${CMAKE_INSTALL_PREFIX}/demo/static")
  install(DIRECTORY css widgets DESTINATION "${CMAKE_INSTALL_PREFIX}/demo/static")
endif ()

if ("${INSTALL_SOURCE}" STREQUAL "front-end")
  install(FILES core.js DESTINATION "${CMAKE_INSTALL_PREFIX}/front-end")
  install(DIRECTORY css widgets DESTINATION "${CMAKE_INSTALL_PREFIX}/front-end")
endif ()
