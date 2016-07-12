OUTPUT_SUB_DIR = $(shell pwd)/output

CMAKE = cmake ../src/ -DOUTPUT_DIR=$(OUTPUT_SUB_DIR)

all:
	cd ./build/ && rm -rf ./* && $(CMAKE) && make && make install