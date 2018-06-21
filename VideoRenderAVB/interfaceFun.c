#include "packetDef.h"
#include "interfaceDef.h"

void testPing(time_t *t)
{
    time(t);
    return;
}

int timeCalibration(char *timeStamp)
{
    struct timeval t;
    struct tm *tLocal;
    time_t zxpTime;
    time_t mk;

    printf("TimeStamp : %s\n", timeStamp);

    zxpTime = atoi(timeStamp);
    tLocal = localtime(&zxpTime);

    printf("zxpTime : %d\n", zxpTime);

    strftime(timeStamp, 32, "%Y-%m-%d %H:%M:%S", tLocal);
    mk = mktime(tLocal);
    t.tv_sec = mk;
    t.tv_usec = 0;

    if((settimeofday(&t, 0)) < 0)
    {
        printf("Set system date error ...\n");
        return -1;
    }

    return 0;
}

int selfTestRequest()
{
    return 0;
}

int forwardingDataOtherAvb(char *data, int len)
{
    char avbFlag[32];
    char ipTemp[32];
    char *avbFalgTemp;
    AvbPacket packet;
    int i = 0, j = 0;

    //memset(&packet, 0, sizeof(packet));
    sscanf(data, "%20s", avbFlag);
    printf("avbFlag : %s\n", avbFlag);

    avbFalgTemp = strtok(avbFlag, ".");
    while(avbFalgTemp != NULL)
    {
        if(i != 3)
        {
            packet.ipAddr[i] = atoi(avbFalgTemp);
            printf("ipaddr[%d] : %d\n", i, packet.ipAddr[i]);
            avbFalgTemp = strtok(NULL, ".");
        }
        else
        {
            sscanf(avbFalgTemp, "%3s", ipTemp);
            packet.ipAddr[i] = atoi(ipTemp);
            printf("ipaddr[%d] : %d\n", i, packet.ipAddr[i]);
            avbFalgTemp = strtok(NULL, ".");
        }
//        packet.ipAddr[i] = atoi(avbFalgTemp);
//        printf("ipaddr[%d] : %d\n", i, packet.ipAddr[i]);
//        avbFalgTemp = strtok(NULL, ".");
        i++;
    }

    memcpy(packet.data, data+20, len - 20);

//    for(j = 0; j < len - 20; j++)
//    {
//        if(j % 20 == 0)
//            printf("\n");
//        printf(" %d ", packet.data[j]);
//    }
//    printf("\n");

    return 0;
}

int getForwardingData(char *data, int len, ForwardDataPacket *dataPacket)
{
    char avbFlag[32];
    char ipTemp[32];
    char *avbFalgTemp;
    AvbPacket packet;
    int i = 0, j = 0;

    //memset(&packet, 0, sizeof(packet));
    sscanf(data, "%20s", avbFlag);
    printf("avbFlag : %s\n", avbFlag);

    avbFalgTemp = strtok(avbFlag, ".");
    while(avbFalgTemp != NULL)
    {
        if(i != 3)
        {
            dataPacket->ipAddr[i] = atoi(avbFalgTemp);
            printf("ipaddr[%d] : %d\n", i, dataPacket->ipAddr[i]);
            avbFalgTemp = strtok(NULL, ".");
        }
        else
        {
            sscanf(avbFalgTemp, "%3s", ipTemp);
            dataPacket->ipAddr[i] = atoi(ipTemp);
            printf("ipaddr[%d] : %d\n", i, dataPacket->ipAddr[i]);
            avbFalgTemp = strtok(NULL, ".");
        }
        i++;
    }

    memcpy(dataPacket->dataBuffer, data+20, len - 20);

//    for(j = 0; j < len - 20; j++)
//    {
//        if(j % 20 == 0)
//            printf("\n");
//        printf(" %d ", dataPacket->dataBuffer[j]);
//    }

//    for(j = 0; j < 50; j++)
//    {
//        if(j % 20 == 0)
//            printf("\n");
//        printf(" 0x%x", dataPacket->dataBuffer[j]);
//    }
//    printf("\n");

    return 0;
}

int getLocalIp()
{
    int ipSocket;

    if((ipSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("ip socket error.\n");
        return -1;
    }

    struct ifreq ifrIp;

    memset(&ifrIp, 0x00, sizeof(ifrIp));
    strncpy(ifrIp.ifr_ifrn.ifrn_name, "eth0", sizeof(ifrIp.ifr_ifrn.ifrn_name) - 1);
    if(ioctl(ipSocket, SIOCGIFADDR, &ifrIp) < 0)
    {
        perror("ip ioctl error.\n");
        return -1;
    }

    struct sockaddr_in *addr;
    addr = (struct sockaddr_in *)&ifrIp.ifr_ifru.ifru_addr;
    close(ipSocket);
    return addr->sin_addr.s_addr;
}

//int getDirAviFile(VideoParam)
//{
//    DIR *dir;
//    struct dirent *dirp;
//    //int size = 0;
//    char* fileName = NULL;
//    char fileNameTemp[DATA_BUF_SIZE_MAX];

//    memset(fileNameTemp, 0, sizeof(fileNameTemp));

//    if((dir = opendir("./")) == NULL)
//    {
//        printf("open dir failed ...\n");
//        return -1;
//    }

//    while((dirp = readdir(dir)) != NULL)
//    {
//        if((strcmp(dirp->d_name, ".") == 0) || (strcmp(dirp->d_name, "..") == 0))
//            continue;

//        if((strstr(dirp->d_name, ".avi") != NULL) || (strstr(dirp->d_name, ".mp3") != NULL))
//        {

//            //sprintf(fileName, "%s | ", dirp->d_name);
//            //printf("file name : %s\n", fileName);
//            strcat(dirp->d_name, "|");
//            fileName = strcat(fileNameTemp, dirp->d_name);

//        }
//    }
//    //printf("file name : %s\n", fileName);
//    return fileName;
//}

//void getFlagModeStr(unsigned char *data, char *ipAddr[], unsigned int *mode)
//{
//    char ipStr1[32];
//    char ipStr2[32];
//    char *ipTemp;
//    unsigned int mode1 = 0;
//    unsigned int mode2 = 1;
//    unsigned int mode3 = 2;
//    const char modeStr1[] = "VidioAndAudio";
//    const char modeStr2[] = "VideoOnly";
//    const char modeStr3[] = "AudioOnly";
//    int i = 0;
//    char buffer[DATA_BUF_SIZE_MAX];
//    char *temp;
//    memcpy(buffer, data, DATA_BUF_SIZE_MAX);
//    sscanf(data, "%20s", ipStr1);
//    ipTemp = strtok(ipStr1, ".");

//    while(ipTemp != NULL)
//    {
//        if(i != 3)
//        {
//            ipAddr[i] = ipTemp;
//            printf("%s\n", ipAddr[i]);
//            ipTemp = strtok(NULL, ".");
//        }
//        else
//        {
//            sscanf(ipTemp, "%3s", ipStr2);
//            ipAddr[i] = ipStr2;
//            printf("%s\n", ipAddr[i]);
//            ipTemp = strtok(NULL, ".");
//        }

//        i++;
//    }


//    temp = strstr(buffer, modeStr1);
//    if(temp != NULL)
//    {
//        *mode = mode1;
//    }

//    temp = strstr(buffer, modeStr2);
//    if(temp != NULL)
//    {
//        *mode = mode2;
//    }

//    temp = strstr(buffer, modeStr3);
//    if(temp != NULL)
//    {
//        *mode = mode3;
//    }

//    if(temp == NULL)
//    {
//        printf("Mode failed ...\n");
//    }
//}

