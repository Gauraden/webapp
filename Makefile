OUTPUT_SUB_DIR = $(shell pwd)/output
# параметры для управления кроссплатформенной сборкой
CMAKE_CXX_COMPILER   ?= "c++"
CMAKE_C_COMPILER     ?= "cc"
CMAKE_FIND_ROOT_PATH ?= ""

CMAKE = cmake ../src/ -DOUTPUT_DIR=$(OUTPUT_SUB_DIR) \
                      -DCMAKE_CXX_COMPILER=$(CMAKE_CXX_COMPILER) \
                      -DCMAKE_C_COMPILER=$(CMAKE_C_COMPILER) \
                      -DCMAKE_FIND_ROOT_PATH=$(CMAKE_FIND_ROOT_PATH)

all: update_submodules build

build:
	@echo "--- Сборка ---------------------------"
	@mkdir -p ./build ./output
	@cd ./build/ && rm -rf ./* && $(CMAKE) && make && make install

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
