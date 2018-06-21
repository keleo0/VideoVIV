#include "VideoRenderPAL.h"

extern float values[];

VideoRenderPAL::VideoRenderPAL(VideoManager *manager) : VideoRenderBase(manager),
    m_pDataSource(0),
    m_rendererData(0)
{
#ifdef VIDEOVIV_DEBUG
        qDebug() << Q_FUNC_INFO;
#endif
    m_rendererData = (GLES2RendererData*)malloc(sizeof(GLES2RendererData));
    m_rendererData->texture = 0;
    m_rendererData->viv_planes[0] = NULL;

    memset((void*)&m_buf, 0, sizeof(VideoFrame));

    if(m_buf.size >0)
        m_buf.virtual_addr = (void*)malloc(m_buf.size);
    m_pDataThread = new QThread();
    m_pDataSource = new DataSource(&m_buf, this);
    m_pDataSource->moveToThread(m_pDataThread);
}

VideoRenderPAL::~VideoRenderPAL()
{
    if(m_pDataSource)
        m_pDataSource->deleteLater();
    if(m_rendererData)
        free(m_rendererData);
}

void VideoRenderPAL::initialize()
{
    VideoRenderBase::initialize();
    if(m_pDataThread)
    {
        connect(m_pDataThread, &QThread::started, m_pDataSource, &DataSource::startCapture);
        connect(m_pDataSource, &DataSource::receivedata, m_manager, &VideoManager::update);
    }
}

void VideoRenderPAL::uninitialize()
{
    VideoRenderBase::uninitialize();
    m_pDataThread->disconnect();
    m_pDataSource->disconnect();
}

void VideoRenderPAL::startFrame()
{
#ifdef VIDEOVIV_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif
    m_pDataSource->m_bCapture = true;
    m_pDataThread->start();
}

void VideoRenderPAL::pauseFrame()
{
#ifdef VIDEOVIV_DEBUG
        qDebug() << Q_FUNC_INFO;
#endif
    m_pDataSource->v4l2_capture->v4l2_capture_clearBuffer();
}

void VideoRenderPAL::endFrame()
{
#ifdef VIDEOVIV_DEBUG
        qDebug() << Q_FUNC_INFO;
#endif
    m_pDataSource->m_bCapture = false;
    m_pDataSource->v4l2_capture->v4l2_capture_clearBuffer();
    m_pDataThread->terminate();
    m_pDataThread->wait(10);
}

void VideoRenderPAL::updateFrame()
{
#ifdef VIDEOVIV_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif
    if(m_pDataSource->m_bCapture)
    {
        m_program->bind();
        m_program->enableAttributeArray(ATTRIB_VERTEX);
        m_program->setAttributeArray(ATTRIB_VERTEX, GL_FLOAT, values, 3);
        if (m_buf.virtual_addr == NULL)
        {
            qDebug() << "buffer->virtual_addr == NULL";
            return;
        }
        m_rendererData->viv_planes[0] = NULL;
        //glClearColor(0,0,1,0);
        glDisable(GL_DEPTH_TEST);

        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1,&(m_rendererData->texture));
        glBindTexture(GL_TEXTURE_2D, m_rendererData->texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (m_rendererData->viv_planes[0] == NULL && m_buf.virtual_addr)
        {
            glTexDirectVIV(
                        GL_TEXTURE_2D,
                        m_buf.width, m_buf.height,
                        GL_VIV_UYVY,
                        (GLvoid **) &(m_rendererData->viv_planes)
                        );
            memcpy(m_rendererData->viv_planes[0], m_buf.virtual_addr, m_buf.size);
        }
        glTexDirectInvalidateVIV(GL_TEXTURE_2D);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_program->disableAttributeArray(ATTRIB_VERTEX);
        m_program->release();
        glDeleteTextures(1, &m_rendererData->texture);
    }
}
