OUTPUT_SUB_DIR = $(shell pwd)/output
# параметры для управления кроссплатформенной сборкой
CMAKE_CXX_COMPILER   ?= "c++"
CMAKE_C_COMPILER     ?= "cc"
CMAKE_FIND_ROOT_PATH ?= ""

CMAKE = cmake ../src/ -DOUTPUT_DIR=$(OUTPUT_SUB_DIR) \
                      -DCMAKE_CXX_COMPILER=$(CMAKE_CXX_COMPILER) \
                      -DCMAKE_C_COMPILER=$(CMAKE_C_COMPILER) \
                      -DCMAKE_FIND_ROOT_PATH=$(CMAKE_FIND_ROOT_PATH)

JQUERY_PATH = "./src/third-party/jquery"

all: build_bin install help

pull: update_submodules

help:
	@echo " --- Помощь -------------------------------------------------------------------------------------"
	@echo " * build_bin         - сборка библиотеки, демонстрационного сервера, тестов"
	@echo " * build_jquery      - сборка JQuery"
	@echo " * tests             - запуск юнит-тестов"
	@echo " * install           - установка библиотеки для реализации сервера, веб приложения, в директорию /output"
	@echo " * install_frontend  - установка библиотек для реализации клиента, веб приложения, в директорию /output/front-end"
	@echo " * install_demo      - установка приложения-демонстрации, в директорию /output/demo"
	@echo " * install_utests    - установка юнит тестов в директорию /output"
	@echo " * update_submodules - обновление исходников сторонних проектов"
	@echo " * dependencies      - проверка наличия необходимых пакетов"

build_bin:
	@echo "--- Сборка ---------------------------"
	@mkdir -p ./build ./output
	@cd ./build/ && rm -rf ./* && $(CMAKE) && make

build_jquery:
	@echo "--- Сборка JQuery --------------------"
	@./submodules.sh rebuild_jquery "$(JQUERY_PATH)"

tests:
	@echo "--- Тестирование ---------------------"
	@cd ./output && ./webapp_units_tests

install:
	@echo "--- Установка back-end ---------------"
	@cd ./build/ && $(CMAKE) -DINSTALL_SOURCE=back-end && make install
	
install_frontend:
	@echo "--- Установка front-end --------------"
	@./submodules.sh build_jquery "$(JQUERY_PATH)"
	@cd ./build/ && $(CMAKE) -DINSTALL_SOURCE=front-end && make install
	
install_demo:
	@echo "--- Установка demo -------------------"
	@./submodules.sh build_jquery "$(JQUERY_PATH)"
	@cd ./build/ && $(CMAKE) -DINSTALL_SOURCE=demo && make install
	
install_utests:
	@echo "--- Установка unit tests -------------"
	@cd ./build/ && $(CMAKE) -DINSTALL_SOURCE=unit-tests && make install

update_submodules:
	@echo "--- Обновление сторонних библиотек ---"
	@./submodules.sh init "./src/third-party"
	@git submodule foreach git pull origin master

dependencies:
	$(shell [[ $(whereis -b npm) =~ ^npm:\ (.*)$ ]] || echo "Установите пакет NPM!" && exit 1)
	$(shell [[ $(whereis -b grunt) =~ ^grunt:\ (.*)$ ]] && npm install -g grunt-cli)