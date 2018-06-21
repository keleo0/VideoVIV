#ifndef PACKETDEF_H
#define PACKETDEF_H

#define TOTAL_SIZE_MAX 1460

#define DATA_BUF_SIZE_MAX 1441
#define FIXED_SIZE 19

#define Tranverse16(X) ((((unsigned short)(X) & 0xff00) >> 8) |(((unsigned short)(X) & 0x00ff) << 8))
//#define Tranverse32(X) ((((unsigned int)(X) & 0xff000000) >> 24) | (((unsigned int)(X) & 0x00ff0000) >> 8) | (((unsigned int)(X) & 0x0000ff00) << 8) | (((unsigned int)(X) & 0x000000ff) << 24))

//#define PACKED __attribute__((packed, aligned(1)))
#pragma pack(push,1)
typedef enum _PacketType {
    REQUEST_PACKET = 0x01,
    RETURN_PACKET = 0x02,
    SIMPLEX_PACKET = 0x03
}PacketType;

typedef enum _ActionMark {
    STANDARD_AVB_PING = 0x10,
    TIME_CALIBRATION = 0x11,
    SELF_TEST_REQUEST = 0x20,
    STANDARD_AVB_DISCONNECT = 0x30,
    FORWARDING_DATA_OTHER_AVB = 0x40,
    OPEN_VIDEO_NO_RECORDING = 0x50,
    //OPEN_VIDEO_RECORDING = 0x51,
    CLOSE_VIDEO = 0x53,
    //GET_LOCAL_VIDEO_FILE = 0x52,
    AVB_STANDARD_PING = 0x70,
    OTHER_AVB_FORWARDING_DATA = 0x91,
    ENABLE_DISPLAY = 0x01
}ActionMark;

typedef struct _CustomPacket {
    unsigned short guideHead;//引导头
    unsigned int totalDataBufSize;//所有数据区大小
    unsigned int currentDataBufSize;//当前数据区大小
    unsigned int randomMark;//随机标记
    unsigned int actionMark;//行为标记
    unsigned char type;//包类型
    unsigned char currentDataBuf[DATA_BUF_SIZE_MAX];//当前数据区
}CustomPacket;

typedef struct _ForwardDataPacket {
    unsigned short guideHead;//引导头
    unsigned int totalDataBufSize;//所有数据区大小
    unsigned int currentDataBufSize;//当前数据区大小
    unsigned int randomMark;//随机标记
    unsigned int actionMark;//行为标记
    unsigned char type;//包类型
    unsigned char ipAddr[4];
    unsigned char localIpAddr[4];
    unsigned char outFlag;
    unsigned char mode;
    unsigned char dataBuffer[DATA_BUF_SIZE_MAX];
}ForwardDataPacket;

#pragma pack(pop)


#endif // PACKETDEF_H
