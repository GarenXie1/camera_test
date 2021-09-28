#include <asm/types.h>          /* for videodev2.h */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
//#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include "video_capture.h"

#include "config.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef unsigned char uint8_t;


FILE *yuv_fp;

unsigned int n_buffers = 0;
DIR *dirp;

int cnt = 0;
unsigned int MOTOR_POSITION_VAL = 0;
unsigned int GAIN_VALUE = 200;
unsigned int exposure = 0;

void errno_exit(const char *s) {
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

int xioctl(int fd, int request, void *arg) {
	int r = 0;
	do {
		r = ioctl(fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

void open_camera(struct camera *cam) {
	struct stat st;

	if (-1 == stat(cam->device_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", cam->device_name,
				errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", cam->device_name);
		exit(EXIT_FAILURE);
	}

	cam->fd = open(cam->device_name, O_RDWR, 0); //  | O_NONBLOCK

	if (-1 == cam->fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", cam->device_name, errno,
				strerror(errno));
		exit(EXIT_FAILURE);
	}
	
}

void close_camera(struct camera *cam) {
	if (-1 == close(cam->fd))
		errno_exit("close");

	cam->fd = -1;
}

static int v4l2_g_ctrl(int fp, unsigned int id)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;

    ret = ioctl(fp, VIDIOC_G_CTRL, &ctrl);
    if (ret < 0) {
        printf("error:v4l2_g_ctrl error id:%d, value:%d \n",ctrl.id, ctrl.value);
        return ret;
    }

    return ctrl.value;
}
static int v4l2_s_ctrl(int fp, unsigned int id, unsigned int value)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;
    ctrl.value = value;
	
    ret = ioctl(fp, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        printf("error:v4l2_s_ctrl error id:%d, value:%d \n",ctrl.id, ctrl.value);
        return ret;
    }

    return 0;
}

void init_file() {
	yuv_fp = fopen(OUTPUT_FILE, "wa+");
}

void close_file() {
	fclose(yuv_fp);
}

static int fwrite_cnt = 0;
void encode_frame(uint8_t *yuv_frame, size_t yuv_length) {
	//写yuv文件
	fwrite(yuv_frame, yuv_length, 1, yuv_fp);
}

int read_and_encode_frame(struct camera *cam) {
	struct v4l2_buffer buf;
	unsigned int value = 0;
	char *id_buf = NULL;

	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	//this operator below will change buf.index and (0 <= buf.index <= 3)
	if (-1 == xioctl(cam->fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
		case EAGAIN:
			return 0;
		case EIO:
			/* Could ignore EIO, see spec. */
			/* fall through */
		default:
			errno_exit("VIDIOC_DQBUF");
		}
	}

#ifdef ENCODE_FILE
	encode_frame(cam->buffers[buf.index].start, buf.length);
#endif

	if (-1 == xioctl(cam->fd, VIDIOC_QBUF, &buf))
		errno_exit("VIDIOC_QBUF");
		
	return 1;
}

void start_capturing(struct camera *cam) {
	unsigned int i,value;
	enum v4l2_buf_type type;

	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(cam->fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(cam->fd, VIDIOC_STREAMON, &type))
		errno_exit("VIDIOC_STREAMON");
}

void stop_capturing(struct camera *cam) {
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(cam->fd, VIDIOC_STREAMOFF, &type))
		errno_exit("VIDIOC_STREAMOFF");

}
void uninit_camera(struct camera *cam) {
	unsigned int i;

	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap(cam->buffers[i].start, cam->buffers[i].length))
			errno_exit("munmap");

	free(cam->buffers);
}

int v4l2_display_sizes_pix_format(struct camera *cam , __u32 pixel_format)
{
	int ret = 0;
	int fsizeind = 0; /*index for supported sizes*/
	int fd = cam->fd;
	struct v4l2_frmsizeenum fsize;

	printf("------VIDIOC_ENUM_FRAMESIZES------\\n");
    printf("V4L2 pixel sizes:\n");

	CLEAR(fsize);
	fsize.index = 0;
	fsize.pixel_format = pixel_format;

	while ((ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize)) == 0)	{
		fsize.index++;
		if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
			printf("('%c%c%c%c'--> %u x %u ) Pixels\n", fsize.pixel_format >> 0, fsize.pixel_format >> 8,\
						fsize.pixel_format >> 16, fsize.pixel_format >> 24, fsize.discrete.width, fsize.discrete.height);
			fsizeind++;
		}
	}
	printf("\n");
    return fsizeind;
}


int display_pix_format(struct camera *cam)
{
    struct v4l2_fmtdesc fmt;
    int index;
	int fd = cam->fd;

	printf("------VIDIOC_ENUM_FMT------\\n");
    printf("V4L2 pixel formats:\n");

    index = 0;
    CLEAR(fmt);
    fmt.index = index;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (xioctl(fd, VIDIOC_ENUM_FMT, &fmt) != -1) {
        printf("%i: [0x%08X] '%c%c%c%c' (%s)\n", index, fmt.pixelformat, fmt.pixelformat >> 0, fmt.pixelformat >> 8, fmt.pixelformat >> 16, fmt.pixelformat >> 24, fmt.description);

        memset(&fmt, 0, sizeof(fmt));
        fmt.index = ++index;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }
    printf("\n");
}

void init_mmap(struct camera *cam) {
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 6;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	//分配内存
	if (-1 == xioctl(cam->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
					"memory mapping\n", cam->device_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n", cam->device_name);
		exit(EXIT_FAILURE);
	}

	cam->buffers = calloc(req.count, sizeof(*(cam->buffers)));

	if (!cam->buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		//将VIDIOC_REQBUFS中分配的数据缓存转换成物理地址
		if (-1 == xioctl(cam->fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		cam->buffers[n_buffers].length = buf.length;
		cam->buffers[n_buffers].start = mmap(NULL /* start anywhere */,
				buf.length, PROT_READ | PROT_WRITE /* required */,
				MAP_SHARED /* recommended */, cam->fd, buf.m.offset);

		if (MAP_FAILED == cam->buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

void init_camera(struct camera *cam) {
	struct v4l2_capability *cap = &(cam->v4l2_cap);
	struct v4l2_cropcap *cropcap = &(cam->v4l2_cropcap);
	struct v4l2_crop *crop = &(cam->crop);
	struct v4l2_format *fmt = &(cam->v4l2_fmt);
	unsigned int min;

	if (-1 == xioctl(cam->fd, VIDIOC_QUERYCAP, cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n", cam->device_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}
    printf("------VIDIOC_QUERYCAP------\n");
    printf("Driver: \"%s\"\n", cap->driver);
    printf("Card: \"%s\"\n", cap->card);
    printf("Bus: \"%s\"\n", cap->bus_info);
    printf("Version: %d.%d\n", (cap->version >> 16) && 0xff, (cap->version >> 24) && 0xff);
    printf("Capabilities: 0x%08x\n", cap->capabilities);

	if (!(cap->capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n", cam->device_name);
		exit(EXIT_FAILURE);
	}else{
		fprintf(stderr, "	%s is video capture device\n", cam->device_name);
	}

	if (!(cap->capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, "%s does not support streaming i/o\n",cam->device_name);
		exit(EXIT_FAILURE);
	}else{
		fprintf(stderr, "	%s does support streaming i/o\n", cam->device_name);
	}
	printf("\n");

	display_pix_format(cam);
	v4l2_display_sizes_pix_format(cam,PIXELFORMAT);
	/* Select video input, video standard and tune here. */

	CLEAR(*cropcap);

	cropcap->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	crop->c.width = cam->width;
	crop->c.height = cam->height;
	crop->c.left = 0;
	crop->c.top = 0;
	crop->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	CLEAR(*fmt);

	fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt->fmt.pix.width = cam->width;
	fmt->fmt.pix.height = cam->height;
	fmt->fmt.pix.pixelformat = PIXELFORMAT;
	fmt->fmt.pix.field = PIXELFIELD;

	if (-1 == xioctl(cam->fd, VIDIOC_S_FMT, fmt))
		errno_exit("VIDIOC_S_FMT");

	printf("------VIDIOC_S_FMT------\n");
	printf("	width*height = %d*%d\n", fmt->fmt.pix.width,fmt->fmt.pix.height);
	printf("	pixelformat = '%c%c%c%c'\n", fmt->fmt.pix.pixelformat >> 0, fmt->fmt.pix.pixelformat >> 8, \
										fmt->fmt.pix.pixelformat >> 16, fmt->fmt.pix.pixelformat >> 24);
	printf("\n");

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt->fmt.pix.width * 2;
	if (fmt->fmt.pix.bytesperline < min)
		fmt->fmt.pix.bytesperline = min;
	min = fmt->fmt.pix.bytesperline * fmt->fmt.pix.height;
	if (fmt->fmt.pix.sizeimage < min)
		fmt->fmt.pix.sizeimage = min;

	init_mmap(cam);

}

void v4l2_init(struct camera *cam) {
	open_camera(cam);
	init_camera(cam);
	start_capturing(cam);
	init_file();
}

void v4l2_close(struct camera *cam) {
	stop_capturing(cam);
	uninit_camera(cam);
	close_camera(cam);
	free(cam);
	close_file();
}

