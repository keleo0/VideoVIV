#ifndef VEDIOMANAGER_H
#define VEDIOMANAGER_H

//#ifdef __cplusplus
//extern "C"{
//#endif

#include <QObject>
#include <QQuickItem>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QSGTexture>
#include <QOpenGLFunctions_ES2>
#include <QTimer>
#include <g2d.h>
#include <QMutex>
#include <QVector4D>
#include <QThread>

#include "video_defines.h"
#include "glext_header.h"
#include "datathread.h"

typedef struct _GLES2RendererData
{   
    GLuint texture;
    GLvoid* viv_planes[3];
}GLES2RendererData;

class VedioManager;
class VedioRenderer : public QObject,protected QOpenGLFunctions_ES2{
    Q_OBJECT

public:
    VedioRenderer();
    VedioRenderer(VedioManager* pManage);
    ~VedioRenderer();

    void setViewportSize(const QSize &size) { m_viewportSize = size; }
    DataThread* m_pDataThread;
    void setWindow(QQuickWindow* _window){m_pWindow=_window;}
    void setVedioParas(QVector4D _para){m_vedioPara=_para;}

    void startVedio();
    void stopVedio();
    void setBkgColor(QVector4D _color){m_bkgColor=_color;}
public slots:
    void UpdateFrame();
    void UpdateFrameBkd();


private:
    bool m_bSetLastBg;
    QVector4D m_vedioPara;
//    bool m_bJudge;
    void initialProgram();
    void initialProgramBkd();
    GLES2RendererData* m_rendererData;
    QSize m_viewportSize;
    VideoFrame m_buf;
    QOpenGLShaderProgram *m_program;
    QOpenGLShaderProgram *m_programBkd;
    QQuickWindow* m_pWindow;
    VedioManager* m_pVedioManage;
    QVector4D m_bkgColor;
    GLuint m_bkgpos;
};



class VedioManager: public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QVector4D bkgColr READ bkgColr WRITE setbkgColr NOTIFY bkgColrChanged)
    Q_PROPERTY(QVector4D vedioPara READ vedioPara WRITE setVedioPara NOTIFY vedioParaChanged)
//    Q_PROPERTY(bool driveUpdate READ driveUpdate WRITE setDriveUpdate NOTIFY driveUpdateChanged)
    Q_PROPERTY(VideoSource videoSource READ videoSource WRITE setVideoSource)
public:
    VedioManager();

    enum VideoSource { PAL, AVB };
    Q_ENUM(VideoSource)

    VideoSource videoSource() const;
    void setVideoSource(const VideoSource &_s);

    QVector4D vedioPara() const;
    void setVedioPara(const QVector4D &_c);

//    bool driveUpdate() const;
//    void setDriveUpdate(const bool &_c);

    QVector4D bkgColr() const;
    void setbkgColr(const QVector4D &_c);

    Q_INVOKABLE void visibleChangeVedio(bool);
    Q_INVOKABLE void controlState(bool _state);
signals:
    void vedioParaChanged();
    void driveUpdateChanged();
    void bkgColrChanged();
public slots:
    void sync();
    void cleanup();
    void driveFrame();

private slots:
    void handleWindowChanged(QQuickWindow *win);
private:
    VedioRenderer *m_vedioRender;
    QVector4D m_vedioPara;
    QVector4D m_bkgColr;
    VideoSource m_videoSource;
//    bool m_driveUpdate;
//    bool m_bJiaoti;
public:

};

#endif // VEDIOMANAGER_H
