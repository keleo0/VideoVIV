#ifndef DATATHREAD_H
#define DATATHREAD_H
#include <QObject>
#include <QDebug>

#include "v4l2_capture.h"
#include "video_defines.h"

class VideoRenderPAL;
class DataSource : public QObject
{
    Q_OBJECT
public:
    DataSource(VideoFrame* , VideoRenderPAL *, QObject *parent = 0); //zk 7
    VideoFrame* m_buf;
    V4l2_Capture *v4l2_capture;
    bool m_bCapture;
    VideoRenderPAL* m_prender;

signals:
    void receivedata();

public slots:
    void startCapture();
};

#endif // DATATHREAD_H
