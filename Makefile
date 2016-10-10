OUTPUT_SUB_DIR = $(shell pwd)/output
# параметры для управления кроссплатформенной сборкой
CMAKE_CXX_COMPILER   ?= "c++"
CMAKE_C_COMPILER     ?= "cc"
CMAKE_FIND_ROOT_PATH ?= ""

CMAKE = cmake ../src/ -DOUTPUT_DIR=$(OUTPUT_SUB_DIR) \
                      -DCMAKE_CXX_COMPILER=$(CMAKE_CXX_COMPILER) \
                      -DCMAKE_C_COMPILER=$(CMAKE_C_COMPILER) \
                      -DCMAKE_FIND_ROOT_PATH=$(CMAKE_FIND_ROOT_PATH)

all: build_bin help

help:
	@echo " --- Помощь -------------------------------------------------------------------------------------"
	@echo " * build_bin         - сборка библиотеки, демонстрационного сервера, тестов"
	@echo " * build_jquery      - сборка JQuery"
	@echo " * tests             - запуск юнит-тестов"
	@echo " * install           - установка всех необходимых файлов, для веб приложения, в директорию output"
	@echo " * update_submodules - обновление исходников сторонних проектов"
	@echo " * dependencies      - проверка наличия необходимых пакетов"

build_bin:
	@echo "--- Сборка ---------------------------"
	@mkdir -p ./build ./output
	@cd ./build/ && rm -rf ./* && $(CMAKE) && make && make install

build_jquery:
	@echo "--- Сборка JQuery --------------------"
	@cd ./src/third-party/jquery && npm install && grunt

tests:
	@echo "--- Тестирование ---------------------"
	@cd ./output && ./webapp_units_tests

install:
	@echo "--- Установка ------------------------"
	@cd ./build/ && $(CMAKE) && make install

update_submodules:
	@echo "--- Обновление сторонних библиотек ---"
	$(shell for subm in ./src/third-party/*; do [ -d "$subm" ] && [ ! -f "$subm/.git" ] && git submodule init && git submodule update; done)
	@git submodule foreach git pull origin master

dependencies:
	$(shell [[ $(whereis -b npm) =~ ^npm:\ (.*)$ ]] || echo "Установите пакет NPM!" && exit 1)
	$(shell [[ $(whereis -b grunt) =~ ^grunt:\ (.*)$ ]] && npm install -g grunt-cli)