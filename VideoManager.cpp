#include "VideoManager.h"
#include "VideoRenderPAL.h"

float values[] = {
    -1,-1,0,
    1,-1,0,
    1,1,0,
    -1,1,0,
    -1,-1,0,
    1,1,0
};

VideoRenderBase::VideoRenderBase(VideoManager *manager) :
    m_pDataThread(0),
    m_program(0),
    m_manager(manager)
{
#ifdef VIDEOVIV_DEBUG
        qDebug() << Q_FUNC_INFO;
#endif
    initializeOpenGLFunctions();
}

VideoRenderBase::~VideoRenderBase()
{
    if(m_pDataThread)
    {
        if(m_pDataThread->isRunning())
        {
            m_pDataThread->exit();
            m_pDataThread->wait();
        }
        m_pDataThread->deleteLater();
    }
    if(m_program)
        m_program->deleteLater();
}

void VideoRenderBase::initialize()
{
#ifdef VIDEOVIV_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif
    if (!m_program) {
        m_program = new QOpenGLShaderProgram();
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "attribute vec4 vertices;"
                                           "varying vec2 tcoord;"
                                           "void main() {"
                                           "gl_Position = vertices;"
                                           "float xx=(vertices.x+1.0)*0.5;"
                                           "float yy=(1.0+vertices.y)*0.5;"
                                           "tcoord = vec2(xx,yy);"
                                           "}");
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "varying vec2 tcoord;"
                                           "uniform sampler2D tex;"
                                           "void main(void){"
                                           "gl_FragColor = texture2D(tex,tcoord);}"
                                           );

        m_program->bindAttributeLocation("vertices", ATTRIB_VERTEX);
        m_program->link();
    }
}

RenderGenerator::RenderGenerator(VideoManager *manager) :
    m_render(0),
    m_manager(manager)
{
#ifdef VIDEOVIV_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif
    m_manager->m_renderInterface = this;
    for(int i = 1; i < VideoManager::VideoSourceCount; i++)
    {
        VideoRenderBase *render = 0;
        switch (i)
        {
        case VideoManager::PAL:
            render = new VideoRenderPAL(this->manager());
            break;
        case VideoManager::AVB:
            //m_render = new VideoRenderAVB(this);
            break;
        default:
            qDebug() << "Warning!!!: Unknown video source type ~~~~~~";
            return;
        }
        if(render)
            m_manager->m_renderHash.insert(i, render);
    }
    m_manager->m_bInited = true;
    m_render = m_manager->render();
    if(m_render)
    {
        m_render->initialize();
        m_render->startFrame();
    }
}

RenderGenerator::~RenderGenerator()
{
    if(m_render)
        m_render->deleteLater();
}

void RenderGenerator::render()
{
    if(m_render)
        m_render->updateFrame();
}

QOpenGLFramebufferObject *RenderGenerator::createFramebufferObject(const QSize &size)
{
#ifdef VIDEOVIV_DEBUG
        qDebug() << Q_FUNC_INFO;
#endif
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

QQuickFramebufferObject::Renderer *VideoManager::createRenderer() const
{
#ifdef VIDEOVIV_DEBUG
        qDebug() << Q_FUNC_INFO;
#endif
    return  new RenderGenerator(const_cast<VideoManager*>(this));
}

void VideoManager::setVideoSource(const VideoSource &source)
{
#ifdef VIDEOVIV_DEBUG
    qDebug() << Q_FUNC_INFO << source;
#endif
    if((m_videoSource != source) && (source < VideoSourceCount) && (source > 0))
    {
        if(m_bInited)
        {
            if(m_renderHash.value(m_videoSource) && m_renderHash.value(source))
            {
                VideoRenderBase *render = m_renderHash.value(m_videoSource);
                render->endFrame();
                render->uninitialize();

                render = m_renderHash.value(source);
                m_renderInterface->m_render = render;
                render->initialize();
                render->startFrame();
                m_runningState = Running;

                m_videoSource = source;
            }
        }
        else
            m_videoSource = source;
    }
}

 void VideoManager::setRunningState(const RunningState &state)
 {
#ifdef VIDEOVIV_DEBUG
     qDebug() << Q_FUNC_INFO << state;
#endif
     if((m_runningState != state) && (state < RunningStateCount) && (state > 0))
     {
         if(m_bInited)
         {
             VideoRenderBase *render = m_renderInterface->getRender();
             switch (state)
             {
             case Running:
                 render->startFrame();
                 break;
             case Pause:
                 render->pauseFrame();
                 break;
             case End:
                 render->endFrame();
                 break;
             default:
                 qDebug() << "Warning!!!: Unknown video source type ~~~~~~";
                 return;
             }
         }
         m_runningState = state;
     }
 }
