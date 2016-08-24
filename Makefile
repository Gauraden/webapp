OUTPUT_SUB_DIR = $(shell pwd)/output

CMAKE = cmake ../src/ -DOUTPUT_DIR=$(OUTPUT_SUB_DIR)

all:
	@echo "--- Сборка -----------------------"
	@mkdir -p ./build ./output
	@cd ./build/ && rm -rf ./* && $(CMAKE) && make && make install

tests:
	@echo "--- Тестирование -----------------"
	@cd ./output && ./units_tests