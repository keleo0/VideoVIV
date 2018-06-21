#ifndef VIDEORENDERPAL_H
#define VIDEORENDERPAL_H

//#include <g2d.h>
#include "VideoManager.h"
#include "datathread.h"
#include "video_defines.h"
#include "glext_header.h"

struct GLES2RendererData
{
    GLuint texture;
    GLvoid* viv_planes[3];
};

class VideoRenderPAL : public VideoRenderBase
{
    Q_OBJECT
public:
    VideoRenderPAL(VideoManager *manager = 0); //zk 6
    ~VideoRenderPAL();

    void initialize();
    void uninitialize();
    void startFrame();
    void updateFrame();
    void pauseFrame();
    void endFrame();

    DataSource* m_pDataSource;
    GLES2RendererData* m_rendererData;
    VideoFrame m_buf;
};

#endif // VIDEORENDERPAL_H
