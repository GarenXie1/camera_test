TOP_DIR := $(shell pwd)
APP = $(TOP_DIR)/cameratest

CROSS_COMPLICE=/home/garen/buildroot/buildroot/output/host/bin/aarch64-linux-gnu-
CC=$(CROSS_COMPLICE)gcc

CFLAGS = -g
LIBS = -lpthread
DEP_LIBS = -L$(TOP_DIR)/lib
HEADER =
OBJS = main.o video_capture.o 

#####编译时, 链接静态库####
# -static
#


all:  $(OBJS)			# 这里会导致 main.c -> main.o ; video_capture.c --> video_capture.o
	$(CC) -g -o $(APP) $(OBJS) $(LIBS)

clean:
	rm -f *.o a.out $(APP) core *~
