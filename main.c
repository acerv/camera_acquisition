/**
 * Copyright by Andrea Cervesato <sawk.ita@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 **/

#include "camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#define DEFAULT_VIDEO "/dev/video0"
#define DEFAULT_FORMAT CAMERA_FMT_RGB32
#define DEFAULT_NUM_OF_FRAMES 5
#define DEFAULT_WIDTH 1024
#define DEFAULT_HEIGHT 768

static const char short_options[] = "d:hf:c:w:v:";
static const struct option 
long_options[] = {
    { "device", required_argument,  NULL,   'd' },
    { "help",   no_argument,        NULL,   'h' },
    { "format", no_argument,        NULL,   'f' },
    { "counts", no_argument,        NULL,   'c' },
    { "width",  no_argument,        NULL,   'w' },
    { "height", no_argument,        NULL,   'v' },
    { 0, 0, 0, 0 }
};

static void usage(char **argv) 
{
    printf("Usage: %s [options]\n\n"
           "Version 1.0\n"
           "Options:\n"
           "-h | --help          Print this message\n"
           "-d | --device name   Video device name [default: %s]\n"
           "-f | --format name   The acquisition format rgb32, uyvy, yuyv, jmpeg, grey [default: %s]\n"
           "-c | --counts num    The number of frames to acquire [default: %d]\n\n"
           "-w | --width  num    The horizontal resolution [default: %d]\n\n"
           "-v | --height num    The vertical resolution [default: %d]\n\n",
           argv[0], 
           DEFAULT_VIDEO, 
           "rgb32",
           DEFAULT_NUM_OF_FRAMES,
           DEFAULT_WIDTH,
           DEFAULT_HEIGHT);
}

int main(int argc, char* argv[])
{
    int i = 0;
    int ret = 0;
    frame_t* frames = NULL;
    int num_of_frames = 0;
    camera_device* dev = NULL;
    struct devparams params = {
        DEFAULT_VIDEO,
        DEFAULT_NUM_OF_FRAMES,
        DEFAULT_FORMAT,
        DEFAULT_WIDTH,
        DEFAULT_HEIGHT
    };
    int idx = 0;
    int c = 0;

    for (;;) {
        c = getopt_long(argc, argv,
            short_options,
            long_options,
            &idx);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                break;
            case 'd':
                params.devstr = optarg;
                break;
            case 'h':
                usage(argv);
                exit(0);
            case 'f':
                if (strcmp(optarg, "jmpeg") == 0) {
                    params.format = CAMERA_FMT_JMPEG;
                } else if (strcmp(optarg, "grey") == 0) {
                    params.format = CAMERA_FMT_GREYSCALE;
                } else if (strcmp(optarg, "yuyv") == 0) {
                    params.format = CAMERA_FMT_YUYV;
                } else if (strcmp(optarg, "uyvy") == 0) {
                    params.format = CAMERA_FMT_UYVY;
                } else if (strcmp(optarg, "rgb32") == 0) {
                    params.format = CAMERA_FMT_RGB32;
                } else {
                    params.format = DEFAULT_FORMAT;
                }
                break;
            case 'c':
                errno = 0;
                params.frames = (int)strtol(optarg, NULL, 0);
                if (errno) {
                    perror("Number of frames");
                    exit(1);
                }
                if (params.frames <= 0) {
                    params.frames = DEFAULT_NUM_OF_FRAMES;
                }
                break;
            case 'w':
                errno = 0;
                params.width = (int)strtol(optarg, NULL, 0);
                if (errno) {
                    perror("Horizontal resolution");
                    exit(1);
                }
                if (params.width <= 0) {
                    params.width = DEFAULT_WIDTH;
                }
                break;
            case 'v':
                errno = 0;
                params.height = (int)strtol(optarg, NULL, 0);
                if (errno) {
                    perror("Vertical resolution");
                    exit(1);
                }
                if (params.height <= 0) {
                    params.height = DEFAULT_HEIGHT;
                }
                break;
            default:
                usage(argv);
                exit(1);
        }
    }

#ifdef DEBUG
    camera_print_informations(params.devstr);
#endif

    dev = camera_new(&params);

    if (dev) {
        camera_get_frame_pointer(dev, &frames, &num_of_frames);

        ret = camera_acquire_frames(dev);
        if (ret != CAMERA_NO_ERROR) {
            if (ret == CAMERA_ERR_DATA_CORRUPTED) {
                fprintf(stderr, "Camera data might be corrupted\n");
                exit(1);
            } else if (ret == CAMERA_ERR_TIMEOUT) {
                fprintf(stderr, "Camera acquisition timeout\n");
                exit(1);
            } else if (ret == CAMERA_ERR_MEMORY_HANDLE) {
                fprintf(stderr, "Camera memory handling error\n");
                exit(1);
            }
        }

        for (i = 0; i < num_of_frames; i++) {
            printf("Frame %d: %dx%d %dbytes\n", 
                i, 
                params.width, 
                params.height, 
                frames[i].length);
        }
        printf("\n");
        camera_free(&dev);
    }

    return 0;
}
