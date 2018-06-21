#include <QQuickWindow>
#include <QOpenGLContext>
#include <QQuickView>
#include <QCoreApplication>
#include <QDebug>
#include <QMatrix4x4>

#include "vediomanager.h"

#define ATTRIB_VERTEX 3
//float values[] = {-1,-1, 1,-1, 1,1, -1,-1, 1,1, -1,1};
float values[] = {
    -1,-1,-1,
    1,-1,-1,
    1,1,-1,
    -1,1,-1,
    -1,-1,-1,
    1,1,-1
};

extern QQuickView *viewer;
bool g_bClearVideoBuffer = false;

VedioRenderer::VedioRenderer()
    :  m_program(0),m_programBkd(0)
{
}
VedioRenderer::VedioRenderer(VedioManager* pManage)
    :  m_program(0),m_pVedioManage(pManage),m_programBkd(0)
{
    m_bkgColor = QVector4D(0,1,1,1);
//    m_bReadVedio = true;
    m_bSetLastBg = true;
    m_vedioPara = QVector4D(0,0,200,200);
//    m_bJudge = false;
    m_rendererData = (GLES2RendererData*)malloc(sizeof(GLES2RendererData));
    m_rendererData->texture = 0;
    m_rendererData->viv_planes[0] = NULL;

    memset((void*)&m_buf, 0, sizeof(VideoFrame));

    if(m_buf.size >0)
        m_buf.virtual_addr = (void*)malloc(m_buf.size);
    initializeOpenGLFunctions();
    initialProgram();
    initialProgramBkd();

//        glDisable(GL_BLEND);
//        glFrontFace(GL_CW);
//        glCullFace(GL_FRONT);
//        glEnable(GL_CULL_FACE);
//        glEnable(GL_DEPTH_TEST);

    m_pDataThread = new DataThread(&m_buf,this);
    connect(m_pDataThread, SIGNAL(receivedata()), m_pVedioManage, SLOT(driveFrame()));
//     connect(m_pDataThread,SIGNAL(receivedata()),m_pWindow,SLOT(update()),Qt::QueuedConnection);
//    startVedio();
}

void VedioRenderer::initialProgram()
{
    if (!m_program) {
        m_program = new QOpenGLShaderProgram();
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "attribute vec4 vertices;"
                                           "varying vec2 tcoord;"
                                           "void main() {"
                                           "gl_Position = vertices;"
                                           "float xx=(vertices.x+1.0)*0.5;"
                                           "float yy=(1.0-vertices.y)*0.5;"
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
void VedioRenderer::initialProgramBkd()
{
    if (!m_programBkd) {
        m_programBkd = new QOpenGLShaderProgram();
        m_programBkd->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                              "attribute vec4 vertices;"
                                              "void main() {"
                                              "    gl_Position = vertices;"
                                              "}");
        m_programBkd->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                              "uniform vec4 bkcolor;"
                                              "void main(void){"
                                              "gl_FragColor = bkcolor;}"
                                              );
        //"uniform vec4 bkcolor;"
        m_programBkd->bindAttributeLocation("vertices", ATTRIB_VERTEX);
        m_programBkd->link();
        m_bkgpos = m_programBkd->uniformLocation("bkcolor");
        qDebug() << m_bkgpos;
    }
}
VedioRenderer::~VedioRenderer()
{
    if(m_pDataThread->isRunning())
    {
        m_pDataThread->exit();
        m_pDataThread->wait();
        m_pDataThread->deleteLater();
        m_pDataThread = NULL;
    }
    delete m_program;
    delete m_programBkd;
    if (m_rendererData == NULL)
        return;

    free(m_rendererData);
}

void VedioRenderer::UpdateFrame()
{
    if(m_pDataThread->m_bCapture)
    {
        //qDebug() << "UpdateFrame";

        m_program->bind();
        m_program->enableAttributeArray(ATTRIB_VERTEX);
        m_program->setAttributeArray(ATTRIB_VERTEX, GL_FLOAT, values, 3);

        if (m_buf.virtual_addr == NULL)
        {
            qDebug() << "buffer->virtual_addr == NULL";
            return;
        }
        m_rendererData->viv_planes[0] = NULL;
        glClearColor(0,1,0,0);
        glViewport((GLint)(m_vedioPara.x()), m_pVedioManage->window()->height() - (GLint)(m_vedioPara.y()), (GLsizei)(m_vedioPara.z()), (GLsizei)(m_vedioPara.w()));
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
        glDeleteTextures(1, &m_rendererData->texture);//
    }
    else
    {
        if(m_pVedioManage->isVisible())
            UpdateFrameBkd();
        else {
            if(!m_bSetLastBg) {
                  UpdateFrameBkd();
                  m_bSetLastBg = true;
            }
        }
    }
}
void VedioRenderer::UpdateFrameBkd()
{
    //qDebug() << "background";
    //qDebug()<<m_bkgColor;
    m_programBkd->bind();
    m_programBkd->setUniformValue(m_bkgpos,m_bkgColor);
    m_programBkd->enableAttributeArray(ATTRIB_VERTEX);
    m_programBkd->setAttributeArray(ATTRIB_VERTEX, GL_FLOAT, values, 3);

    glClearColor(0,1,0,0);
//    glClear(GL_COLOR_BUFFER_BIT);
    glViewport((GLint)(m_vedioPara.x()), m_pVedioManage->window()->height() - (GLint)(m_vedioPara.y()), (GLsizei)(m_vedioPara.z()), (GLsizei)(m_vedioPara.w()));
    glDisable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_programBkd->disableAttributeArray(ATTRIB_VERTEX);
    m_programBkd->release();
}
void VedioRenderer::startVedio()
{
    if(m_pVedioManage->isVisible()) {
        qDebug() << "start";
        m_pDataThread->m_bCapture = true;
        g_bClearVideoBuffer = true;
        m_bSetLastBg = false;
        m_pDataThread->start();
    }
}

void VedioRenderer::stopVedio()
{
    //disconnect(m_pWindow, SIGNAL(beforeRendering()), this, SLOT(UpdateFrame()));
    // connect(m_pWindow, SIGNAL(beforeRendering()), this, SLOT(UpdateFrameBkd()), Qt::DirectConnection);

    qDebug() << "stop";
    m_pDataThread->m_bCapture = false;
//    m_pVedioManage->window()->update();
}

VedioManager::VedioManager()
    :  m_vedioRender(0)
    //      m_driveUpdate(true)
{
//    m_bJiaoti = true;
    connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
}
void VedioManager::sync()
{
    if (!m_vedioRender)
    {
        m_vedioRender = new VedioRenderer(this);
        connect(window(), SIGNAL(beforeRendering()), m_vedioRender, SLOT(UpdateFrame()), Qt::DirectConnection);
    }

    m_vedioRender->setViewportSize(window()->size() * window()->devicePixelRatio());
    m_vedioRender->setWindow(window());
    m_vedioRender->setVedioParas(m_vedioPara);
    m_vedioRender->setBkgColor(m_bkgColr);
}
void VedioManager::handleWindowChanged(QQuickWindow *win)
{   
    if (win) {
        connect(win, SIGNAL(beforeSynchronizing()), this, SLOT(sync()), Qt::DirectConnection);
        connect(win, SIGNAL(sceneGraphInvalidated()), this, SLOT(cleanup()), Qt::DirectConnection);
        win->setClearBeforeRendering(false);
        disconnect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
    }
}

void VedioManager::cleanup()
{
    if (m_vedioRender) {
        delete m_vedioRender;
        m_vedioRender = NULL;
    }
}
void VedioManager::driveFrame()
{
//    if(m_vedioRender->m_pDataThread->m_bCapture) {
    window()->update();
        //this->parentItem()->update();
        //QMetaObject::invokeMethod(this->parent(), "update");
//    }
}

QVector4D VedioManager::vedioPara() const
{
    return m_vedioPara;
}
void VedioManager::setVedioPara(const QVector4D &_c)
{
    qDebug() << "set setVedioPara";
    m_vedioPara = _c;
    if(m_vedioRender)
    {
        qDebug()<<"set setVedioPara1";
        m_vedioRender->setVedioParas(m_vedioPara);
    }
}

//bool VedioManager::driveUpdate() const
//{
//    return m_driveUpdate;
//}
//void VedioManager::setDriveUpdate(const bool &_c)
//{
//    m_driveUpdate = _c;
//}

QVector4D VedioManager::bkgColr()const
{
    return m_bkgColr;
}
void VedioManager::setbkgColr(const QVector4D &_c)
{
    qDebug() << "set bkgColr";
    m_bkgColr = _c;
    if(m_vedioRender)
    {
        qDebug() << "set bkgColr1";
        m_vedioRender->setBkgColor(m_bkgColr);
    }
}

VideoSource VedioManager::videoSource() const
{
    return m_videoSource;
}
void VedioManager::setVideoSource(const VideoSource &_s)
{
    qDebug() << "set videoSource" << _s;
    m_videoSource = _s;
}

void VedioManager::controlState(bool _state)
{
    if(_state)
    {
        if (m_vedioRender)
        {
            //disconnect(window(), SIGNAL(beforeRendering()), m_vedioRender, SLOT(UpdateFrameBkd()));
            //connect(window(), SIGNAL(beforeRendering()), m_vedioRender, SLOT(UpdateFrame()), Qt::DirectConnection);

            m_vedioRender->startVedio();
        }
    }
    else
    {
        if (m_vedioRender)
        {
            m_vedioRender->stopVedio();
        }
    }
}

void VedioManager::visibleChangeVedio(bool visibleState)
{
    if(visibleState)
        connect(window(), SIGNAL(beforeRendering()), m_vedioRender, SLOT(UpdateFrame()), Qt::DirectConnection);
    else
        disconnect(window(), SIGNAL(beforeRendering()), m_vedioRender, SLOT(UpdateFrame()));
//    m_bJiaoti = !m_bJiaoti;

//    if(m_bJiaoti)
//    {
//        //        if (m_vedioRender)
//        //        {
//        //            connect(window(), SIGNAL(beforeRendering()), m_vedioRender, SLOT(UpdateFrame()), Qt::DirectConnection);
//        //        }
//        m_vedioRender->startVedio();
//    }
//    else
//    {
//        //        if (m_vedioRender)
//        //        {
//        //            connect(window(), SIGNAL(beforeRendering()), m_vedioRender, SLOT(UpdateFrameBkd()), Qt::DirectConnection);
//        //        }
//        m_vedioRender->stopVedio();
//    }
}
