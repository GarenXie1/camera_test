#ifndef _VIDEO_CAPTURE_H
#define _VIDEO_CAPTURE_H

#include "videodev2.h"

struct buffer {
	void *start;
	size_t length;
};

struct camera {
	char *device_name;
	int fd;
	int width;
	int height;
	int display_depth;
	int image_size;
	int frame_number;
	struct v4l2_capability v4l2_cap;
	struct v4l2_cropcap v4l2_cropcap;
	struct v4l2_format v4l2_fmt;
	struct v4l2_crop crop;
	struct buffer *buffers;
};

void errno_exit(const char *s);

int xioctl(int fd, int request, void *arg);

void open_camera(struct camera *cam);
void close_camera(struct camera *cam);

int read_and_encode_frame(struct camera *cam);

void start_capturing(struct camera *cam);
void stop_capturing(struct camera *cam);

void init_camera(struct camera *cam);
void uninit_camera(struct camera *cam);

void init_mmap(struct camera *cam);

void v4l2_init(struct camera *cam);
void v4l2_close(struct camera *cam);
int display_pix_format(struct camera *cam);
int v4l2_display_sizes_pix_format(struct camera *cam , __u32 pixel_format);



#endif

