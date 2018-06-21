#ifndef AVBDEFINE_H
#define AVBDEFINE_H

#define DATA_BUFFER_SIZE 1456

#define DEFINE_HEAD 0x10

#define PACKET_TYPE_IMAGE 0x21
#define PACKET_TYPE_AUDIO 0x22
#define PACKET_TYPE_VIDEO 0x23
#define PACKET_TYPE_DATA  0x24

typedef struct _AvbPacket {
    unsigned char sid[4];
    unsigned int timeStamp;
    unsigned char packetType;
    unsigned char seqNumber;
    unsigned short dataSize;
    unsigned char ipAddr[4];
    unsigned short portNum;
    unsigned short reserve;
    unsigned char data[DATA_BUFFER_SIZE];
}AvbPacket;

#endif // AVBDEFINE_H
