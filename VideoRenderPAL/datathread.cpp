#include "datathread.h"
#include "VideoRenderPAL.h"
#include <QApplication>

DataSource::DataSource(VideoFrame* _buf, VideoRenderPAL *prender, QObject *parent) : QObject(parent)
{
#ifdef VIDEOVIV_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif
    m_prender = prender;
    m_buf=_buf;
    v4l2_capture = new V4l2_Capture();
    m_bCapture = false;

    if((v4l2_capture->v4l2_capture_init(m_buf, 0, 0, 720, 576)) <0)
    {
        qDebug()<<"v4l2_capture v4l2_capture_init fialed!";
        m_bCapture=false;
    }
}

void DataSource::startCapture()
{
#ifdef VIDEOVIV_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif
    while(m_bCapture)
    {
        if(m_prender->manager()->runningState() == VideoManager::Running)
        {
            if(v4l2_capture->v4l2_capture_getframe(m_buf) <0)
            {
                qDebug()<<("v4l2_capture_get_frame failed!\n");
                break;
            }
            emit receivedata();
        }
    }
}
