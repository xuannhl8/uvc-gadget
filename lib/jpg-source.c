/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * JPEG still image video source
 *
 * Copyright (C) 2018 Paul Elder
 *
 * Contact: Paul Elder <paul.elder@ideasonboard.com>
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <linux/videodev2.h>
#include <jpeglib.h>

#include "events.h"
#include "timer.h"
#include "tools.h"
#include "v4l2.h"
#include "jpg-source.h"
#include "video-buffers.h"

struct jpg_source {
	struct video_source src;

	unsigned int imgsize;
	unsigned char *imgdata;

	struct timer *timer;
	bool streaming;
};

#define to_jpg_source(s) container_of(s, struct jpg_source, src)

static void jpg_source_destroy(struct video_source *s)
{
	struct jpg_source *src = to_jpg_source(s);

	if (src->imgdata)
		free(src->imgdata);

	timer_destroy(src->timer);

	free(src);
}

static int jpg_source_set_format(struct video_source *s __attribute__((unused)),
				  struct v4l2_pix_format *fmt)
{
	if (fmt->pixelformat != v4l2_fourcc('M', 'J', 'P', 'G')) {
		printf("jpg-source: unsupported fourcc\n");
		return -EINVAL;
	}

	return 0;
}

static int jpg_source_set_frame_rate(struct video_source *s, unsigned int fps)
{
	struct jpg_source *src = to_jpg_source(s);

	timer_set_fps(src->timer, fps);

	return 0;
}

static int jpg_source_free_buffers(struct video_source *s __attribute__((unused)))
{
	return 0;
}

static int jpg_source_stream_on(struct video_source *s)
{
	struct jpg_source *src = to_jpg_source(s);
	int ret;

	ret = timer_arm(src->timer);
	if (ret)
		return ret;

	src->streaming = true;
	return 0;
}

static int jpg_source_stream_off(struct video_source *s)
{
	struct jpg_source *src = to_jpg_source(s);
	int ret;

	/*
	 * No error check here, because we want to flag that streaming is over
	 * even if the timer is still running due to the failure.
	 */
	ret = timer_disarm(src->timer);
	src->streaming = false;

	return ret;
}

static void jpg_source_fill_buffer(struct video_source *s,
				   struct video_buffer *buf)
{
	struct jpg_source *src = to_jpg_source(s);
	unsigned int size;
	
	/*
	 * Nothing currently stops the user from providing a source file which is
	 * larger than the buffer size calculated from the format we have set.
	 * Clamp the size of the buffer we copy to be sure we don't overflow the
	 * buffer we was allocated to receive it.
	 */

	/* Function read and find frame in JPG Source */
	static int inDex = 0;
	static int frameCounter = 0;
	int inDex_Start_Frame, inDex_End_Frame, frameSize;
	bool needFillBuffer = true; /* Flag accept to fill buffer */
	while (1) {
		if (!needFillBuffer) {
        	break;
    	}
		unsigned char prevSoI_inDex = src->imgdata[inDex];
    	unsigned char nowSoI_inDex = src->imgdata[inDex + 1];
		/* Check Start of Image */
		if (prevSoI_inDex == 0xFF && nowSoI_inDex == 0xD8)
		{
			/* Check End of Image */
			inDex_Start_Frame = inDex;
        	inDex += 2; /* Skip SoI */
			while ( inDex < src->imgsize - 1 ){
				unsigned char prevEoI_inDex = src->imgdata[inDex];
            	unsigned char nowEoI_inDex = src->imgdata[inDex + 1];

				if (prevEoI_inDex == 0xFF && nowEoI_inDex == 0xD9){
					inDex_End_Frame = inDex + 2; /* EoI point */
					frameSize = inDex_End_Frame - inDex_Start_Frame;
					frameCounter++;

					/* Copy data into buffer */
					int size = min(src->imgsize - inDex_Start_Frame, frameSize);
					size = min(size, buf->size); /* Ensure we don't exceed buffer size */
					memcpy(buf->mem, src->imgdata + inDex_Start_Frame, size);
					buf->bytesused = size;
					// printf("Index %d - Framesize %d\n", buf->index, size);
					
					needFillBuffer = false;

					break; /* Exit loop */
				}
			  inDex++;
			}
		}

	  inDex++; /* Find the next Start of Image */	
	  if (inDex >= src->imgsize)
		{
			inDex = 0;
			needFillBuffer = true;
			continue;
		}
	}
	/*
	 * Wait for the timer to elapse to ensure that our configured frame rate
	 * is adhered to.
	 */
	if (src->streaming)
		timer_wait(src->timer);
}

static const struct video_source_ops jpg_source_ops = {
	.destroy = jpg_source_destroy,
	.set_format = jpg_source_set_format,
	.set_frame_rate = jpg_source_set_frame_rate,
	.alloc_buffers = NULL,
	.export_buffers = NULL,
	.free_buffers = jpg_source_free_buffers,
	.stream_on = jpg_source_stream_on,
	.stream_off = jpg_source_stream_off,
	.queue_buffer = NULL,
	.fill_buffer = jpg_source_fill_buffer,
};

struct video_source *jpg_video_source_create(const char *img_path)
{
	struct jpg_source *src;
	int fd = -1;
	int ret;

	printf("using jpg video source\n");

	if (img_path == NULL)
		return NULL;

	src = malloc(sizeof *src);
	if (!src)
		return NULL;

	memset(src, 0, sizeof *src);
	src->src.ops = &jpg_source_ops;
	src->src.type = VIDEO_SOURCE_STATIC;

	fd = open(img_path, O_RDONLY);
	if (fd == -1) {
		printf("Unable to open MJPEG image '%s'\n", img_path);
		goto err_free_src;
	}

	src->imgsize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	src->imgdata = malloc(src->imgsize);
	if (src->imgdata == NULL) {
		printf("Unable to allocate memory for MJPEG image\n");
		goto err_close_fd;
	}
	/* Read MJPEG file only once and save to memory */
	ret = read(fd, src->imgdata, src->imgsize);
	if (ret < 0) {
		fprintf(stderr, "error reading data from %s: %d\n", img_path, errno);
		goto err_free_imgdata;
	}

	src->timer = timer_new();
	if (!src->timer)
		goto err_free_imgdata;

	close(fd);
	/* Print file description */
	printf("Processing MJPEG file: %s\n", img_path);
    printf("Total file size: %u bytes\n", src->imgsize);
	return &src->src;

err_free_imgdata:
	free(src->imgdata);
err_close_fd:
	close(fd);
err_free_src:
	free(src);

	return NULL;
}

void jpg_video_source_init(struct video_source *s, struct events *events)
{
	struct jpg_source *src = to_jpg_source(s);

	src->src.events = events;
}
