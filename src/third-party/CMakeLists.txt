set(BOOTSTRAP_DIR    "bootstrap/dist")
set(BOOTSTRAP_DT_DIR "bootstrap-datetimepicker/build")
set(JQUERY_DIR       "jquery/dist")
set(MOMENT_DIR       "moment/min")

set(3DPARTY_OUTPUT_DIR "${CMAKE_INSTALL_PREFIX}/static/third-party")
set(JS_OUTPUT_DIR      "${CMAKE_INSTALL_PREFIX}/static/third-party/js")
set(CSS_OUTPUT_DIR     "${CMAKE_INSTALL_PREFIX}/static/third-party/css")
set(FONT_OUTPUT_DIR    "${CMAKE_INSTALL_PREFIX}/static/third-party/font")

#install(DIRECTORY
install(FILES
  ${MOMENT_DIR}/moment-with-locales.min.js
  ${JQUERY_DIR}/jquery.min.js
  ${BOOTSTRAP_DIR}/js/bootstrap.min.js
  ${BOOTSTRAP_DT_DIR}/js/bootstrap-datetimepicker.min.js
DESTINATION "${JS_OUTPUT_DIR}")

install(FILES
  ${BOOTSTRAP_DIR}/css/bootstrap.min.css
  ${BOOTSTRAP_DIR}/css/bootstrap-theme.min.css
  ${BOOTSTRAP_DT_DIR}/css/bootstrap-datetimepicker.min.css
DESTINATION "${CSS_OUTPUT_DIR}")

install(DIRECTORY
  ${BOOTSTRAP_DIR}/fonts
DESTINATION "${3DPARTY_OUTPUT_DIR}")
