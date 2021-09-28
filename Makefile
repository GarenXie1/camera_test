TOP_DIR := $(shell pwd)
APP = $(TOP_DIR)/cameratest

CROSS_COMPLICE=/home/garen/buildroot/buildroot/output/host/bin/aarch64-linux-gnu-
CC=$(CROSS_COMPLICE)gcc

LDFLAGS =
INCLUDEFLAGS =
CFLAGS = -g		# -Wall -O
LIBS = -lpthread
OBJS = main.o video_capture.o

#####编译时, 链接静态库####
# -static
#

cameratest:  $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)


%.o:%.c		# 把所有的 .c 文件都编译成 .o 文件
	$(CC) -o $@ -c $< $(CFLAGS) $(INCLUDEFLAGS)


%.d:%.c
	@set -e; rm -f $@; $(CC) -MM $< $(INCLUDEFLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(OBJS:.o=.d)								# 自动生成 头文件".h" 依赖

clean:
	rm -f *.o $(APP) *.d *.d.*