//#include <stdio.h>
//#include <errno.h>
//#include <stdint.h>
//#include <semaphore.h>
//#include <pthread.h>
//#include <sys/stat.h>
//#include <sys/time.h>
//#include <fcntl.h>
//#include <sys/ioctl.h>
//#include <unistd.h>
//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <sys/mman.h>
//#include <signal.h>
//#include <linux/videodev2.h>
//#include <linux/mxcfb.h>
//#include <gst/gst.h>
//#include <gst/app/gstappsrc.h>
//#include "packetDef.h"
//#include "interfaceDef.h"

#include "gstreamer.h"
#include "packetDef.h"
//typedef enum _CaptureMode {
//    VIDEO = 0,
//    IMAGE = 1,
//    AUDIO = 2
//}CaptureMode;

//typedef struct _CustomData{
//    char *fileName;
//    CaptureMode mode;
//    GstElement *pipeLine;
//    GstElement *audioSource;
//    GstElement *videoSource;
//    GstElement *fileSink;
//    GstElement *udpsink;
//    GstBus *bus;
//    GMainLoop *mainLoop;
//    GThread *thread;
//}CustomData;

CustomData customData = {"testAudioVideoCapture.avi", VIDEO, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    gchar *debug;
    GError *error;

    switch(GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS:
        g_print("End of stream.\n");
        g_main_loop_quit(customData.mainLoop);
        break;
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);
        g_printerr("ERROR : %s\n", error->message);
        g_error_free(error);
        g_main_loop_quit(customData.mainLoop);
        break;
    default:
        break;
    }

    return TRUE;
}

void bus_loop_thread_func()
{
    g_main_loop_run(customData.mainLoop);
    gst_element_set_state(customData.pipeLine, GST_STATE_NULL);
}

int capture_set_file_name(char *fileName)
{
//    if(fileName)
//        customData.fileName = fileName;
//    else
//        return -1;

//    return 0;

   // printf("filename : %s\n", fileName);

}

int capture_set_mode(int captureMode)
{
    switch (captureMode) {
    case 0:
        customData.mode = VIDEO;
        break;
    case 1:
        customData.mode = IMAGE;
        break;
    case 2:
        customData.mode = AUDIO;
        break;
    default:
        return -1;
    }

    return 0;
}

void getVideoParam(unsigned int mark, unsigned char *data, VideoParameter *param)
{
    char ipStr1[32];
    char ipStr2[32];
    char *ipTemp;
    const char modeStr1[] = "VideoAndAudio";
    const char modeStr2[] = "VideoOnly";
    const char modeStr3[] = "AudioOnly";
    int i = 0;
    //char buffer[DATA_BUF_SIZE_MAX];
    char tempStr[DATA_BUF_SIZE_MAX];
    //char tempStr[1421];
    //char *fileNameTemp;
    char *temp1, *temp2, *temp3;

    //memcpy(buffer, data, DATA_BUF_SIZE_MAX);

    sscanf(data, "%20s", ipStr1);
    printf("ipStr1 = %s\n", ipStr1);
    ipTemp = strtok(ipStr1, ".");

    printf("ipTemp = %s\n", ipTemp);
    while(ipTemp != NULL)
    {
        if(i != 3)
        {
            param->ipAddr[i] = atoi(ipTemp);
            printf("%d\n", param->ipAddr[i]);
            ipTemp = strtok(NULL, ".");
        }
        else
        {
            sscanf(ipTemp, "%3s", ipStr2);
            param->ipAddr[i] = atoi(ipStr2);
            printf("%d\n", param->ipAddr[i]);
            ipTemp = strtok(NULL, ".");
        }
        i++;
        printf("ipTemp = %s, i = %d, ipStr2 = %s\n", ipTemp, i, ipStr2);
    }



//    if((0 != param->ipAddr[0]) && (0 != param->ipAddr[1]) && (0 != param->ipAddr[2]) && (0 != param->ipAddr[3]))
//    {
//        param->inOutFlag = 0x01;
//        param->enableOutputDisplay = 0x01;
//    }
//    else
//    {
//        param->inOutFlag = 0x00;
//        param->enableOutputDisplay = 0x00;
//    }

    memcpy(tempStr, data+20,1421);

    temp1 = strstr(tempStr, modeStr1);
    printf("wa ka ka\n");
    printf("temp1 : %s\n", temp1);
    if(temp1 != NULL)
    {
        param->mode = 0;
        //sprintf(param->fileName, "%s.avi", fileNameTemp);
    }

    temp2 = strstr(tempStr, modeStr2);
    if(temp2 != NULL)
    {
        param->mode = 1;
        //sprintf(param->fileName, "%s.avi", fileNameTemp);
    }

    temp3 = strstr(tempStr, modeStr3);
    if(temp3 != NULL)
    {
        param->mode = 2;
        //sprintf(param->fileName, "%s.mp3", fileNameTemp);
    }

    if((temp1 == NULL) && (temp2 == NULL) && (temp3 == NULL))
    {
        printf("Mode failed ...\n");
    }
}

int pipeline_init(unsigned char mark, VideoParameter param)
{
    char ipStr[32];

    printf("ip = %d\n", param.ipAddr[0]);
    printf("ip = %d\n", param.ipAddr[1]);
    printf("ip = %d\n", param.ipAddr[2]);
    printf("ip = %d\n", param.ipAddr[3]);
    sprintf(ipStr, "%d.%d.%d.%d", param.ipAddr[0], param.ipAddr[1], param.ipAddr[2], param.ipAddr[3]);
    printf("ipStr : %s\n", ipStr);

//    if((0 != param.ipAddr[0]) && (0 != param.ipAddr[1]) && (0 != param.ipAddr[2]) && (0 != param.ipAddr[3]))
//    {
//        param.inOutFlag = 0x01;
////        param.enableOutputDisplay = 0x01;
////        printf("enableDisplay : 0x%x, mode : 0x%x\n", param.enableOutputDisplay, param.mode);
//    }
//    else
//    {
//        param.inOutFlag = 0x00;
////        param.enableOutputDisplay = 0x01;
////        printf("enableDisplay : 0x%x, mode : 0x%x\n", param.enableOutputDisplay, param.mode);
//    }

    if((param.inOutFlag == 0x01) && (mark == OPEN_VIDEO_NO_RECORDING))
    {
        printf("I am here IN...\n");

        char videoPipelineString[] = "tvsrc fps-n=25 fps-d=1 ! "
                "video/x-raw-yuv,width=720,height=576,framerate=25/1 ! "
                "queue max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                "vpuenc codec=6 quant=10 seqheader-method=3 bitrate=2048000 framerate-nu=25 framerate-de=1 force-framerate=true ! "
                "queue max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                "mux. alsasrc do-timestamp=true ! "
                "mfw_mp3encoder sample-rate=44100 bitrate=128 ! "
                "queue max-size-buffers=3000 max-size-bytes=0 max-size-time=0 ! "
                "mux. mpegtsmux name=mux ! "
                "udpsink name=myUdpsink port=5002 sync=false async=false";

        char imagePipelineString[] = "tvsrc fps-n=25 fps-d=1 ! "
                "video/x-raw-yuv,width=720,height=576,framerate=25/1 ! "
                "queue max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                "vpuenc codec=6 quant=10 seqheader-method=3 bitrate=2048000 framerate-nu=25 framerate-de=1 force-framerate=true ! "
                "queue max-size-buffers=3000 max-size-bytes=0 max-size-time=0 ! "
                "mpegtsmux ! "
                "udpsink name=myUdpsink port=5002 sync=false async=false";

        char audioPipelineString[] = "alsasrc do-timestamp=true ! "
                "queue max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                "mfw_mp3encoder ! "
                "mpegtsmux ! "
                "queue max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                "udpsink name=myUdpsink port=5002 sync=true async=false";

        gst_init(NULL, NULL);

        switch(param.mode)
        {
        case VIDEO:
            customData.pipeLine = gst_parse_launch(videoPipelineString, NULL);
            printf("pipe mode = %d\n", param.mode);
            break;
        case IMAGE:
            customData.pipeLine = gst_parse_launch(imagePipelineString, NULL);
            printf("pipe mode = %d\n", param.mode);
            break;
        case AUDIO:
            customData.pipeLine = gst_parse_launch(audioPipelineString, NULL);
            printf("pipe mode = %d\n", param.mode);
            break;
        default:
            return -1;
        }

        customData.udpsink = gst_bin_get_by_name(GST_BIN(customData.pipeLine), "myUdpsink");
        g_object_set(G_OBJECT(customData.udpsink), "host", ipStr, NULL);

        customData.mainLoop = g_main_loop_new(NULL, FALSE);

        customData.bus = gst_pipeline_get_bus(GST_PIPELINE(customData.pipeLine));
        gst_bus_add_watch(customData.bus, (GstBusFunc)bus_call, NULL);

        gst_element_set_state(customData.pipeLine, GST_STATE_READY);
    }
    else if((param.inOutFlag == 0x00) && (mark == OPEN_VIDEO_NO_RECORDING))
    {
        printf("I am here local display...\n");
        char videoPlayString[] = "tvsrc ! mfw_v4lsink";
        char imagePlayString[] = "tvsrc ! mfw_v4lsink";
        char audioPlayString[] = "alsasrc ! alsasink sync=false";

        gst_init(NULL, NULL);

        switch(param.mode)
        {
        case VIDEO:
            customData.pipeLine = gst_parse_launch(videoPlayString, NULL);
            printf("pipe mode = %d\n", param.mode);
            break;
        case IMAGE:
            customData.pipeLine = gst_parse_launch(imagePlayString, NULL);
            printf("pipe mode = %d\n", param.mode);
            break;
        case AUDIO:
            customData.pipeLine = gst_parse_launch(audioPlayString, NULL);
            printf("pipe mode = %d\n", param.mode);
            break;
        default:
            return -1;
        }

        customData.mainLoop = g_main_loop_new(NULL, FALSE);

        customData.bus = gst_pipeline_get_bus(GST_PIPELINE(customData.pipeLine));
        gst_bus_add_watch(customData.bus, (GstBusFunc)bus_call, NULL);

        gst_element_set_state(customData.pipeLine, GST_STATE_READY);
    }
    else if((param.enableOutputDisplay == 0x01) && (mark == ENABLE_DISPLAY))
    {
        printf("I am here OUT...\n");
        char videoDecString[] = "udpsrc port=5002 ! "
                                     "queue  max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                                     "mpegtsdemux name=demux demux. ! "
                                     "queue  max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                                     "vpudec  framerate-nu=25 framerate-de=1 low-latency=true experimental-tsm=false ! "
                                     "mfw_v4lsink sync=false demux. ! "
                                     "queue max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                                     "beepdec ! "
                                     "audioconvert ! "
                                     "audio/x-raw-int,channels=2 ! "
                                     "alsasink sync=false";

        char imageDecString[] = "udpsrc port=5002 do-timestamp=true ! "
                                     "queue max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                                     "mpegtsdemux ! "
                                     "vpudec framerate-nu=25 framerate-de=1 low-latency=true experimental-tsm=false ! "
                                     "mfw_v4lsink sync=false";

        char audioDecString[] = "udpsrc port=5002 do-timestamp=true ! "
                                     "queue max-size-buffers=2000 max-size-bytes=0 max-size-time=0 ! "
                                     "mpegtsdemux ! "
                                     "beepdec ! "
                                     "audioconvert ! "
                                     "audio/x-raw-int,channels=2 ! "
                                     "alsasink sync=false";


        gst_init(NULL, NULL);

        switch(param.mode)
        {
        case VIDEO:
            customData.pipeLine = gst_parse_launch(videoDecString, NULL);
            printf("I am here ------------------- mode %d\n", param.mode);
            break;
        case IMAGE:
            customData.pipeLine = gst_parse_launch(imageDecString, NULL);
            printf("I am here ------------------- mode %d\n", param.mode);
            break;
        case AUDIO:
            customData.pipeLine = gst_parse_launch(audioDecString, NULL);
            printf("I am here ------------------- mode %d\n", param.mode);
            break;
        default:
            return -1;
        }

        customData.mainLoop = g_main_loop_new(NULL, FALSE);

        customData.bus = gst_pipeline_get_bus(GST_PIPELINE(customData.pipeLine));
        gst_bus_add_watch(customData.bus, (GstBusFunc)bus_call, NULL);

        gst_element_set_state(customData.pipeLine, GST_STATE_READY);
    }

    return 0;
}


int capture_start()
{
    if(customData.pipeLine)
    {
        gst_element_set_state(customData.pipeLine, GST_STATE_PLAYING);
        customData.thread = g_thread_create((GThreadFunc)bus_loop_thread_func, NULL, TRUE, NULL);
    }
    else
        return -1;

    return 0;
}

int capture_stop()
{
    if(customData.pipeLine)
    {
        if(FALSE == gst_element_send_event(customData.pipeLine, gst_event_new_eos()))
        {
            printf("Send EOS event failed\n");
            return -1;
        }
    }
    else
        return -1;

    return 0;
}

void pipeline_destroy()
{
    if(customData.thread)
        g_thread_join(customData.thread);

    if(customData.mainLoop)
        g_main_loop_unref(customData.mainLoop);

    if(customData.bus)
        gst_object_unref(customData.bus);

    if(customData.pipeLine)
        gst_object_unref(customData.pipeLine);
}

#if 0
int main(int argc, char *argv[])
{
    char commandStr[128];
    int flag = 1;

    if(argc < 2)
    {
        printf("Please inpute right parameters, eg. : testAudioVideoCapture 0");
        return -1;
    }
    else
        printf("You input parameters mode=%d\n", atoi(argv[1]));

//    if(capture_set_file_name(argv[1]) != 0)
//    {
//        printf("ERROR : you input parameters is wrong,"
//               "Please inpute right parameters, eg : testAudioVideoCapture video.avi 0");
//        return -1;
//    }

    if(capture_set_mode(atoi(argv[1])) != 0)
    {
        printf("ERROR : you input parameters is wrong,"
               "Please inpute right parameters, eg : testAudioVideoCapture video.avi 0");
        return -1;
    }

    if(pipeline_init() != 0)
    {
        printf("ERROR : pipeline initial failed!\n");
        goto err;
    }

    if(capture_start() != 0)
    {
        printf("ERROR : pipeline start failed!\n");
        goto err;
    }

    printf("........................ Capture Starting ........................\n");

    while(flag)
    {
        commandStr[0] = ' ';
        scanf("%s", commandStr);
        switch(commandStr[0])
        {
        case 'Q':
        case 'q':
            flag = 0;
            break;
        default:
            break;
        }
    }

    if(capture_stop() != 0)
    {
        printf("ERROR : pipeline stop failed!\n");
        goto err;
    }

    pipeline_destroy();

    printf("........................ Capture Stop ........................\n");

    return 0;

err:
    pipeline_destroy();


    printf("Hello World!\n");
    return 0;
}
#endif
