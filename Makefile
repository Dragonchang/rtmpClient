SHELL := /bin/bash 
CC := g++
$(shell mkdir -p  out)
$(shell mkdir -p  out/bin)
$(shell mkdir -p  out/obj)
BIN_APP :=rtmpClient
#当前目录下除了out的所有子目录
SUBDIRS=$(shell ls -l | grep ^d | awk '{if(($$9 != "out")&&($$9 != "libs")) print $$9}')
DEBUG=$(shell ls -l | grep ^d | awk '{if($$9 == "out") print $$9}')
ROOT_DIR=$(shell pwd)
OUT := out/
BIN := out/bin
OBJ := out/obj
#当前目录的cpp文件
SRCS := ${wildcard *.cpp}
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

OPENCV := /usr/local/ffmpeg
FFMPEG := /usr/local/ffmpeg
#将TARGET OBJ BIN ROOT_DIR这些变量导入到shell中
export CC TARGET OBJ BIN ROOT_DIR OUT BIN BIN_APP OPENCV FFMPEG

all:$(SUBDIRS) $(OBJS) DEBUG

$(SUBDIRS):ECHO
	make -C $@

DEBUG:ECHO
	make -C out

ECHO:
	@echo $(SUBDIRS)

#将cpp文件编译为o文件，并放在指定放置目标文件的目录中即OBJS_DIR
$(OBJS):%.o:%.cpp
	$(CC) -c -g -Wconversion -fPIC -I $(FFMPEG)/include/  $^ -o $(ROOT_DIR)/$(OBJ)/$@ -Os -std=c++11

clean:
	rm -rf $(ROOT_DIR)/$(OBJ)/*.o
	rm -rf $(ROOT_DIR)/$(BIN)/
