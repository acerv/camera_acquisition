
# Camera acquisition library

## Introduction
This is a library that I wrote to play with CMOS cameras such as the Imaging Source DFK72BUC02 that I own.
For now, the program only supports Linux (using Video4Linux library and mmap method), but in the future the goal is to port
the code on Windows as well.

## Usage
The example provides a tester, which can be compiled using make. The tester help is the following:
```
Usage: ./tester [options]

Version 1.0
Options:
-d | --device name   Video device name [default: /dev/video0]
-h | --help          Print this message
-f | --format name   The acquisition format uyvy, yuyv, jmpeg, grey [default: yuyv]
-c | --counts num    The number of frames to acquire [default: 5]
```

## Tests in Linux
To know what resolutions and formats can be supported by the camera in Linux, use the v4l2-ctl tool.
The Imaging Source DFK72BUC02 camera has been tested with the Raspberry Pi 3 device and, according with the 
v4l2-ctl tools, the supported resolutions and format are the following:
* Resolutions:  1296x1944, 1024x768
* Formats:      UYVY, YUYV

## Introduction to the Video4Linux2 library
### From v4l2 documentation (Streaming I/O Memory Mapping chap.3 documentation):
    Streaming is an I/O method where only pointers to buffers are exchanged between application and driver, the data
    itself is not copied. Memory mapping is primarily intended to map buffers in device memory into the application's
    address space. Device memory can be, for example, the video memory on a graphics card with a video capture add-on.
    However, being the most efficient I/O method available for a long time, many other drivers support streaming as
    well, allocating buffers in DMA-able main memory.

### How the streaming buffer works:
    There are two buffer queues: the incoming queue and the outgoing queue. The incoming queue is the one that contains 
    buffers from user space which must be filled during frames acquisition. When a buffer is filled, it's removed from 
    the incoming queue and moved in the outgoing queue. At this point, the buffer can be read at user space.

### ioctl commands explanation:
    VIDIOC_QUERYCAP: it asks for device capabilities
    VIDIOC_S_FMT: it sets the acquisition format
    VIDIOC_REQBUFS: it selects the acquisition method and it sets the number of buffers to acquire.
                    If requested method is not supported, ioctl returns -1 value.
    VIDIOC_QUERYBUF: this command is used to know the acquisition buffer's registers.
                    After the command execution, the v4l2_buffer will contain the "m.offset" and "m.length" values,
                    which can be used to access the memory space with the mmap() routine.
                    NOTE: the buffer is always stored in the physical memory and it can't be swapped. So, call
                    munmap() to release the resources as soon as possible.
    VIDIOC_QBUF/VIDIOC_DQBUF: queue and dequeue a buffer in the input queue of the device.
                    Use VIDIOC_QBUF to enqueue a buffer, then use VIDIOC_DQBUF when the buffer is filled.
                    By using VIDIOC_QUERYBUF command, it's possible to know the status of a buffer in any time.
    VIDIOC_STREAMON/VIDIOC_STREAMOFF: these routines start/stop the streaming communication with the device

### v4l2_capability flags:
    V4L2_CAP_READWRITE: flag that indicates if device supports read() or write() operations. If I/O is supported,
                        then select() and poll() must be supported as well
    V4L2_CAP_STREAMING: flag that indicates if device supports userptr or mmap methods

### v4l2_buffer flags:
    V4L2_BUF_FLAG_MAPPED: the buffer resides in the device memory and it has been mapped in the user space addresses
    V4L2_BUF_FLAG_QUEUED: the buffer is queued in the incoming queue
    V4L2_BUF_FLAG_DONE: the buffer has been moved in the outgoing queue and it has been filled
    V4L2_BUF_FLAG_ERROR: the buffer is correctly acquired, but data can be corrupted

## Userful Links

http://www.theimagingsource.com/en_US/products/cameras/usb-ccd-color/dfk72buc02/
http://linuxtv.org/downloads/v4l-dvb-apis/buffer.html#v4l2-buffer
http://linuxtv.org/downloads/v4l-dvb-apis/mmap.html
https://github.com/torvalds/linux/blob/master/include/uapi/linux/videodev2.h
