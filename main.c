#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <malloc.h>
#include "video_capture.h"

#include "config.h"

struct camera *cam;
pthread_t mythread;

double get_wall_time()
{
    struct timeval time;
    if (gettimeofday(&time, NULL))
        return 0.;
    return (double) time.tv_sec + (double) time.tv_usec * .000001;
}

void capture_encode_thread(void) {
	long count = 0;
    double after , fps , before;

	for (;;) {

		before = get_wall_time();
		if (count++ >= CAPTURE_FRAME_NUM) // 采集N帧的数据
				{
			printf("------need to exit from thread------- \n");
			break;
		}

		int r = 1;
		fd_set fds;
		struct timeval tv;


		FD_ZERO(&fds);
		FD_SET(cam->fd, &fds);

		/* Timeout. */
		tv.tv_sec = 300;
		tv.tv_usec = 0;

		r = select(cam->fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				continue;

			errno_exit("select");
		}

		if (0 == r) {
			v4l2_close(cam);
			fprintf(stderr, "select timeout\n");
			exit(EXIT_FAILURE);
		}

		if (read_and_encode_frame(cam) != 1) {
			fprintf(stderr, "read_fram fail in thread\n");
			break;
		}

		after = get_wall_time();
		fps = 1.0 / (after - before);
		printf("FPS[%d]: %.2f fps\n", count, fps);

	}
}

int main(int argc, char **argv) {
	cam = (struct camera *) malloc(sizeof(struct camera));
	if (!cam) {
		printf("malloc camera failure!\n");
		exit(1);
	}
	cam->device_name = VIDEO_DEVICE;
	cam->width = CAPTURE_WIDTH;
	cam->height = CAPTURE_HEIGHT;
	cam->buffers = NULL;

	v4l2_init(cam);

	if (0 != pthread_create(&mythread, NULL, (void *) capture_encode_thread, NULL)) {
		fprintf(stderr, "thread create fail\n");
	}
	pthread_join(mythread, NULL);
	printf("-----------end program------------");
CLOSE:
	v4l2_close(cam);

	return 0;
}
