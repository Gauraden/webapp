#!/bin/bash

Init() {
  for subm in ${1}/*; do
    [ -d "$subm" ] && [ ! -f "$subm/.git" ] && git submodule init && git submodule update
  done
  return 0
}

RebuildJQuery() {
  cd "${1}"
  npm install
  grunt
}

BuildJQuery() {
  cd "${1}"
  [ ! -f ./dist/jquery.js ] && npm install && grunt
  return 0
}

case "${1}" in
	'init'          ) Init          "${2}";;
	'build_jquery'  ) BuildJQuery   "${2}";;
	'rebuild_jquery') RebuildJQuery "${2}";;
esac
