1.源文件camera_hdmiin：
作用：抛开底层上报buf给上层的过程，直接在底层抓帧数据并写入文件，确认底层数据是否获取正常
使用方法：
解压，并修改里面的Makefile：
CC = /opt/arm-2011.09/bin/arm-none-linux-gnueabi-gcc
这个改为你的编译链的地址；然后make clean && make，就可以在解压目录的bin目录得到编译结果，push到机器里，就可以运行。

编译:
	make clean && make
运行:
	./cameratest

修改 格式, 抓取帧数:
	config.h
		#define CAPTURE_WIDTH       1280
		#define CAPTURE_HEIGHT      720
		#define CAPTURE_FRAME_NUM   1000
		#define PIXELFORMAT         V4L2_PIX_FMT_YUYV	

得到:
	data.yuv


