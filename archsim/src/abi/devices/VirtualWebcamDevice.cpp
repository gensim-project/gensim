/*
 * VirtualWebcamDevice.cpp
 *
 *  Created on: 7 Mar 2014
 *      Author: harry
 */

#include "abi/EmulationModel.h"
#include "abi/devices/VirtualWebcamDevice.h"
#include "abi/memory/MemoryModel.h"
#include "util/Log.h"

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <sys/mman.h>

struct VirtualWebcamBuffer {
	void *start;
	unsigned int length;
};

struct VirtualWebcamContext {
	int fd, width, height, nr_buffers;
	struct VirtualWebcamBuffer *buffers;
};

static int ioctl_r(int fd, int request, void *arg)
{
	int rc;

	do {
		rc = v4l2_ioctl(fd, request, arg);
	} while (rc == -1 && ((errno == EINTR) || (errno == EAGAIN)));

	if (rc == -1) {
		fprintf(stderr, "error: unable to perform ioctl: %d: %s\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}

static int webcam_init_buffers(struct VirtualWebcamContext *ctx)
{
	struct v4l2_requestbuffers rb;
	int i;

	memset(&rb, 0, sizeof(rb));

	rb.count = 4;
	rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rb.memory = V4L2_MEMORY_MMAP;

	if (ioctl_r(ctx->fd, VIDIOC_REQBUFS, &rb)) {
		fprintf(stderr, "error: unable to request buffer information\n");
		return -1;
	}

	fprintf(stderr, "debug: number of buffers: %d\n", rb.count);

	ctx->buffers = (struct VirtualWebcamBuffer*) calloc(rb.count, sizeof(*ctx->buffers));
	if (ctx->buffers == NULL) {
		fprintf(stderr, "error: unable to allocate buffer descriptors\n");
		return -1;
	}

	ctx->nr_buffers = rb.count;
	for (i = 0; i < ctx->nr_buffers; i++) {
		struct v4l2_buffer buffer;

		memset(&buffer, 0, sizeof(buffer));
		buffer.type = rb.type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (ioctl_r(ctx->fd, VIDIOC_QUERYBUF, &buffer)) {
			fprintf(stderr, "error: unable to query buffer information\n");
			return -1;
		}

		fprintf(stderr, "debug: about to mmap %x @ %x\n", buffer.length, buffer.m.offset);

		ctx->buffers[i].length = buffer.length;
		ctx->buffers[i].start = v4l2_mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->fd, buffer.m.offset);

		if (ctx->buffers[i].start == MAP_FAILED) {
			fprintf(stderr, "error: unable to mmap buffer: %s\n", strerror(errno));
			return -1;
		}
	}

	return 0;
}

static struct VirtualWebcamContext *webcam_init()
{
	struct VirtualWebcamContext *ctx = new VirtualWebcamContext();
	struct v4l2_format fmt;

	ctx->fd = v4l2_open("/dev/video0", O_RDWR | O_NONBLOCK, 0);

	ctx->width = 640;
	ctx->height = 480;

	bzero(&fmt, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = ctx->width;
	fmt.fmt.pix.height      = ctx->height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	if (ioctl_r(ctx->fd, VIDIOC_S_FMT, &fmt)) {
		v4l2_close(ctx->fd);
		delete ctx;
		LOG(LOG_ERROR) << "[WEBCAM] Could not set webcam format";
		return NULL;
	}

	if (webcam_init_buffers(ctx)) {
		v4l2_close(ctx->fd);
		delete ctx;
		LOG(LOG_ERROR) << "[WEBCAM] Could not init webcam buffer";
		return NULL;
	}

	return ctx;
}

static void webcam_destroy(struct VirtualWebcamContext *webcam)
{
	int i;

	for (i = 0; i < webcam->nr_buffers; i++)
		v4l2_munmap(webcam->buffers[i].start, webcam->buffers[i].length);

	free(webcam->buffers);
	v4l2_close(webcam->fd);
	free(webcam);
}

static int webcam_start(struct VirtualWebcamContext *webcam)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int i;

	for (i = 0; i < webcam->nr_buffers; i++) {
		struct v4l2_buffer buffer;

		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (ioctl_r(webcam->fd, VIDIOC_QBUF, &buffer)) {
			fprintf(stderr, "error: unable to queue buffer %d\n", i);
			return -1;
		}
	}

	return ioctl_r(webcam->fd, VIDIOC_STREAMON, &type);
}

static int webcam_stop(struct VirtualWebcamContext *webcam)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	return ioctl_r(webcam->fd, VIDIOC_STREAMOFF, &type);
}

static struct v4l2_buffer webcam_grab_start(struct VirtualWebcamContext *webcam, void **imagep)
{
	struct v4l2_buffer buffer;
	bzero(&buffer, sizeof(buffer));

	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.memory = V4L2_MEMORY_MMAP;

	if (ioctl_r(webcam->fd, VIDIOC_DQBUF, &buffer)) {
		fprintf(stderr, "error: unable to dequeue buffer\n");
		exit(1);
	}

	*imagep = webcam->buffers[buffer.index].start;

	return buffer;
}

static int webcam_grab_end(struct VirtualWebcamContext *webcam, struct v4l2_buffer buffer)
{
	if (ioctl_r(webcam->fd, VIDIOC_QBUF, &buffer)) {
		fprintf(stderr, "error: unable to queue buffer\n");
		return -1;
	}

	return 0;
}

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			VirtualWebcamDevice::VirtualWebcamDevice(EmulationModel &model) : Thread("Virtual Webcam"), emulation_model(model), terminated(false)
			{

			}

			VirtualWebcamDevice::~VirtualWebcamDevice()
			{
				terminated = true;
				while(webcam) ;
			}

			int VirtualWebcamDevice::GetComponentID()
			{
				return 'b';
			}

			bool VirtualWebcamDevice::Attach()
			{
				emulation_model.memory_model.MapRegion(0xff100000, 0x100000, (abi::memory::RegionFlags)(abi::memory::RegFlagRead | abi::memory::RegFlagWrite), "[webcam]");
				start();
				return true;
			}

			void VirtualWebcamDevice::run()
			{
				webcam = webcam_init();
				webcam_start(webcam);

				uint32_t webcam_buffer = 3 * webcam->width * webcam->height;

				while(!terminated) {
					char *image_data;
					auto buffer = webcam_grab_start(webcam, (void**)&image_data);

					emulation_model.memory_model.WriteBlock(0xff100000, image_data, webcam_buffer);

					webcam_grab_end(webcam, buffer);

					usleep(30000);
				}

				webcam_destroy(webcam);
				webcam = NULL;
			}

		}
	}
}
