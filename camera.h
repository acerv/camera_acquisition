#ifndef CAMERA_H
#define CAMERA_H

/**
 * Copyright by Andrea Cervesato <sawk.ita@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 **/

#include <stddef.h>

// camera abstract data type
typedef struct camera_device_type camera_device;

// the supported camera formats
enum {
    CAMERA_FMT_JMPEG = 0,
    CAMERA_FMT_GREYSCALE,
    CAMERA_FMT_YUYV,
    CAMERA_FMT_UYVY,
    CAMERA_FMT_RGB32
};

// error codes
enum {
    CAMERA_NO_ERROR = 0,
    CAMERA_ERR_DATA_CORRUPTED = -1000,
    CAMERA_ERR_TIMEOUT,
    CAMERA_ERR_MEMORY_HANDLE
};

// this routine prints the camera informations
void camera_print_informations(const char* devstr);

// the device informations and parameters
struct devparams {
    char* devstr;
    int frames;
    int format;
    int width;
    int height;
};

// this routine creates a new camera instance
camera_device* camera_new(struct devparams* params);

// this routine releases the resources used by the camera instance
void camera_free(camera_device** dev_ptr);

// a single frame
typedef struct frame {
    void* start;
    size_t length;
} frame_t;

// This routine returns the frames list 
void camera_get_frame_pointer(camera_device* dev, frame_t** frames_ptr, int* num_of_frames);

// this routine acquires the frame from camera
int camera_acquire_frames(camera_device* dev);

#endif
