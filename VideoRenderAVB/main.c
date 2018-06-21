#include "packetDef.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "interfaceDef.h"
#include "gstreamer.h"

#define DEBUG_MSG 1

#define MAX_LINE 1024
//#define FIFO_NAME "/myfifo"

VideoParameter videoParam;
VideoParameter dispParam;
CustomPacket *recvPacket;
CustomPacket sendPacket;
int linkId = 0;
int sendLen = 0;
ForwardDataPacket *forwardDataPacket;
ForwardDataPacket recvForwardPacket;
struct sockaddr_in udpClientAddr;
int udpClientSocket;
char testBuffer[1024] = "ExileRiven";
unsigned int udpClientAddrLen = 0;
int n;
int t = 0;
unsigned char threadRunFlag = 0;
unsigned char runFlag = 0;
char strcatbuffer[32];

int timeout_count = 0;
unsigned char closeLinkIdFlag = 0;

void usage(char *command)
{
    printf("usage : %s portnum filename\n", command);
    exit(0);
}

void timeout_disconnect(int signo)
{
    if((1 == closeLinkIdFlag) && (linkId > 0)) {
        printf("timemout disconnect!\n");
        closeLinkIdFlag = 0;
        close(linkId);
    }
}

void *thread_function(void *arg)
{
    while(1)
    {
        n = recvfrom(udpClientSocket, &recvForwardPacket, sizeof(recvForwardPacket), 0, (struct sockaddr *)&udpClientAddr, &udpClientAddrLen);
        if(n > 0)
        {
#if DEBUG_MSG
            printf("--------------------thread recvfrom start--------------------\n");
            printf("recvForwardPacket->guideHead : 0x%x\n",recvForwardPacket.guideHead);
            printf("recvForwardPacket->totalDataBufSize : 0x%x\n",recvForwardPacket.totalDataBufSize);
            printf("recvForwardPacket->currentDataBufSize : 0x%x\n",recvForwardPacket.currentDataBufSize);
            printf("recvForwardPacket->randomMark : 0x%x\n",recvForwardPacket.randomMark);
            printf("recvForwardPacket->actionMark : 0x%x\n",recvForwardPacket.actionMark);
            printf("recvForwardPacket->type : 0x%x\n",recvForwardPacket.type);
            printf("recvForwardPacket->ipAddr : %d.%d.%d.%d\n", recvForwardPacket.ipAddr[0], recvForwardPacket.ipAddr[1]
                   , recvForwardPacket.ipAddr[2], recvForwardPacket.ipAddr[3]);
            printf("recvForwardPacket->localIpAddr : %d.%d.%d.%d\n", recvForwardPacket.localIpAddr[0],
                   recvForwardPacket.localIpAddr[1], recvForwardPacket.localIpAddr[2], recvForwardPacket.localIpAddr[3]);

            printf("recvForwardPacket->mode : 0x%x\n", recvForwardPacket.mode);
            printf("recvForwardPacket->outFlag : 0x%x\n", recvForwardPacket.outFlag);
            //printf("forwardDataPacket->outFlag : 0x%x\n", forwardDataPacket-);

            for(t = 0; t < 50; t++)
            {
                printf("0x%02x ", recvForwardPacket.dataBuffer[t]);
                if((t > 0) && ((t % 20) == 0))
                    printf("\n");
            }
            printf("\n");

            printf("---------------------thread recvfrom end---------------------\n");
#endif
            if(recvForwardPacket.actionMark == FORWARDING_DATA_OTHER_AVB)
            {
#if DEBUG_MSG
                printf("thread send.....\n");
#endif
                sendPacket.guideHead = recvForwardPacket.guideHead;//引导头
                sendPacket.totalDataBufSize = recvForwardPacket.totalDataBufSize;//所有数据区大小
                sendPacket.currentDataBufSize = recvForwardPacket.currentDataBufSize;//当前数据区大小
                sendPacket.randomMark = recvForwardPacket.randomMark;//随机标记
                sendPacket.actionMark = OTHER_AVB_FORWARDING_DATA;//行为标记
                sendPacket.type = RETURN_PACKET;//包类型

                sprintf(strcatbuffer, "%s", inet_ntoa(udpClientAddr.sin_addr));
                for(t = 0; t < (int)strlen(strcatbuffer); t++)
                {
                    recvForwardPacket.dataBuffer[t] = strcatbuffer[t];
                }
                //sprintf(recvForwardPacket.dataBuffer, "%s", strcatbuffer);
                //memcpy(sendPacket.currentDataBuf, strcatbuffer, 12);
                memcpy(sendPacket.currentDataBuf, recvForwardPacket.dataBuffer, DATA_BUF_SIZE_MAX);

                sendLen = send(linkId, &sendPacket, sizeof(sendPacket), 0);
                if(sendLen < 0)
                {
                    perror("thread send error.\n");
                }
            }
            else if(recvForwardPacket.actionMark == ENABLE_DISPLAY)
            {
                if((recvForwardPacket.outFlag == 0x01) && (threadRunFlag == 0))
                {
                    printf("thread gstreamer start.....\n");
                    videoParam.mode = recvForwardPacket.mode;
                    videoParam.enableOutputDisplay = recvForwardPacket.outFlag;
                    recvPacket->actionMark = ENABLE_DISPLAY;
                    if(pipeline_init(recvForwardPacket.actionMark, videoParam) != 0)
                    {
                        printf("ERROR : pipeline initial failed!\n");
                    }

                    if(capture_start() != 0)
                    {
                        printf("ERROR : pipeline start failed!\n");
                    }
                    threadRunFlag = 1;
                }
            }
            else if(recvForwardPacket.actionMark == CLOSE_VIDEO)
            {
                if((recvForwardPacket.outFlag == 0x02) && (threadRunFlag == 1))
                {
                    printf("thread gstreamer stop.....\n");
                    if(capture_stop() != 0)
                    {
                        printf("ERROR : pipeline stop failed!\n");
                    }

                    pipeline_destroy();
                    threadRunFlag = 0;
                }

            }
            memset(&recvForwardPacket, 0x00, sizeof(recvForwardPacket));
        }

    }
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int socketId;
    int recvLen;
    unsigned int clientAddrLen;

    pthread_t pid;
    int result;

    timer_t zxpTime;
    unsigned char tempbuffer[TOTAL_SIZE_MAX];

    unsigned char openFlag = 0;

    int i = 0;

    char ipTemp[32];
    char ethMacStr[32];

    /******************timer count*****************/
    struct itimerval tick;

    signal(SIGALRM, timeout_disconnect);
    memset(&tick, 0, sizeof(tick));

    tick.it_value.tv_sec = 30;
    tick.it_value.tv_usec = 0;
    tick.it_interval.tv_sec = 30;
    tick.it_interval.tv_usec = 0;

//    if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
//        printf("Set timer failed!\n");
//    else
//        printf("Set timer success!\n");

    memset(strcatbuffer, 0x00, sizeof(strcatbuffer));

    if(argc != 3)
    {
        usage(argv[0]);
    }

    sprintf(ethMacStr, "ifconfig eth0 hw ether 1E:ED:19:27:1A:%x", getLocalIp() >> 24);
    if((system(ethMacStr)) < 0)
    {
        printf("set eth0 hw mac addr error.\n");
    }
    printf("eth hw mac addr : %s\n", ethMacStr);

    recvPacket = (CustomPacket *)malloc(sizeof(CustomPacket));
    if(recvPacket)
        printf("RecvPacket Mallock success.\n");

    forwardDataPacket = (ForwardDataPacket *)malloc(sizeof(ForwardDataPacket));
    if(forwardDataPacket)
        printf("Udp Packet Mallock success.\n");

    memset(&dispParam, 0x00, sizeof(dispParam));
    memset(&videoParam, 0x00, sizeof(videoParam));
    memset(tempbuffer, 0x00, sizeof(tempbuffer));
    memset(recvPacket, 0x00, sizeof(recvPacket));
    memset(&sendPacket, 0x00, sizeof(sendPacket));
    memset(&recvForwardPacket, 0x00, sizeof(recvForwardPacket));

    if((socketId = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Create socket failed\n");
        exit(0);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1]));
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(socketId, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Bind socket failed\n");
        exit(0);
    }

    if(-1 == listen(socketId, 10))
    {
        perror("Listen socket failed\n");
        exit(0);
    }

    /******************udp client*****************/
    if((udpClientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("create udp client socket failed.\n");
        exit(1);
    }

//
    udpClientAddr.sin_family = AF_INET;
    udpClientAddr.sin_port = htons(atoi(argv[2]));
    udpClientAddr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr(argv[2]);

    if (bind(udpClientSocket, (struct sockaddr *)&udpClientAddr, sizeof(udpClientAddr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    udpClientAddrLen = sizeof(udpClientAddr);

    printf("-------------------- thread --------------------------\n");
    result = pthread_create(&pid, NULL, thread_function, NULL);
    if(result != 0)
    {
        perror("Thread create failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for thread to finish...\n");

    while(1)
    {
        clientAddrLen = sizeof(clientAddr);
        linkId = accept(socketId, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if(-1 == linkId)
        {
            perror("Accept socket failed\n");
            exit(0);
        }
        else
        {
            printf("\nConnect ...%d\n", linkId);
        }

        while((recvLen = recv(linkId, tempbuffer, sizeof(tempbuffer), 0)) > 0)
        {
//            recvLen = recv(linkId, tempbuffer, sizeof(tempbuffer), 0);
//            if(recvLen <= 0)
//            {
//                printf("Recieve Data From Server Failed\n");
//                break;
//            }
#if DEBUG_MSG
            printf("-------------------------Tcp Recv Info Start-------------------------------\n");
            printf("recvLen = %d, %d\n", recvLen, sizeof(tempbuffer));

            for(i = 0; i < recvLen; i++)
            {
                printf("0x%02x ", tempbuffer[i]);
                if((i > 0) && ((i % 20) == 0))
                    printf("\n");
            }
            printf("\n");

            printf("--------------------------Tcp Recv Info End--------------------------------\n");
#endif
            recvPacket = (CustomPacket *)&tempbuffer[0];

            //recvPacket->guideHead = Tranverse16(recvPacket->guideHead);

#if DEBUG_MSG
            printf("引导头 : 0x%04x\n", recvPacket->guideHead);
            printf("所有数据区大小 : 0x%08x\n", recvPacket->totalDataBufSize);
            printf("当前数据区大小 : 0x%08x\n", recvPacket->currentDataBufSize);
            printf("随机标记 : 0x%08x\n", recvPacket->randomMark);
            printf("行为标记 : 0x%08x\n", recvPacket->actionMark);
            printf("包类型 : 0x%02x\n", recvPacket->type);
            printf("数据区 : %s\n", recvPacket->currentDataBuf);
#endif
            switch (recvPacket->actionMark) {
            case STANDARD_AVB_PING:
                switch(recvPacket->type) {
                case REQUEST_PACKET:

                    if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
                        printf("Set timeout failed!\n");
                    else
                        printf("Set timeout success!\n");

                    closeLinkIdFlag = 1;

                    printf("***************TEST PING***************\n");
                    sendPacket.guideHead = recvPacket->guideHead;//引导头
                    sendPacket.randomMark = recvPacket->randomMark;//随机标记
                    sendPacket.actionMark = recvPacket->actionMark;//行为标记
                    sendPacket.type = RETURN_PACKET;//包类型

                    testPing(&zxpTime);
                    printf("Timestamp : %d\n", zxpTime);
                    sprintf(sendPacket.currentDataBuf, "%d", zxpTime);
                    printf("timestamp buffer : %s\n", sendPacket.currentDataBuf);
                    sendPacket.currentDataBufSize = strlen(sendPacket.currentDataBuf);
                    sendPacket.totalDataBufSize = sendPacket.currentDataBufSize;// + FIXED_SIZE;
#if DEBUG_MSG
                    printf("引导头 : 0x%04x\n", sendPacket.guideHead);
                    printf("所有数据区大小 : 0x%08x\n", sendPacket.totalDataBufSize);
                    printf("当前数据区大小 : 0x%08x\n", sendPacket.currentDataBufSize);
                    printf("随机标记 : 0x%08x\n", sendPacket.randomMark);
                    printf("行为标记 : 0x%08x\n", sendPacket.actionMark);
                    printf("包类型 : 0x%02x\n", sendPacket.type);
                    printf("数据区 : %s\n", sendPacket.currentDataBuf);
#endif
                    //memcpy(sendPacket.currentDataBuf, recvPacket->currentDataBuf, DATA_BUF_SIZE_MAX);
                    sendLen = send(linkId, &sendPacket, sizeof(sendPacket), 0);
                    printf("sendLen = %d\n", sendLen);
                    if(sendLen < 0)
                    {
                        perror("return test ping failed\n");
                        //exit(0);
                    }

//                    if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
//                        printf("Set timer failed!\n");
//                    else
//                        printf("Set timer success!\n");

                    break;
                case RETURN_PACKET:
                    break;
                case SIMPLEX_PACKET:
                    break;
                default:
                    break;
                }

                break;
            case TIME_CALIBRATION:
                switch(recvPacket->type) {
                case REQUEST_PACKET:

                    if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
                        printf("Set timeout failed!\n");
                    else
                        printf("Set timeout success!\n");

                    closeLinkIdFlag = 1;

                    printf("***************TIME CALIBRATION***************\n");
#if DEBUG_MSG
                    printf("引导头 : 0x%04x\n", recvPacket->guideHead);
                    printf("所有数据区大小 : 0x%08x\n", recvPacket->totalDataBufSize);
                    printf("当前数据区大小 : 0x%08x\n", recvPacket->currentDataBufSize);
                    printf("随机标记 : 0x%08x\n", recvPacket->randomMark);
                    printf("行为标记 : 0x%08x\n", recvPacket->actionMark);
                    printf("包类型 : 0x%02x\n", recvPacket->type);
                    printf("数据区 : %s\n", recvPacket->currentDataBuf);
#endif

                    sendPacket.guideHead = recvPacket->guideHead;//引导头
                    sendPacket.randomMark = recvPacket->randomMark;//随机标记
                    sendPacket.actionMark = recvPacket->actionMark;//行为标记
                    sendPacket.type = RETURN_PACKET;//包类型

                    //printf("-------------------------Send Info-------------------------------\n");
                    if(timeCalibration(recvPacket->currentDataBuf) == 0)
                    {
                        sprintf(sendPacket.currentDataBuf, "OK");
                    }
                    else
                    {
                        sprintf(sendPacket.currentDataBuf, "Time calibration failed ...");
                    }


                    sendPacket.currentDataBufSize = strlen(sendPacket.currentDataBuf);//recvPacket->currentDataBufSize;
                    sendPacket.totalDataBufSize = sendPacket.currentDataBufSize;

#if DEBUG_MSG
                    printf("引导头 : 0x%04x\n", sendPacket.guideHead);
                    printf("所有数据区大小 : 0x%08x\n", sendPacket.totalDataBufSize);
                    printf("当前数据区大小 : 0x%08x\n", sendPacket.currentDataBufSize);
                    printf("随机标记 : 0x%08x\n", sendPacket.randomMark);
                    printf("行为标记 : 0x%08x\n", sendPacket.actionMark);
                    printf("包类型 : 0x%02x\n", sendPacket.type);
                    printf("数据区 : %s\n", sendPacket.currentDataBuf);
#endif
                    //memcpy(sendPacket.currentDataBuf, recvPacket->currentDataBuf, DATA_BUF_SIZE_MAX);
                    sendLen = send(linkId, &sendPacket, sizeof(sendPacket), 0);
                    if(sendLen < 0)
                    {
                        perror("return time calibration failed\n");
                        //exit(0);
                    }

                    //printf("recv current buf : %s\n", recvPacket.currentDataBuf);

                    break;
                case RETURN_PACKET:
                    break;
                case SIMPLEX_PACKET:
                    break;
                default:
                    break;
                }
                break;
            case SELF_TEST_REQUEST:
                switch(recvPacket->type) {
                case REQUEST_PACKET:
                    if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
                        printf("Set timeout failed!\n");
                    else
                        printf("Set timeout success!\n");

                    closeLinkIdFlag = 1;

                    printf("***************SELF TEST REQUEST***************\n");
                    sendPacket.guideHead = recvPacket->guideHead;//引导头
                    sendPacket.randomMark = recvPacket->randomMark;//随机标记
                    sendPacket.actionMark = recvPacket->actionMark;//行为标记
                    sendPacket.type = RETURN_PACKET;//包类型

                    if(selfTestRequest() == 0)
                    {
                        sprintf(sendPacket.currentDataBuf, "OK");
                    }
                    else
                    {
                        sprintf(sendPacket.currentDataBuf, "Self test failed ...");
                    }

                    //printf("buffer : %s\n", sendPacket.currentDataBuf);
                    sendPacket.currentDataBufSize = strlen(sendPacket.currentDataBuf);//recvPacket->currentDataBufSize;
                    sendPacket.totalDataBufSize = sendPacket.currentDataBufSize;
                    sendLen = send(linkId, &sendPacket, sizeof(sendPacket), 0);
                    if(sendLen < 0)
                    {
                        perror("return self test request failed\n");
                        //exit(0);
                    }
                    break;
                case RETURN_PACKET:
                    break;
                case SIMPLEX_PACKET:
                    break;
                default:
                    break;
                }
                break;
            case STANDARD_AVB_DISCONNECT:
                printf("***************STANDARD AVB DISCONNECT***************\n");
                printf("action mark : 0x%02x\n", recvPacket->actionMark);
                closeLinkIdFlag = 0;
                printf("\ndisconnect ...\n");
                close(linkId);
                break;
            case FORWARDING_DATA_OTHER_AVB:
                switch(recvPacket->type) {
                case REQUEST_PACKET:
                    if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
                        printf("Set timeout failed!\n");
                    else
                        printf("Set timeout success!\n");

                    closeLinkIdFlag = 1;

                    printf("***************FORWARDING DATA OTHER AVB***************\n");
#if DEBUG_MSG
                    printf("Forwarding data other avb recv buffer : %s\n", recvPacket->currentDataBuf);

                    for(i = 0; i < 50; i++)
                    {
                        if(i % 20 == 0)
                            printf("\n");
                        printf(" 0x%02x ", recvPacket->currentDataBuf[i]);
                    }
                    printf("\n");
#endif
                    if(getForwardingData(recvPacket->currentDataBuf, sizeof(recvPacket->currentDataBuf), forwardDataPacket) == 0)
                    {

                        if((0 == forwardDataPacket->ipAddr[0]) && (0 == forwardDataPacket->ipAddr[1]) &&
                                (0 == forwardDataPacket->ipAddr[2]) && (0 == forwardDataPacket->ipAddr[3]))
                        {
                            printf("forward data to local......\n");
                            sendPacket.guideHead = recvPacket->guideHead;//引导头
                            sendPacket.randomMark = recvPacket->randomMark;//随机标记
                            sendPacket.actionMark = OTHER_AVB_FORWARDING_DATA;//行为标记
                            sendPacket.type = RETURN_PACKET;//包类型
                            sendPacket.currentDataBufSize = recvPacket->currentDataBufSize;
                            sendPacket.totalDataBufSize = recvPacket->totalDataBufSize;

                            printf("forward data to sendPacket.guideHead : 0x%x\n", sendPacket.guideHead);
                            printf("forward data to sendPacket.guideHead : 0x%x\n", sendPacket.randomMark);

                            memcpy(sendPacket.currentDataBuf, recvPacket->currentDataBuf, DATA_BUF_SIZE_MAX);
                            sendLen = send(linkId, &sendPacket, sizeof(sendPacket), 0);
                            if(sendLen < 0)
                            {
                                perror("return forwarding data other avb failed\n");
                                //exit(0);
                            }
                        }
                        if((getLocalIp() >> 24) == forwardDataPacket->ipAddr[3])
                        {
                            printf("forward data to local......\n");
                            sendPacket.guideHead = recvPacket->guideHead;//引导头
                            sendPacket.randomMark = recvPacket->randomMark;//随机标记
                            sendPacket.actionMark = OTHER_AVB_FORWARDING_DATA;//行为标记
                            sendPacket.type = RETURN_PACKET;//包类型
                            sendPacket.currentDataBufSize = recvPacket->currentDataBufSize;
                            sendPacket.totalDataBufSize = recvPacket->totalDataBufSize;

                            printf("forward data to sendPacket.guideHead : 0x%x\n", sendPacket.guideHead);
                            printf("forward data to sendPacket.guideHead : 0x%x\n", sendPacket.randomMark);

                            memcpy(sendPacket.currentDataBuf, recvPacket->currentDataBuf, DATA_BUF_SIZE_MAX);
                            sendLen = send(linkId, &sendPacket, sizeof(sendPacket), 0);
                            if(sendLen < 0)
                            {
                                perror("return forwarding data other avb failed\n");
                                //exit(0);
                            }
                        }
                        else
                        {
                            sprintf(ipTemp, "%d.%d.%d.%d", forwardDataPacket->ipAddr[0], forwardDataPacket->ipAddr[1],
                                    forwardDataPacket->ipAddr[2], forwardDataPacket->ipAddr[3]);

                            printf("ipTemp : %s\n", ipTemp);

                            udpClientAddr.sin_family = AF_INET;
                            udpClientAddr.sin_port = htons(atoi(argv[2]));
                            udpClientAddr.sin_addr.s_addr = inet_addr(ipTemp);

                            forwardDataPacket->guideHead = recvPacket->guideHead;
                            forwardDataPacket->randomMark = recvPacket->randomMark;
                            forwardDataPacket->actionMark = FORWARDING_DATA_OTHER_AVB;
                            forwardDataPacket->type = RETURN_PACKET;


                            forwardDataPacket->totalDataBufSize = recvPacket->totalDataBufSize;//所有数据区大小
                            forwardDataPacket->currentDataBufSize = recvPacket->currentDataBufSize;//当前数据区大小


                            //inet_ntoa(clientAddr.sin_addr);

                            //(strcatbuffer, "%s", inet_ntoa(clientAddr.sin_addr));

//                            int m = 0;
//                            for(m = 0; m < 13; m++)
//                            {
//                                recvPacket->currentDataBuf[m] = strcatbuffer[m];
//                            }

                            memcpy(forwardDataPacket->dataBuffer, recvPacket->currentDataBuf, DATA_BUF_SIZE_MAX);

                            if((sendto(udpClientSocket, forwardDataPacket, sizeof(ForwardDataPacket), 0, (struct sockaddr *)&udpClientAddr, sizeof(udpClientAddr))) > 0)
                            {
                                printf("sendto success.\n");
                            }
                        }
                    }

                    break;
                case RETURN_PACKET:
                    break;
                case SIMPLEX_PACKET:
                    break;
                default:
                    break;
                }
                break;
            case OPEN_VIDEO_NO_RECORDING:
                switch(recvPacket->type) {
                case REQUEST_PACKET:

                    if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
                        printf("Set timeout failed!\n");
                    else
                        printf("Set timeout success!\n");

                    closeLinkIdFlag = 1;

                    if(1 == threadRunFlag)
                    {
                        sendPacket.guideHead = recvPacket->guideHead;//引导头
                        sendPacket.randomMark = recvPacket->randomMark;//随机标记
                        sendPacket.actionMark = recvPacket->actionMark;//行为标记
                        sendPacket.type = RETURN_PACKET;//包类型

                        sendPacket.currentDataBufSize = recvPacket->currentDataBufSize;
                        sendPacket.totalDataBufSize = recvPacket->totalDataBufSize;
                        sprintf(sendPacket.currentDataBuf, "请先关闭接收图像.");

                        //memcpy(sendPacket.currentDataBuf, recvPacket->currentDataBuf, DATA_BUF_SIZE_MAX);
                        sendLen = send(linkId, &sendPacket, sizeof(sendPacket), 0);
                        if(sendLen < 0)
                        {
                            printf("open video no recording linkid : %d\n", linkId);
                            perror("return open video no recording failed\n");
                            //exit(0);
                        }
                        break;
                    }

#if DEBUG_MSG
                    printf("open video no recording recv buffer : %s\n", recvPacket->currentDataBuf);
                    printf("----------------buffer-------------------\n");
                    for(i = 0; i < 40; i++)
                    {
                        printf("0x%x ", recvPacket->currentDataBuf[i]);
                        if((i > 0) && ((i % 20) == 0))
                            printf("\n");
                    }
                    printf("\n");
                    printf("----------------buffer-------------------\n");
#endif
                    sendPacket.guideHead = recvPacket->guideHead;//引导头
                    sendPacket.randomMark = recvPacket->randomMark;//随机标记
                    sendPacket.actionMark = recvPacket->actionMark;//行为标记
                    sendPacket.type = RETURN_PACKET;//包类型

                    getVideoParam(recvPacket->actionMark, recvPacket->currentDataBuf, &videoParam);
#if DEBUG_MSG
                    printf("open video ip = %d\n", videoParam.ipAddr[0]);
                    printf("open video ip = %d\n", videoParam.ipAddr[1]);
                    printf("open video ip = %d\n", videoParam.ipAddr[2]);
                    printf("open video ip = %d\n", videoParam.ipAddr[3]);
                    printf("open video mode = %d\n", videoParam.mode);
#endif

                    if((0 != videoParam.ipAddr[0]) && (0 != videoParam.ipAddr[1]) && (0 != videoParam.ipAddr[2]) && (0 != videoParam.ipAddr[3]))
                    {
                        videoParam.inOutFlag = 0x01;
                        videoParam.enableOutputDisplay = 0x01;

                        sprintf(ipTemp, "%d.%d.%d.%d", videoParam.ipAddr[0], videoParam.ipAddr[1],
                                videoParam.ipAddr[2], videoParam.ipAddr[3]);

                        printf("open video ipTemp : %s\n", ipTemp);

                        udpClientAddr.sin_family = AF_INET;
                        udpClientAddr.sin_port = htons(atoi(argv[2]));
                        udpClientAddr.sin_addr.s_addr = inet_addr(ipTemp);
                        printf("udpClientAddr : %s\n", inet_ntoa(udpClientAddr.sin_addr));
                    }
                    else
                    {
                        videoParam.inOutFlag = 0x00;
                        videoParam.enableOutputDisplay = 0x00;
                    }

                    memcpy(dispParam.ipAddr, videoParam.ipAddr, 4);
                    dispParam.mode = videoParam.mode;
                    dispParam.inOutFlag = videoParam.inOutFlag;//0:local out display,1:in
                    dispParam.enableOutputDisplay = videoParam.enableOutputDisplay;

                    if(videoParam.enableOutputDisplay == 0x01)
                    {
                        forwardDataPacket->actionMark = ENABLE_DISPLAY;
                        forwardDataPacket->outFlag = 0x01;
                        forwardDataPacket->mode = videoParam.mode;
                        printf("forwardDataPacket->mode : 0x%x\n", forwardDataPacket->mode);
                        if((sendto(udpClientSocket, forwardDataPacket, sizeof(ForwardDataPacket), 0, (struct sockaddr *)&udpClientAddr, sizeof(udpClientAddr))) > 0)
                        {
                            printf("udp sendto success.\n");
                        }
                    }


                    if(0 == openFlag)
                    {
                        if(pipeline_init(recvPacket->actionMark, videoParam) != 0)
                        {
                            printf("ERROR : pipeline initial failed!\n");
                            sprintf(sendPacket.currentDataBuf, "管道初始化失败.");
                           // goto err;
                        }

                        if(capture_start() != 0)
                        {
                            printf("ERROR : pipeline start failed!\n");
                            sprintf(sendPacket.currentDataBuf, "打开视频失败.");
                            //goto err;
                        }
                        openFlag = 1;
                        printf("........................ Capture Starting ........................\n");
                        sprintf(sendPacket.currentDataBuf, "OK");

                    }
                    else
                    {
                        sprintf(sendPacket.currentDataBuf, "视频已经打开，如需关闭，请发送视频关闭命令.");
                    }

                    sendPacket.currentDataBufSize = strlen(sendPacket.currentDataBuf);//recvPacket->currentDataBufSize;
                    sendPacket.totalDataBufSize = sendPacket.currentDataBufSize;//recvPacket->totalDataBufSize;
                    memcpy(sendPacket.currentDataBuf, recvPacket->currentDataBuf, DATA_BUF_SIZE_MAX);
                    sendLen = send(linkId, &sendPacket, sizeof(sendPacket), 0);
                    if(sendLen < 0)
                    {
                        printf("open video no recording linkid : %d\n", linkId);
                        perror("return open video no recording failed.\n");
                        //exit(0);
                    }
                    break;
                case RETURN_PACKET:
                    break;
                case SIMPLEX_PACKET:
                    break;
                default:
                    break;
                }
                break;
            case CLOSE_VIDEO:
                switch(recvPacket->type) {
                case REQUEST_PACKET:
                    if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
                        printf("Set timeout failed!\n");
                    else
                        printf("Set timeout success!\n");

                    closeLinkIdFlag = 1;

                    printf("Forwarding data other avb recv buffer : %s\n", recvPacket->currentDataBuf);

                    sendPacket.guideHead = recvPacket->guideHead;//引导头
                    sendPacket.randomMark = recvPacket->randomMark;//随机标记
                    sendPacket.actionMark = recvPacket->actionMark;//行为标记
                    sendPacket.type = RETURN_PACKET;//包类型

                    if(dispParam.enableOutputDisplay == 0x01)
                    {
                        sprintf(ipTemp, "%d.%d.%d.%d", dispParam.ipAddr[0], dispParam.ipAddr[1],
                                dispParam.ipAddr[2], dispParam.ipAddr[3]);

                        printf("close video ipTemp : %s\n", ipTemp);

                        udpClientAddr.sin_family = AF_INET;
                        udpClientAddr.sin_port = htons(atoi(argv[2]));
                        udpClientAddr.sin_addr.s_addr = inet_addr(ipTemp);
                        forwardDataPacket->actionMark = recvPacket->actionMark;
                        forwardDataPacket->outFlag = 0x02;
                        if((sendto(udpClientSocket, forwardDataPacket, sizeof(ForwardDataPacket), 0, (struct sockaddr *)&udpClientAddr, sizeof(udpClientAddr))) > 0)
                        {
                            printf("udp sendto success.\n");
                        }
                    }

                    if(1 == openFlag)
                    {
                        if(capture_stop() != 0)
                        {
                            printf("ERROR : pipeline stop failed!\n");
                            sprintf(sendPacket.currentDataBuf, "视频关闭失败。");
                            //goto err;
                        }

                        pipeline_destroy();

                        printf("........................ Capture Stop ........................\n");
                        sprintf(sendPacket.currentDataBuf, "OK");
                        openFlag = 0;

                    }
                    else
                    {
                        sprintf(sendPacket.currentDataBuf, "没有打开的视频");
                    }

                    sendPacket.currentDataBufSize = strlen(sendPacket.currentDataBuf);
                    sendPacket.totalDataBufSize = sendPacket.currentDataBufSize;
                    //memcpy(sendPacket.currentDataBuf, recvPacket->currentDataBuf, DATA_BUF_SIZE_MAX);
                    sendLen = send(linkId, &sendPacket, sizeof(sendPacket), 0);
                    if(sendLen < 0)
                    {
                        printf("close video linkid : %d\n", linkId);
                        perror("return close video command failed\n");
                        //exit(0);
                    }
                    break;
                case RETURN_PACKET:
                    break;
                case SIMPLEX_PACKET:
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }

            memset(tempbuffer, 0x00, sizeof(tempbuffer));
            memset(recvPacket, 0x00, sizeof(CustomPacket));
            memset(&videoParam, 0x00, sizeof(videoParam));
            memset(forwardDataPacket, 0x00, sizeof(ForwardDataPacket));

            printf("\n***************OVER***************\n");
        }

//        if(linkId > 0)
//        {
//            printf("\ndisconnect ...\n");
//            close(linkId);
//        }
    }

    if(recvPacket)
        free(recvPacket);
    if(forwardDataPacket)
        free(forwardDataPacket);
    close(socketId);

//    printf("Hello World!\n");
    return 0;
}
