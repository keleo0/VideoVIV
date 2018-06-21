#ifndef VIDEOMANAGER_H
#define VIDEOMANAGER_H

#include <QObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_ES2>
#include <QThread>
#include <QDebug>
#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFramebufferObject>

class VideoManager;
class RenderGenerator;

#define ATTRIB_VERTEX 3

class VideoRenderBase : public QObject, public QOpenGLFunctions_ES2
{
    Q_OBJECT
public:
    VideoRenderBase(VideoManager *); //zk 5
    ~VideoRenderBase();

    virtual void initialize(); //zk 8
    virtual void uninitialize() { m_program->deleteLater(); }
    virtual void startFrame() = 0;
    virtual void updateFrame() = 0;
    virtual void pauseFrame() = 0;
    virtual void endFrame() = 0;

    VideoManager *manager() { return m_manager; }

public:
    QThread *m_pDataThread;
    QOpenGLShaderProgram *m_program;
    VideoManager *m_manager;
};

class RenderGenerator : public QQuickFramebufferObject::Renderer
{
public:
    RenderGenerator(VideoManager *); //zk 4
    ~RenderGenerator();

    void render(); //zk 10
    VideoManager *manager() { return m_manager; }
    VideoRenderBase *getRender() { return m_render; }
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size); //zk 9

    VideoRenderBase *m_render;
    VideoManager *m_manager;
};

class VideoManager : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    Q_PROPERTY(VideoSource videoSource READ videoSource WRITE setVideoSource)
    Q_PROPERTY(RunningState runningState READ runningState WRITE setRunningState)

    VideoManager() : m_bInited(false), m_renderInterface(0) {}

    enum VideoSource {
        NONE,
        PAL,
        AVB,
        VideoSourceCount
    };
    Q_ENUM(VideoSource)

    enum RunningState {
        NONE,
        Running,
        Pause,
        End,
        RunningStateCount
    };
    Q_ENUM(RunningState)

    VideoSource videoSource() const { return m_videoSource; }
    void setVideoSource(const VideoSource &source); //zk 2

    RunningState runningState() const { return m_runningState; }
    void setRunningState(const RunningState &state); //zk 1

    RenderGenerator *renderInterface() const { return m_renderInterface; }
    VideoRenderBase *render() const { return m_renderHash.value(m_videoSource); }

public:
    bool m_bInited;
    VideoSource m_videoSource;
    RunningState m_runningState;
    QHash<int, VideoRenderBase *> m_renderHash;
    RenderGenerator *m_renderInterface;


    Renderer *createRenderer() const; //zk 3
};

#endif // VIDEOMANAGER_H
