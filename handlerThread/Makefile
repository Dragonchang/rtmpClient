#子目录的Makefile直接读取其子目录就行
CC := g++
SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}')
#以下同根目录下的makefile的相同代码的解释
CUR_SOURCE=${wildcard *.cpp}
CUR_OBJS=${patsubst %.cpp, %.o, $(CUR_SOURCE)}
all:$(CUR_OBJS)
$(CUR_OBJS):%.o:%.cpp
	$(CC) -c -g $^ -o $(ROOT_DIR)/$(OBJ)/$@

