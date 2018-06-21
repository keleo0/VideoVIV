#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#ifdef __cplusplus
extern "C"{
#endif

#include "video_defines.h"
#include "linux_header.h"

#include <linux/videodev2.h>
#include <linux/i2c-dev.h>
#include <linux/mxc_v4l2.h>
#include <linux/ipu.h>
#include <linux/mxcfb.h>

#define VIDEO_NAME "/dev/video0"
#define I2C_NAME "/dev/i2c-1"
#define IPU_NAME "/dev/mxc_ipu"

#define VIDEO_WIDTH 720
#define VIDEO_HEIGHT 576
#define VIDEO_CROP_X 0
#define VIDEO_CROP_Y 0
#define VIDEO_CROP_W 720
#define VIDEO_CROP_H 576

typedef struct _CaptureBuffer
{
    unsigned char *start;
    size_t offset;
    unsigned int length;
}CaptureBuffer;

class V4l2_Capture
{
public:
    V4l2_Capture();
    ~V4l2_Capture();

public:
    int32 v4l2_capture_init(VideoFrame *buffer);
    /*crop_x+crop_w =720, crop_y+crop_h=576, x,y,w,h must be 4 bytes for alignment */
    int32 v4l2_capture_init(VideoFrame *buffer, int crop_x, int crop_y, int crop_w, int crop_h);
    void v4l2_capture_destroy();
    void v4l2_capture_clearBuffer();
    int32 v4l2_capture_getframe(VideoFrame *buffer);

private:
    boolean v4l2_capture_starting();
    boolean v4l2_capture_setup();

private:
    //CaptureBuffer capture_buffers[3]; //why here capture_buffers fulsh
    char v4l_capture_dev[100];
    int fd_capture_v4l;
    int g_in_width;
    int g_in_height;
    int g_frame_size;
    int g_capture_num_buffers;
    v4l2_std_id g_current_std;

    int i2c_fd;
    int lost_signal;
    int lost_field;

    int ipu_fd;
    struct ipu_task zxp_ipu_task;
    unsigned char *ipu_invbuf;
    unsigned int ipu_isize;
    unsigned char *ipu_outvbuf;
    unsigned int ipu_osize;

};

#ifdef __cplusplus
}
#endif

#endif // V4L2_CAPTURE_H
