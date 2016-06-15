/**
 * Copyright by Andrea Cervesato <sawk.ita@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 **/

#include <fcntl.h> // open()
#include <unistd.h> // close()
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "camera.h"

// abstract data type definition
struct camera_device_type {
    // the file descriptor
    int file_desc;
    
    // some device informations
    struct v4l2_capability caps;
    struct v4l2_cropcap cropcap;
    struct v4l2_format format;
    
    // the acquisition timeout
    struct timeval timeout;
    
    // data types and format
    int type;
    int memory;

    // memory buffers and registers
    struct v4l2_buffer* buffers;
    frame_t* frames;
    int num_of_frames;
};

static int xioctl(int fd, int request, void* arg)
{
    int ret = 0;
    do { 
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && EINTR == errno);
    return ret;
}

void camera_print_informations(const char* devstr)
{
    struct v4l2_capability      caps;
    struct v4l2_cropcap         cropcap = {0};
    int fd = -1;

    if (devstr == NULL) {
        goto ret_statement;
    }

    fd = open(devstr, O_RDWR);
    if (fd == -1) {
        perror("Opening camera");
        goto ret_statement;
    }
    
    // read camera capabilities
    if (xioctl(fd, VIDIOC_QUERYCAP, &caps) == -1) {
        perror("Reading camera query capabilities");
        goto ret_statement;
    }

    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is not a capture device\n", devstr);
        goto ret_statement;
    }

    printf("\nDriver caps:\n"
           " Driver:\t\"%s\"\n"
           " Card:\t\t\"%s\"\n"
           " Bus:\t\t\"%s\"\n"
           " Version:\t\"%d.%d\"\n",
           caps.driver,
           caps.card,
           caps.bus_info,
           (caps.version>>16)&&0xff,
           (caps.version>>24)&&0xff);
    
    if (caps.capabilities & V4L2_CAP_READWRITE) {
        printf(" Read I/O:\tSupported\n");
    } else {
        printf(" Read I/O:\tNot supported\n");
    }

    if (caps.capabilities & V4L2_CAP_STREAMING) {
        printf(" Streaming:\tSupported\n");
    } else {
        printf(" Streaming:\tNot supported\n");
    }

    // read the cropping capabilities
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;;

    if (xioctl(fd, VIDIOC_CROPCAP, &cropcap) == -1) {
        perror("Reading camera cropping capabilities");
        goto ret_statement;
    }

    printf("\nCamera cropping:\n"
       " Bounds:\t%dx%d+%d+%d\n"
       " Default:\t%dx%d+%d+%d\n"
       " Aspect:\t%d/%d\n",
       cropcap.bounds.width,
       cropcap.bounds.height,
       cropcap.bounds.left,
       cropcap.bounds.top,
       cropcap.defrect.width,
       cropcap.defrect.height,
       cropcap.defrect.left,
       cropcap.defrect.top,
       cropcap.pixelaspect.numerator,
       cropcap.pixelaspect.denominator);

ret_statement:
    if (fd != -1) {
        close(fd);
    }
    printf("\n");
    return;
}

camera_device* camera_new(struct devparams* params)
{
    camera_device* dev = NULL;
    struct v4l2_requestbuffers request = {0};
    struct v4l2_buffer* buffers = {0};
    int fd = 0;
    int i = 0;

    if (params == NULL || params->devstr == NULL) {
        return NULL;
    }
    
    fd = open(params->devstr, O_RDWR);
    if (fd == -1) {
        perror("Opening camera");
        goto cleanup;
    }

    if (params->frames <= 0) {
        params->frames = 1;
    }
        
    // create the camera instance
    dev = (camera_device*)malloc(sizeof(struct camera_device_type));
    dev->file_desc = fd;

    // read camera capabilities
    if (xioctl(fd, VIDIOC_QUERYCAP, &(dev->caps)) == -1) {
        perror("Reading capabilities");
        goto cleanup;
    }

    if (!(dev->caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is not a capture device\n", params->devstr);
        goto cleanup;
    }

    if (!(dev->caps.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "mmap method not supported for this device\n");
        goto cleanup;
    }
        
    // TODO: read the supported camera formats from device
    dev->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    dev->memory = V4L2_MEMORY_MMAP;

    // read the cropping capabilities
    dev->cropcap.type = dev->type;

    if (xioctl(fd, VIDIOC_CROPCAP, &(dev->cropcap)) == -1) {
        perror("Reading cropping capabilities");
        goto cleanup;
    }
        
    // setup the default acquisition format
    dev->format.type = dev->type;
    dev->format.fmt.pix.width = params->width;
    dev->format.fmt.pix.height = params->height;
    dev->format.fmt.pix.field = V4L2_FIELD_NONE;

    switch (params->format) {
        case CAMERA_FMT_JMPEG:
            dev->format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
            break;
        case CAMERA_FMT_YUYV:
            dev->format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
            break;
        case CAMERA_FMT_UYVY:
            dev->format.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
            break;
        case CAMERA_FMT_RGB32:
            dev->format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
            break;
        case CAMERA_FMT_GREYSCALE:
        default:
            dev->format.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
            break;
    }
   
    if (xioctl(fd, VIDIOC_S_FMT, &(dev->format)) == -1) {
        perror("Setting up format");
        goto cleanup;
    }

    // request the buffer
    request.count = params->frames;
    request.type = dev->type;
    request.memory = dev->memory;

    if (xioctl(fd, VIDIOC_REQBUFS, &request) == -1) {
        perror("Requesting buffer");
        goto cleanup;
    }

    if (request.count < (size_t)params->frames) {
        fprintf(stderr, "Can't acquire %d frames\n", params->frames);
        goto cleanup;
    }

    // reserve physical memory on buffers
    buffers = (struct v4l2_buffer*)calloc(params->frames, sizeof(struct v4l2_buffer));
    dev->buffers = (struct v4l2_buffer*)calloc(params->frames, sizeof(struct v4l2_buffer));
    dev->frames = (frame_t*)calloc(params->frames, sizeof(struct frame));
    dev->num_of_frames = params->frames;
        
    // reserve some buffer
    for (i = 0; i < params->frames; i++) {
        buffers[i].type = dev->type;
        buffers[i].memory = dev->memory;
        buffers[i].index = i;

        if (xioctl(fd, VIDIOC_QUERYBUF, &(buffers[i])) == -1) {
            perror("Reserving buffer");
            goto cleanup;
        }

        // map camera buffer. At this point, the device memory
        // is mapped at user space level.
        dev->frames[i].start = mmap(
            NULL, 
            buffers[i].length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            buffers[i].m.offset);

        dev->frames[i].length = buffers[i].length;
    }

    // setup acquisition timeout
    dev->timeout.tv_sec = 2;
    dev->timeout.tv_usec = 0;

    // start frames acquisition
    if (xioctl(dev->file_desc, VIDIOC_STREAMON, &dev->type) == -1) {
        perror("Starting capture");
        goto cleanup;
    }

    // release buffers memory
    free(buffers);

    return dev;

cleanup:
    free(buffers);
    camera_free(&dev);

    return dev;
}

void camera_free(camera_device** device)
{
    camera_device* dev = NULL;
    int i = 0;

    if (device == NULL || (*device) == NULL) {
        return;
    }

    dev = *device;

    // close the device
    if (dev->file_desc != -1) {
        if (xioctl(dev->file_desc, VIDIOC_STREAMOFF, &dev->type) == -1) {
            perror("Closing stream");
        }
        close(dev->file_desc);
    }

    // unmap frames memory
    if (dev->frames != NULL) {
        for (i = 0; i < dev->num_of_frames; i++) {
            if (dev->frames[i].start != NULL) {
                munmap(dev->frames[i].start, dev->frames[i].length);
            }
        }

        free(dev->frames);
    }
     
    // free buffers
    free(dev->buffers);
    free(dev);

    device = NULL;
}

void camera_get_frame_pointer(camera_device* dev, frame_t** frames_ptr, int* num_of_frames)
{
    if (dev == NULL) {
        return;
    }

    *frames_ptr = dev->frames;
    *num_of_frames = dev->num_of_frames;
}

int camera_acquire_frames(camera_device* dev)
{
    int ret = CAMERA_NO_ERROR;
    int i = 0;
    fd_set fds;

    if (!dev) {
        goto ret_statement;
    }
   
    // add buffers in the input queue
    for (i = 0; i < dev->num_of_frames; i++) {
        dev->buffers[i].type = dev->type;
        dev->buffers[i].memory = dev->memory;
        dev->buffers[i].index = i;

        if (xioctl(dev->file_desc, VIDIOC_QBUF, &(dev->buffers[i])) == -1) {
            perror("Querying buffer");
            ret = CAMERA_ERR_MEMORY_HANDLE;
            goto ret_statement;
        }
    }

    // wait for frames acquisition
    FD_ZERO(&fds);
    FD_SET(dev->file_desc, &fds);

    if (select(dev->file_desc + 1, &fds, NULL, NULL, &dev->timeout) == -1) {
        perror("Timeout");
        ret = CAMERA_ERR_TIMEOUT;
        goto ret_statement;
    }

    // the frames have been acquired and they can be removed from the 
    // output queue. Notice that the buffers are cleared
    for (i = 0; i < dev->num_of_frames; i++) {
        if (dev->buffers[i].flags & V4L2_BUF_FLAG_ERROR) {
            fprintf(stderr, "Frame %d memory might be corrupted\n", i);
            ret = CAMERA_ERR_DATA_CORRUPTED;
        }

        if (xioctl(dev->file_desc, VIDIOC_DQBUF, &(dev->buffers[i])) == -1) {
            perror("Retrieving frame");
            ret = CAMERA_ERR_MEMORY_HANDLE;
            goto ret_statement;
        }
    }

ret_statement:
    return ret;
}
