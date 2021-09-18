TOP_DIR := $(shell pwd)
APP = $(TOP_DIR)/bin/cameratest

#CC = /home/local/toolchain/aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
CC = /opt/arm-2011.09/bin/arm-none-linux-gnueabi-gcc
CFLAGS = -g 
LIBS = -lpthread  -lm 
DEP_LIBS = -L$(TOP_DIR)/lib
HEADER =
OBJS = main.o video_capture.o 

all:  $(OBJS)
	$(CC) -g -o $(APP) $(OBJS) $(LIBS) -static

clean:
	rm -f *.o a.out $(APP) core *~
