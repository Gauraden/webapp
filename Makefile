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

pull: update_submodules

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
	@./submodules.sh rebuild_jquery "./src/third-party/jquery"

tests:
	@echo "--- Тестирование ---------------------"
	@cd ./output && ./webapp_units_tests

install:
	@echo "--- Установка ------------------------"
	@cd ./build/ && $(CMAKE) && make install

update_submodules:
	@echo "--- Обновление сторонних библиотек ---"
	@./submodules.sh init "./src/third-party"
	@git submodule foreach git pull origin master
	@./submodules.sh build_jquery "./src/third-party/jquery"

dependencies:
	$(shell [[ $(whereis -b npm) =~ ^npm:\ (.*)$ ]] || echo "Установите пакет NPM!" && exit 1)
	$(shell [[ $(whereis -b grunt) =~ ^grunt:\ (.*)$ ]] && npm install -g grunt-cli)