CC := g++
CFLAGS := -g -L$(OPENCV_FFMPEG)/lib -lpthread -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lavformat -lavcodec -lavutil -lswscale -std=c++11
OBJS=*.o
ODIR=./obj
$(ROOT_DIR)/$(BIN)/$(BIN_APP):$(ODIR)/$(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)