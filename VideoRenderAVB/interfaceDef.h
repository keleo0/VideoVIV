#ifndef INTERFACEDEF_H
#define INTERFACEDEF_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "avbDef.h"

void testPing(time_t *t);
int timeCalibration(char *timeStamp);
int selfTestRequest();
int forwardingDataOtherAvb(char *data, int len);
int getForwardingData(char *data, int len, ForwardDataPacket *dataPacket);
int getLocalIp();
//char* getDirAviFile();
#endif // INTERFACEDEF_H
