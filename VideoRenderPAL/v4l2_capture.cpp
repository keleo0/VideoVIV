#include "v4l2_capture.h"
static CaptureBuffer capture_buffers[3];
//extern bool g_bClearVideoBuffer;

static inline __s32 i2c_smbus_access(int file, char read_write, __u8 command,
                                     int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(file,I2C_SMBUS,&args);
}

static inline __s32 i2c_smbus_read_byte_data(int file, __u8 command)
{
    union i2c_smbus_data data;
    if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
                         I2C_SMBUS_BYTE_DATA,&data))
        return -1;
    else
        return 0x0FF & data.byte;
}

static inline __s32 i2c_smbus_write_byte_data(int file, __u8 command,
                                              __u8 value)
{
    union i2c_smbus_data data;
    data.byte = value;
    return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
                            I2C_SMBUS_BYTE_DATA, &data);
}


int32 V4l2_Capture::v4l2_capture_getframe(VideoFrame *buffer)
{
    struct v4l2_buffer capture_buf;
    enum v4l2_buf_type type;
    unsigned char i2c_value0x10 =0;

    i2c_value0x10 = i2c_smbus_read_byte_data(i2c_fd, 0x10);

    i2c_value0x10 &=0x0F;

    if((0 ==(0x01 & i2c_value0x10)) && (0 == lost_signal))
    {
        lost_signal = 1;
        printf("i2c read addr 0x10 data 0x%02x, lost signal %d\n",i2c_value0x10, lost_signal);
        i2c_smbus_write_byte_data(i2c_fd, 0x0C, 0x37);  //adv7180 to free-run and render blue screen
        return 1;
    }
    else if((0 != (0x01 & i2c_value0x10)) && (1 == lost_signal))
    {
        lost_signal = 0;
        printf("i2c read addr 0x10 low 4 bit 0x%02x, lost signal %d\n",i2c_value0x10, lost_signal);
        i2c_smbus_write_byte_data(i2c_fd, 0x0C, 0x36); //adv7180 to free-run mode dependent on DEF_VAL_AUTO_EN

#if 1
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd_capture_v4l, VIDIOC_STREAMOFF, &type);

        for (int i = 0; i < g_capture_num_buffers; i++)
        {
            munmap(capture_buffers[i].start, capture_buffers[i].length);
        }

        if (v4l2_capture_setup() != TRUE)
        {
            printf("Setup v4l capture failed.\n");
            close(fd_capture_v4l);
            return -1;
        }

        if (v4l2_capture_starting() != TRUE)
        {
            printf("start_capturing failed\n");
            close(fd_capture_v4l);
            return -1;
        }
#endif
    }
/******zhangke clear video buffer*****
    if(g_bClearVideoBuffer) {
        for (int i = 0; i < g_capture_num_buffers; i++)
        {
            memset(capture_buffers[i].start, 0, capture_buffers[i].length);
        }
        memset(ipu_outvbuf, 0, ipu_osize);
        g_bClearVideoBuffer = false;
    }
*********************************/
    memset(&capture_buf, 0, sizeof(capture_buf));
    capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    capture_buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd_capture_v4l, VIDIOC_DQBUF, &capture_buf) < 0)
    {
        printf("VIDIOC_DQBUF failed.\n");
        return -1;
    }

    memcpy(ipu_invbuf, capture_buffers[capture_buf.index].start, ipu_isize);

    if(ioctl(ipu_fd, IPU_QUEUE_TASK, &zxp_ipu_task) < 0)
    {
       printf("ipu queue task failed\n");
        return -1;
    }

    if(buffer->virtual_addr !=NULL)
        memcpy(buffer->virtual_addr, ipu_outvbuf, ipu_osize);

    if (ioctl(fd_capture_v4l, VIDIOC_QBUF, &capture_buf) < 0)
    {
        printf("VIDIOC_QBUF failed\n");
        return -1;
    }

    return 0;
}

void V4l2_Capture::v4l2_capture_clearBuffer()
{
    for (int i = 0; i < g_capture_num_buffers; i++)
    {
        memset(capture_buffers[i].start, 0, capture_buffers[i].length);
    }
    memset(ipu_outvbuf, 0, ipu_osize);
}

V4l2_Capture::V4l2_Capture()
{
    sprintf(v4l_capture_dev,VIDEO_NAME);
    fd_capture_v4l = 0;
    g_capture_num_buffers = 3;
    g_in_width = 0;
    g_in_height = 0;
    g_frame_size = 0;
    g_current_std = V4L2_STD_PAL;

    i2c_fd = 0;
    lost_signal = 0;
    lost_field =0;

    ipu_fd = 0;
    ipu_invbuf = NULL;
    ipu_isize = 0;
    ipu_outvbuf =0;
    ipu_osize = 0;

}

V4l2_Capture::~V4l2_Capture()
{
}

int32 V4l2_Capture::v4l2_capture_init(VideoFrame *buffer)
{
    /*open i2c device*/
    i2c_fd = open(I2C_NAME, O_RDWR);
    if(i2c_fd < 0)
    {
        printf("open i2c failed\n");
        return -1;
    }

    if(ioctl(i2c_fd, I2C_SLAVE_FORCE, 0x20) < 0)
    {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        goto err0;
    }

    /*open ipu task device*/
    ipu_fd = open(IPU_NAME, O_RDWR, 0);
    if(ipu_fd < 0)
    {
        printf("open ipu fail\n");
        goto err0;
    }

    memset(&zxp_ipu_task, 0, sizeof(zxp_ipu_task));

    zxp_ipu_task.input.width =  VIDEO_WIDTH;
    zxp_ipu_task.input.height = VIDEO_HEIGHT;
    zxp_ipu_task.input.format = v4l2_fourcc('U','Y','V','Y');
    zxp_ipu_task.input.crop.pos.x = VIDEO_CROP_X;
    zxp_ipu_task.input.crop.pos.y = VIDEO_CROP_Y;
    zxp_ipu_task.input.crop.w = VIDEO_CROP_W;
    zxp_ipu_task.input.crop.h = VIDEO_CROP_H;
    zxp_ipu_task.input.deinterlace.enable = 1;
    zxp_ipu_task.input.deinterlace.motion = IPU_DEINTERLACE_MAX_FRAME;

    ipu_isize = zxp_ipu_task.input.paddr = zxp_ipu_task.input.width * zxp_ipu_task.input.height * 2;

    if(ioctl(ipu_fd, IPU_ALLOC, &zxp_ipu_task.input.paddr) < 0)
    {
        printf("ipu input alloc fail\n");
        goto err1;
    }

    ipu_invbuf = (unsigned char *)mmap(0, ipu_isize,
                                       PROT_READ | PROT_WRITE,MAP_SHARED,
                                       ipu_fd, zxp_ipu_task.input.paddr);
    if(!ipu_invbuf)
    {
        printf("ipu input buf mmap fail\n");
        goto err1;
    }

    zxp_ipu_task.output.width =  VIDEO_WIDTH;
    zxp_ipu_task.output.height = VIDEO_HEIGHT;
    zxp_ipu_task.output.format = v4l2_fourcc('U','Y','V','Y');
    zxp_ipu_task.output.rotate = 0;
    ipu_osize = zxp_ipu_task.output.paddr = zxp_ipu_task.output.width * zxp_ipu_task.output.height * 2;

    if(ioctl(ipu_fd, IPU_ALLOC, &zxp_ipu_task.output.paddr) < 0)
    {
        printf("ipu output alloc faile\n");
        goto err1;
    }

    ipu_outvbuf = (unsigned char *)mmap(0, ipu_osize,
                                        PROT_READ | PROT_WRITE,MAP_SHARED,
                                        ipu_fd, zxp_ipu_task.output.paddr);
    if(!ipu_outvbuf)
    {
        printf("ipu output buf mmap fail\n");
        goto err1;
    }

    buffer->phys_addr =zxp_ipu_task.output.paddr;
    buffer->virtual_addr =(void *)ipu_outvbuf;
    buffer->width = zxp_ipu_task.output.width;
    buffer->height = zxp_ipu_task.output.height;
    buffer->stride = zxp_ipu_task.output.width*2;
    buffer->format = zxp_ipu_task.output.format;
    buffer->size = ipu_osize;

    /*open v4l2 capture device*/
    if ((fd_capture_v4l = open(v4l_capture_dev, O_RDWR, 0)) <= 0)
    {
        printf("Unable to open %s\n", v4l_capture_dev);
        goto err1;
    }

    if (v4l2_capture_setup() != TRUE)
    {
        printf("Setup v4l capture failed.\n");
        goto err2;
    }

    if (v4l2_capture_starting() != TRUE)
    {
        printf("start_capturing failed\n");
        goto err2;
    }

    return 0;

err2:
    if(fd_capture_v4l !=0)
        close(fd_capture_v4l);

err1:
    if(ipu_outvbuf !=0)
        munmap(ipu_outvbuf, ipu_osize);

    if(ipu_invbuf !=0)
        munmap(ipu_invbuf, ipu_isize);

    if(ipu_fd !=0)
        close(ipu_fd);

err0:
    if(i2c_fd !=0)
        close(i2c_fd);

    return -1;
}

int32 V4l2_Capture::v4l2_capture_init(VideoFrame *buffer, int crop_x, int crop_y, int crop_w, int crop_h)
{
    /*open i2c device*/
    i2c_fd = open(I2C_NAME, O_RDWR);
    if(i2c_fd < 0)
    {
        printf("open i2c failed\n");
        return -1;
    }

    if(ioctl(i2c_fd, I2C_SLAVE_FORCE, 0x20) < 0)
    {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        goto err0;
    }

    /*open ipu task device*/
    ipu_fd = open(IPU_NAME, O_RDWR, 0);
    if(ipu_fd < 0)
    {
        printf("open ipu fail\n");
        goto err0;
    }

    memset(&zxp_ipu_task, 0, sizeof(zxp_ipu_task));

    zxp_ipu_task.input.width =  VIDEO_WIDTH;
    zxp_ipu_task.input.height = VIDEO_HEIGHT;
    zxp_ipu_task.input.format = v4l2_fourcc('U','Y','V','Y');
    zxp_ipu_task.input.crop.pos.x = crop_x;
    zxp_ipu_task.input.crop.pos.y = crop_y;
    zxp_ipu_task.input.crop.w = crop_w;
    zxp_ipu_task.input.crop.h = crop_h;
    zxp_ipu_task.input.deinterlace.enable = 1;
    zxp_ipu_task.input.deinterlace.motion = IPU_DEINTERLACE_MAX_FRAME;

    ipu_isize = zxp_ipu_task.input.paddr = zxp_ipu_task.input.width * zxp_ipu_task.input.height * 2;

    if(ioctl(ipu_fd, IPU_ALLOC, &zxp_ipu_task.input.paddr) < 0)
    {
        printf("ipu input alloc fail\n");
        goto err1;
    }

    ipu_invbuf = (unsigned char *)mmap(0, ipu_isize,
                                       PROT_READ | PROT_WRITE,MAP_SHARED,
                                       ipu_fd, zxp_ipu_task.input.paddr);
    if(!ipu_invbuf)
    {
        printf("ipu input buf mmap fail\n");
        goto err1;
    }

    zxp_ipu_task.output.width =  VIDEO_WIDTH;
    zxp_ipu_task.output.height = VIDEO_HEIGHT;
    zxp_ipu_task.output.format = v4l2_fourcc('U','Y','V','Y');
    zxp_ipu_task.output.rotate = 0;
    ipu_osize = zxp_ipu_task.output.paddr = zxp_ipu_task.output.width * zxp_ipu_task.output.height * 2;

    if(ioctl(ipu_fd, IPU_ALLOC, &zxp_ipu_task.output.paddr) < 0)
    {
        printf("ipu output alloc faile\n");
        goto err1;
    }

    ipu_outvbuf = (unsigned char *)mmap(0, ipu_osize,
                                        PROT_READ | PROT_WRITE,MAP_SHARED,
                                        ipu_fd, zxp_ipu_task.output.paddr);
    if(!ipu_outvbuf)
    {
        printf("ipu output buf mmap fail\n");
        goto err1;
    }

    buffer->phys_addr =zxp_ipu_task.output.paddr;
    buffer->virtual_addr =(void *)ipu_outvbuf;
    buffer->width = zxp_ipu_task.output.width;
    buffer->height = zxp_ipu_task.output.height;
    buffer->stride = zxp_ipu_task.output.width*2;
    buffer->format = zxp_ipu_task.output.format;
    buffer->size = ipu_osize;

    /*open v4l2 capture device*/
    if ((fd_capture_v4l = open(v4l_capture_dev, O_RDWR, 0)) <= 0)
    {
        printf("Unable to open %s\n", v4l_capture_dev);
        goto err1;
    }

    if (v4l2_capture_setup() != TRUE)
    {
        printf("Setup v4l capture failed.\n");
        goto err2;
    }

    if (v4l2_capture_starting() != TRUE)
    {
        printf("start_capturing failed\n");
        goto err2;
    }

    return 0;

err2:
    if(fd_capture_v4l !=0)
        close(fd_capture_v4l);

err1:
    if(ipu_outvbuf !=0)
        munmap(ipu_outvbuf, ipu_osize);

    if(ipu_invbuf !=0)
        munmap(ipu_invbuf, ipu_isize);

    if(ipu_fd !=0)
        close(ipu_fd);

err0:
    if(i2c_fd !=0)
        close(i2c_fd);

    return -1;
}

void V4l2_Capture::v4l2_capture_destroy(void)
{
    int i =0;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(i2c_fd !=0)
        close(i2c_fd);

    if(ipu_outvbuf !=0)
        munmap(ipu_outvbuf, ipu_osize);

    if(ipu_invbuf !=0)
        munmap(ipu_invbuf, ipu_isize);

    if(ipu_fd !=0)
        close(ipu_fd);

    if(ioctl(fd_capture_v4l, VIDIOC_STREAMOFF, &type) <0)
    {
        printf("VIDIOC_STREAMOFF error\n");
    }

    for (i = 0; i < g_capture_num_buffers; i++)
    {
        munmap(capture_buffers[i].start, capture_buffers[i].length);
    }

    close(fd_capture_v4l);


    return;
}

boolean V4l2_Capture::v4l2_capture_setup()
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_dbg_chip_ident chip;
    struct v4l2_streamparm parm;
    v4l2_std_id id;
    unsigned int min;
    int g_input =1;

    if (ioctl (fd_capture_v4l, VIDIOC_QUERYCAP, &cap) < 0)
    {
        if (EINVAL == errno)
        {
            fprintf (stderr, "%s is no V4L2 device\n",v4l_capture_dev);
            return FALSE;
        }
        else
        {
            fprintf (stderr, "%s isn not V4L device,unknow error\n",v4l_capture_dev);
            return FALSE;
        }
    }

   /* printf("driver:%s\n"
           "card:%s\n"
           "bus_info:%s\n"
           "version:%08x\n"
           "capabilities:%08x\n"
           "\n"
           "\n",cap.driver,cap.card,cap.bus_info,cap.version,cap.capabilities);
*/

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf (stderr, "%s is no video capture device\n",v4l_capture_dev);
        return FALSE;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        fprintf (stderr, "%s does not support streaming i/o\n", v4l_capture_dev);
        return FALSE;
    }

    if (ioctl(fd_capture_v4l, VIDIOC_DBG_G_CHIP_IDENT, &chip))
    {
        printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
        return FALSE;
    }

    printf("TV decoder chip is %s\n", chip.match.name);

    if (ioctl(fd_capture_v4l, VIDIOC_S_INPUT, &g_input) < 0)
    {
        printf("VIDIOC_S_INPUT failed\n");
        return FALSE;
    }

    if (ioctl(fd_capture_v4l, VIDIOC_G_STD, &id) < 0)
    {
        printf("VIDIOC_G_STD failed\n");
        return FALSE;
    }

    //printf("std = %x, g_current_std = %x\n",id, g_current_std);
     g_current_std = id;

    if (ioctl(fd_capture_v4l, VIDIOC_S_STD, &id) < 0)
    {
        printf("VIDIOC_S_STD failed\n");
        return FALSE;
    }

    /* Select video input, video standard and tune here. */

    memset(&cropcap, 0, sizeof(cropcap));

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl (fd_capture_v4l, VIDIOC_CROPCAP, &cropcap) < 0)
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */
        if (ioctl (fd_capture_v4l, VIDIOC_S_CROP, &crop) < 0)
        {
            switch (errno)
            {
                case EINVAL:
                    /* Cropping not supported. */
                    fprintf (stderr, "%s  doesn't support crop\n",
                        v4l_capture_dev);
                    break;
                default:
                    /* Errors ignored. */
                    break;
            }
        }
    }
    else
    {
        /* Errors ignored. */
    }
/*
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c.top = 0;
    crop.c.left = 0;
    crop.c.width = 720;
    crop.c.height = 576;
    if(ioctl(fd_capture_v4l, VIDIOC_S_CROP, &crop) < 0)
    {
        printf("set crop failed\n");
        close(fd_capture_v4l);
        return FALSE;
    }
    */
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = 25;
    parm.parm.capture.capturemode = V4L2_MODE_HIGHQUALITY;

    if (ioctl(fd_capture_v4l, VIDIOC_S_PARM, &parm) < 0)
    {
        printf("VIDIOC_S_PARM failed\n");
        return FALSE;
    }

    if (ioctl(fd_capture_v4l, VIDIOC_G_PARM, &parm) < 0)
    {
        printf("VIDIOC_G_PARM failed\n");
        return FALSE;
    }

    if(parm.parm.capture.capability == V4L2_CAP_TIMEPERFRAME)
    {
        printf("timeperframe field is supported\n");
    }

    /*
    printf("capability:%02x\n"
           "capturemode:%02x\n"
           "numerator:%02x\n"
           "denominator:%02x\n"
           "extendedmode:%02x\n"
           "readbuffers:%02x\n"
           "output numerator:%02x\n"
           "output denominator:%02x\n"
           "\n",parm.parm.capture.capability,
           parm.parm.capture.capturemode,
           parm.parm.capture.timeperframe.numerator,
           parm.parm.capture.timeperframe.denominator,
           parm.parm.capture.extendedmode,
           parm.parm.capture.readbuffers,
           parm.parm.output.timeperframe.numerator,
           parm.parm.output.timeperframe.denominator);
    */
    memset(&fmt, 0, sizeof(fmt));

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = VIDEO_WIDTH;
    fmt.fmt.pix.height      = VIDEO_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (ioctl (fd_capture_v4l, VIDIOC_S_FMT, &fmt) < 0){
        fprintf (stderr, "%s iformat not supported \n",
            v4l_capture_dev);
        return FALSE;
    }

    /* Note VIDIOC_S_FMT may change width and height. */

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;

    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    if (ioctl(fd_capture_v4l, VIDIOC_G_FMT, &fmt) < 0)
    {
        printf("VIDIOC_G_FMT failed\n");
        return FALSE;
    }
    /*printf("width = %d\n"
           "height = %d\n"
           "pixelformat = %d\n"
           "filed = %d\n"
           "bytesperline = %d\n"
           "sizeimage = %d\n"
           "colorspace = %d\n"open
           "priv = %d\n",fmt.fmt.pix.width,
           fmt.fmt.pix.height,
           fmt.fmt.pix.pixelformat,
           fmt.fmt.pix.field,
           fmt.fmt.pix.bytesperline,
           fmt.fmt.pix.sizeimage,
           fmt.fmt.pix.colorspace,
           fmt.fmt.pix.priv);*/

    g_in_width = fmt.fmt.pix.width;
    g_in_height = fmt.fmt.pix.height;
    g_frame_size = fmt.fmt.pix.sizeimage;

  //  printf("g_in_width = %d, g_in_height = %d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    memset(&req, 0, sizeof (req));

    req.count               = g_capture_num_buffers;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_MMAP;

    if (ioctl (fd_capture_v4l, VIDIOC_REQBUFS, &req) < 0)
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s does not support memory mapping\n", v4l_capture_dev);
            return FALSE;
        }
        else
        {
            fprintf(stderr, "%s does not support memory mapping, unknow error\n", v4l_capture_dev);
            return FALSE;
        }
    }

    if (req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n", v4l_capture_dev);
        return FALSE;
    }

    return TRUE;
}

boolean V4l2_Capture::v4l2_capture_starting()
{
    int i;
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;

    for (i = 0; i < g_capture_num_buffers; i++)
    {
        memset(&buf, 0, sizeof (buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd_capture_v4l, VIDIOC_QUERYBUF, &buf) < 0)
        {
            printf("VIDIOC_QUERYBUF error\n");
            return FALSE;
        }

        capture_buffers[i].length = buf.length;
        capture_buffers[i].offset = (size_t) buf.m.offset;
        capture_buffers[i].start = (unsigned char *)mmap (NULL, capture_buffers[i].length,
                                                          PROT_READ | PROT_WRITE, MAP_SHARED,
                                                          fd_capture_v4l,
                                                          capture_buffers[i].offset);
        memset(capture_buffers[i].start, 0xFF, capture_buffers[i].length);
    }

    for (i = 0; i < g_capture_num_buffers; i++)
    {
        memset(&buf, 0, sizeof (buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.m.offset = capture_buffers[i].offset;
        if (ioctl (fd_capture_v4l, VIDIOC_QBUF, &buf) < 0)
        {
            printf("VIDIOC_QBUF error\n");
            return FALSE;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl (fd_capture_v4l, VIDIOC_STREAMON, &type) < 0)
    {
        printf("VIDIOC_STREAMON error\n");
        return FALSE;
    }

    return TRUE;
}
