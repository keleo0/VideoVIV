#ifndef DEFINES_H
#define DEFINES_H

#ifndef NULL
#define NULL 0
#endif

typedef unsigned char boolean;

#ifndef TRUE
#define TRUE  ((boolean) 1)
#endif
#ifndef FALSE
#define FALSE ((boolean) 0)
#endif

typedef unsigned short ushort16;
typedef signed short short16;
typedef unsigned int   uint32;
typedef signed int int32;
typedef unsigned long ulong32;
typedef signed long long32;
typedef unsigned long long  ulong64;
typedef signed long long long64;

typedef enum _MainloopState
{
    MAINLOOP_RUN   = 0,
    MAINLOOP_STOP  = 1,
    MAINLOOP_ERROR = 2
}MainloopState;

typedef struct _VideoFrame_
{
    unsigned int   phys_addr;
    void* virtual_addr;
    int width;
    int height;
    int stride;
    int format;
    int size;
}VideoFrame;

#define fourcc(a, b, c, d)\
     (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))

#define VIDEO_FORMAT_I420  fourcc('I', '4', '2', '0')	/*!< 12 YUV 4:2:0 */
#define VIDEO_FORMAT_YV12  fourcc('Y', 'V', '1', '2')	/*!< 12 YVU 4:2:0 */
#define VIDEO_FORMAT_NV12  fourcc('N', 'V', '1', '2') /* 12  Y/CbCr 4:2:0  */
#define VIDEO_FORMAT_YUY2  fourcc('Y', 'U', 'V', '9')	/*!< 9  YUV 4:1:0 */
#define VIDEO_FORMAT_UYVY  fourcc('U', 'Y', 'V', 'Y')	/*!< 16 YUV 4:2:2 */
#define VIDEO_FORMAT_RGB16 fourcc('R', 'G', 'B', 'P')	/*!< 1 6  RGB-5-6-5   */
#define VIDEO_FORMAT_RGBA  fourcc('B', 'G', 'B', 'A')	/*!< 32  BGR-8-8-8-8  */
#define VIDEO_FORMAT_BGRA  fourcc('B', 'G', 'R', 'A')	/*!< 32  BGR-8-8-8-8  */

#endif // DEFINES_H
