#include <QGuiApplication>
#include <QQuickView>

#include "VideoManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<VideoManager>("VideoRendering", 1, 0, "VideoManager");

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:/main.qml"));
    view.show();

    return app.exec();
}
