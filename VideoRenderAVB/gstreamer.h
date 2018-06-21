#ifndef GSTREAMER_H
#define GSTREAMER_H

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <linux/videodev2.h>
#include <linux/mxcfb.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <sys/types.h>
#include <dirent.h>


typedef struct _VideoParameter {
    unsigned char ipAddr[4];
    unsigned int mode;
    //unsigned char fileName[64];
    //char *fileList;
    unsigned char inOutFlag;//0:local out display,1:in
    unsigned char enableOutputDisplay;
}VideoParameter;

typedef enum _CaptureMode {
    VIDEO = 0,
    IMAGE = 1,
    AUDIO = 2
}CaptureMode;

typedef struct _CustomData{
    char *fileName;
    CaptureMode mode;
    GstElement *pipeLine;
    GstElement *audioSource;
    GstElement *videoSource;
    GstElement *fileSink;
    GstElement *udpsink;
    GstBus *bus;
    GMainLoop *mainLoop;
    GThread *thread;
}CustomData;

void bus_loop_thread_func();

int capture_set_file_name(char *fileName);

int capture_set_mode(int captureMode);

int pipeline_init(unsigned char mark, VideoParameter param);

int capture_start();

int capture_stop();

void pipeline_destroy();

void getVideoParam(unsigned int mark, unsigned char *data, VideoParameter *param);


#endif // GSTREAMER_H
