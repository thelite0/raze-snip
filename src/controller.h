#pragma once
#include "capture.h"
#include "cornerguard.h"
#include <QObject>
#include <QImage>
#include <QRectF>
#include <QPainterPath>
#include <QElapsedTimer>
#include <QQmlEngine>
#include <QQuickView>
#include <QTemporaryFile>
#include <QJsonArray>
#include <memory>
class Controller : public QObject {
 Q_OBJECT
 Q_CLASSINFO("D-Bus Interface", "org.raze.Snip")
 Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY changed)
 Q_PROPERTY(bool active READ active NOTIFY changed)
 Q_PROPERTY(QRectF selection READ selection NOTIFY changed)
 Q_PROPERTY(bool testing READ testing NOTIFY changed)
 Q_PROPERTY(QPointF cursorPosition READ cursorPosition NOTIFY changed)
public:
 explicit Controller(QObject *parent=nullptr);
 ~Controller() override;
 int mode()const{return snipMode;}
 bool active()const{return selecting;}
 bool testing()const{return !testDirectory.isEmpty();}
 QPointF cursorPosition()const{return pointer;}
 void warm(){prepareOverlays();}
 QRectF selection()const{return region;}
 const QImage &snapshot()const{return frozen;}
 QRect workspace()const{return bounds;}
 qreal captureScale()const{return scale;}
 QPainterPath selectedPath()const;
 void setMode(int value);
 void start(const QStringList &args);
 void registerShortcut(bool replaceConflict);
 void runSmokeTest(const QString &directory);
 Q_INVOKABLE void down(qreal x,qreal y,int button,int modifiers);
 Q_INVOKABLE void move(qreal x,qreal y);
 Q_INVOKABLE void up(qreal x,qreal y,int button,int modifiers);
 Q_INVOKABLE void key(int key,int modifiers);
 Q_INVOKABLE void cancel();
 Q_INVOKABLE void accept(int modifiers=0);
 Q_INVOKABLE void editLast();
 Q_INVOKABLE void editorDone(const QImage &image,bool save);
public slots:
 Q_SCRIPTABLE void Capture(const QStringList &args){start(args);}
 Q_SCRIPTABLE void WindowSnapshot(const QString &json);
 Q_SCRIPTABLE QVariantMap Status()const;
 Q_SCRIPTABLE void Quit();
signals:
 void changed();
 void editorImage(const QImage &image);
private:
 void prepareOverlays();
 void showOverlays();
 void hideOverlays();
 void captured(const QImage &image,qreal scale);
 void finish(const QImage &image,int modifiers);
 void clipboard(const QImage &image,bool save);
 void snapshotWindows();
 void unloadScript();
 void fail(const QString &reason);
 ::Capture backend;
 CornerGuard cornerGuard;
 QQmlEngine engine;
 QList<QQuickView*> overlays;
 QQuickView *editor=nullptr;
 QImage frozen,lastImage;
 QRect bounds;
 QRectF region;
 QPointF anchor,pointer,keyboardOffset;
 QPainterPath freePath;
 qreal scale=1;
 int snipMode=0;
 bool selecting=false,dragging=false,saveRequested=false,annotateRequested=false,screenRequested=false,workspaceRequested=false;
 quint64 clipboardSerial=0;
 QString activeScreenName,scriptName,windowHandle,windowScreenName;
 bool windowCapturePending=false;
 int windowCaptureModifiers=0;
 std::unique_ptr<QTemporaryFile> script;
 QJsonArray windows;
 QElapsedTimer latency;
 qint64 lastCaptureMs=0;
 QString testDirectory;
 bool smokeWindowPhase=false,smokeMonitorPhase=false;
};
