#include "controller.h"
#include "canvas.h"
#include "geometry.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QCursor>
#include <QMimeData>
#include <QScreen>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QQmlContext>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QBuffer>
#include <QImageWriter>
#include <QSaveFile>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QAction>
#include <KGlobalAccel>
#include <KSystemClipboard>
#include <LayerShellQt/Window>
#include <cstdio>

Controller::Controller(QObject *parent):QObject(parent),backend(this){
 engine.rootContext()->setContextProperty("snip",this);
 connect(&backend,&::Capture::ready,this,&Controller::captured);
 connect(&backend,&::Capture::failed,this,&Controller::fail);
 connect(qGuiApp,&QGuiApplication::screenRemoved,this,[this]{cancel();for(auto *v:overlays)v->deleteLater();overlays.clear();});
 connect(qGuiApp,&QGuiApplication::screenAdded,this,[this]{cancel();for(auto *v:overlays)v->deleteLater();overlays.clear();});
}
Controller::~Controller(){unloadScript();for(auto *v:overlays)delete v;delete editor;}
void Controller::registerShortcut(bool replaceConflict){
 auto *action=new QAction("Capture a snip",this); action->setObjectName("CaptureArea");
 action->setProperty("componentName","org.raze.snip");action->setProperty("componentDisplayName","Raze Snip");
 const QKeySequence seq(Qt::META|Qt::SHIFT|Qt::Key_S);
 const QKeySequence print(Qt::Key_Print);
 if(replaceConflict){KGlobalAccel::stealShortcutSystemwide(seq);KGlobalAccel::stealShortcutSystemwide(print);}
 KGlobalAccel::self()->setDefaultShortcut(action,{seq,print});
 KGlobalAccel::self()->setShortcut(action,{seq,print},replaceConflict?KGlobalAccel::NoAutoloading:KGlobalAccel::Autoloading);
 connect(action,&QAction::triggered,this,[this]{start({"--area"});});
}
void Controller::prepareOverlays(){
 if(!overlays.isEmpty())return;
 for(auto *screen:qGuiApp->screens()){
  auto *view=new QQuickView(&engine,nullptr);
  view->setProperty("captureOverlay",true);
  view->setScreen(screen);view->setTitle("Raze Snip Capture");view->setColor(Qt::black);
  view->setFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
  view->setResizeMode(QQuickView::SizeRootObjectToView);
  view->setInitialProperties({{"screenX",screen->geometry().x()},{"screenY",screen->geometry().y()},{"toolbarVisible",screen->name()==activeScreenName}});
  view->setSource(QUrl("qrc:/qml/Overlay.qml"));
  if(view->status()==QQuickView::Error){view->deleteLater();fail("Capture overlay QML failed to load");return;}
  auto *layer=LayerShellQt::Window::get(view);
  layer->setScreen(screen);// KWin treats unrecognized layer scopes as normal windows and scales them on opening.
  // Utility surfaces are excluded from its normal-window and popup animations.
  layer->setScope("utility");layer->setLayer(LayerShellQt::Window::LayerOverlay);
  layer->setAnchors(LayerShellQt::Window::Anchors::fromInt(15));
  layer->setExclusiveZone(-1);layer->setDesiredSize(QSize(0,0));
  layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
  view->resize(screen->geometry().size());
  connect(screen,&QScreen::geometryChanged,view,[this,view](const QRect &g){cancel();view->resize(g.size());if(view->rootObject()){view->rootObject()->setProperty("screenX",g.x());view->rootObject()->setProperty("screenY",g.y());}});
  connect(view,&QQuickView::closing,this,[this]{cancel();});
  overlays<<view;
 }
}
void Controller::start(const QStringList &args){
 if(backend.busy()||selecting)return;
 if(editor)editor->hide();
 latency.start();region={};freePath={};keyboardOffset={};dragging=false;snipMode=0;
 saveRequested=args.contains("--save");annotateRequested=args.contains("--annotate");
 screenRequested=args.contains("--screen");workspaceRequested=args.contains("--workspace");
 if(args.contains("--edit")){editLast();return;}
 bounds={};for(auto *s:qGuiApp->screens())bounds=bounds.united(s->geometry());
 if(bounds.isEmpty()){fail("No Wayland outputs found");return;}
 QDBusInterface kwin("org.kde.KWin","/KWin","org.kde.KWin");
 QDBusReply<QString> active=kwin.call("activeOutputName");activeScreenName=active.isValid()?active.value():qGuiApp->primaryScreen()->name();
 if(!screenRequested&&!workspaceRequested){prepareOverlays();snapshotWindows();}
 backend.start(screenRequested?"CaptureScreen":"CaptureWorkspace",screenRequested?QVariantList{activeScreenName}:QVariantList{});
}
void Controller::captured(const QImage &image,qreal capturedScale){
 scale=capturedScale;lastCaptureMs=latency.elapsed();
 if(windowCapturePending){
  windowCapturePending=false;finish(image,windowCaptureModifiers);
  if(smokeWindowPhase){
   const QString phase=smokeMonitorPhase?"monitor":"window";
   image.save(testDirectory+"/"+phase+".png");
   QTimer::singleShot(200,this,[this,phase]{
    QFile report(testDirectory+"/"+phase+"-status.json");if(report.open(QIODevice::WriteOnly))report.write(QJsonDocument(QJsonObject::fromVariantMap(Status())).toJson());
    const int code=lastImage.width()>32&&lastImage.height()>32?0:1;
    if(!smokeMonitorPhase&&code==0){
     smokeMonitorPhase=true;windows={};selecting=true;snipMode=2;
     QScreen *target=qGuiApp->primaryScreen();for(auto *s:qGuiApp->screens())if(s->name()==activeScreenName)target=s;
     const auto point=target->geometry().center();move(point.x(),point.y());
     if(region!=QRectF(target->geometry())||windowScreenName!=target->name()){fail("Monitor target selection failed");return;}
     accept();
    }else QTimer::singleShot(0,qGuiApp,[code]{qGuiApp->exit(code);});
   });
  }
  return;
 }
 frozen=image;
 if(!testDirectory.isEmpty()){
  QDir().mkpath(testDirectory);frozen.save(testDirectory+"/capture.png");
  prepareOverlays();showOverlays();
  QTimer::singleShot(100,this,[this]{
   // Exercise the real Wayland constraint on the surface under the pointer.
   for(auto *view:overlays)if(view->screen()->geometry().contains(QCursor::pos())){
    const QRect g=view->screen()->geometry();
    for(const QPoint corner:{g.topLeft(),g.topRight(),g.bottomLeft(),g.bottomRight()}){
     cornerGuard.update(view,corner-g.topLeft());
     if(cornerGuard.requested())break;
    }
    break;
   }
  });
  QTimer::singleShot(180,this,[this]{
   QFile guardReport(testDirectory+"/corner-guard.json");
   if(guardReport.open(QIODevice::WriteOnly))guardReport.write(QJsonDocument(QJsonObject{{"available",cornerGuard.available()},{"requested",cornerGuard.requested()},{"active",cornerGuard.active()}}).toJson());
   const bool guardPassed=cornerGuard.available()&&cornerGuard.requested()&&cornerGuard.active();
   cornerGuard.release();
   if(!guardPassed){fprintf(stderr,"raze-snip: pointer constraint smoke check failed\n");cancel();qGuiApp->exit(1);return;}
   for(auto *v:overlays)if(v->rootObject()->property("toolbarVisible").toBool())v->grabWindow().save(testDirectory+"/overlay.png");
   QRect g=qGuiApp->primaryScreen()->geometry();
   for(auto *s:qGuiApp->screens())if(s->name()==activeScreenName)g=s->geometry();
   down(g.x()+100,g.y()+150,Qt::LeftButton,0);move(g.x()+420,g.y()+350);
   QTimer::singleShot(80,this,[this,g]{
    for(auto *v:overlays)if(v->rootObject()->property("toolbarVisible").toBool())v->grabWindow().save(testDirectory+"/selection.png");
    up(g.x()+420,g.y()+350,Qt::LeftButton,0);
    QTimer::singleShot(250,this,[this]{
     QFile report(testDirectory+"/status.json");if(report.open(QIODevice::WriteOnly))report.write(QJsonDocument(QJsonObject::fromVariantMap(Status())).toJson());
     lastImage.save(testDirectory+"/crop.png");const int code=lastImage.size()==QSize(qCeil(320*scale),qCeil(200*scale))?0:1;
     if(code==0&&!windows.isEmpty()){
      smokeWindowPhase=true;windowCapturePending=true;windowCaptureModifiers=0;latency.restart();
      QJsonObject biggest;double largest=0;
      for(const auto &value:windows){auto w=value.toObject();if(w["monitor"].toBool())continue;double area=w["w"].toDouble()*w["h"].toDouble();if(area>largest){largest=area;biggest=w;}}
      backend.start("CaptureWindow",{biggest["id"].toString()});
     }else QTimer::singleShot(0,qGuiApp,[code]{qGuiApp->exit(code);});
    });
   });
  });return;
 }
 if(screenRequested||workspaceRequested){finish(image,0);return;}
 showOverlays();
}
void Controller::showOverlays(){
 selecting=true;emit changed();
 for(auto *v:overlays){v->rootObject()->setProperty("toolbarVisible",v->screen()->name()==activeScreenName);v->show();QMetaObject::invokeMethod(v->rootObject(),"begin");}
 for(auto *v:overlays)if(v->screen()->name()==activeScreenName)v->requestActivate();
 fprintf(stderr,"raze-snip: workspace ready in %lld ms (%dx%d, scale %.3f)\n",static_cast<long long>(latency.elapsed()),frozen.width(),frozen.height(),scale);
}
void Controller::hideOverlays(){cornerGuard.release();for(auto *v:overlays)v->hide();selecting=false;dragging=false;emit changed();unloadScript();}
void Controller::cancel(){hideOverlays();region={};freePath={};frozen={};emit changed();}
void Controller::setMode(int value){
 if(value<0||value>3)return;
 snipMode=value;region={};freePath={};dragging=false;emit changed();
 if(value==3&&selecting)finish(frozen,0);
}
QPainterPath Controller::selectedPath()const{
 if(snipMode==1&&!freePath.isEmpty()){QPainterPath p=freePath;p.closeSubpath();return p;}
 QPainterPath p;if(!region.isEmpty())p.addRect(region);return p;
}
void Controller::down(qreal x,qreal y,int button,int){
 if(button==Qt::RightButton){cancel();return;}
 if(!selecting||button!=Qt::LeftButton)return;
 keyboardOffset={};pointer=QPointF(x,y);anchor=pointer;dragging=true;
 if(snipMode==0){region={anchor,QSizeF()};}
 else if(snipMode==1){freePath={};freePath.moveTo(anchor);region={};}
 else if(snipMode==2){move(x,y);}
 emit changed();
}
void Controller::move(qreal x,qreal y){
 if(!selecting)return;
 pointer=QPointF(x,y)+keyboardOffset;
 if(snipMode==0&&dragging)region=QRectF(anchor,pointer).normalized().intersected(bounds);
 else if(snipMode==1&&dragging){freePath.lineTo(pointer);region=freePath.boundingRect().intersected(bounds);}
 else if(snipMode==2){
  region={};windowHandle.clear();windowScreenName.clear();for(auto it=windows.end();it!=windows.begin();){const auto w=(*--it).toObject();QRectF r(w["x"].toDouble(),w["y"].toDouble(),w["w"].toDouble(),w["h"].toDouble());if(r.contains(pointer)){
   if(w["monitor"].toBool()){
    for(auto *s:qGuiApp->screens())if(s->geometry().contains(pointer.toPoint())){region=s->geometry();windowScreenName=s->name();break;}
   }else{region=r.intersected(bounds);windowHandle=w["id"].toString();}
   break;
  }}
  if(region.isEmpty()){
   for(auto *screen:qGuiApp->screens()){
    if(screen->geometry().contains(pointer.toPoint())){
     region=screen->geometry();windowScreenName=screen->name();break;
    }
   }
  }
 }
 emit changed();
}
void Controller::up(qreal x,qreal y,int button,int modifiers){
 if(button==Qt::RightButton){cancel();return;}
 if(!dragging||button!=Qt::LeftButton)return;
 move(x,y);dragging=false;if(region.width()>=1&&region.height()>=1)accept(modifiers);
}
void Controller::accept(int modifiers){
 if(!selecting||region.isEmpty())return;
 if(snipMode==2&&(!windowHandle.isEmpty()||!windowScreenName.isEmpty())){
  windowCapturePending=true;windowCaptureModifiers=modifiers;hideOverlays();latency.restart();if(!windowScreenName.isEmpty())backend.start("CaptureScreen",{windowScreenName});
  else backend.start("CaptureWindow",{windowHandle});
  return;
 }
 const QImage result=SnipGeometry::crop(frozen,bounds,scale,region,snipMode==1?selectedPath():QPainterPath());
 finish(result,modifiers);
}
void Controller::key(int key,int mods){
 if(key==Qt::Key_Escape){cancel();return;}
 if((key==Qt::Key_Return||key==Qt::Key_Enter)&&!region.isEmpty()){accept(mods);return;}
 if(region.isEmpty())return;
 int dx=0,dy=0,step=(mods&Qt::ShiftModifier)?10:1;
 if(key==Qt::Key_Left)dx=-step;
 if(key==Qt::Key_Right)dx=step;
 if(key==Qt::Key_Up)dy=-step;
 if(key==Qt::Key_Down)dy=step;
 if(dx||dy){const auto shifted=region.translated(dx,dy);if(bounds.contains(shifted.toAlignedRect())){region=shifted;freePath.translate(dx,dy);anchor+=QPointF(dx,dy);pointer+=QPointF(dx,dy);keyboardOffset+=QPointF(dx,dy);emit changed();}}
}
void Controller::finish(const QImage &image,int mods){
 if(image.isNull()){fail("Selection produced an empty image");return;}
 hideOverlays();lastImage=image;clipboard(image,saveRequested||(mods&Qt::ShiftModifier));frozen={};
 if(annotateRequested||(mods&Qt::ControlModifier))editLast();
}
void Controller::clipboard(const QImage &image,bool save){
 const quint64 serial=++clipboardSerial;
 auto *initial=new QMimeData;initial->setImageData(image);KSystemClipboard::instance()->setMimeData(initial,QClipboard::Clipboard);
 const QString directory=QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)+"/Screenshots";
 const QString filename="Screenshot_"+QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss")+".png";
 auto *watcher=new QFutureWatcher<QPair<QByteArray,QString>>(this);
 connect(watcher,&QFutureWatcherBase::finished,this,[this,watcher,image,serial]{
  const auto result=watcher->result();watcher->deleteLater();
  if(!result.second.isEmpty())fprintf(stderr,"raze-snip: %s\n",qPrintable(result.second));
  if(!result.first.isEmpty()&&serial==clipboardSerial&&KSystemClipboard::instance()->ownsClipboard()){auto *mime=new QMimeData;mime->setImageData(image);mime->setData("image/png",result.first);KSystemClipboard::instance()->setMimeData(mime,QClipboard::Clipboard);}
 });
 watcher->setFuture(QtConcurrent::run([image,save,directory,filename]{
  QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);QImageWriter writer(&buffer,"png");writer.setCompression(1);
  if(!writer.write(image))return qMakePair(QByteArray{},"PNG encoding failed: "+writer.errorString());
  QString error;
  if(save){
   if(!QDir().mkpath(directory))error="Cannot create "+directory;
   else {QString path=directory+"/"+filename;
    if(QFile::exists(path))path.chop(4),path+="_"+QDateTime::currentDateTime().toString("zzz")+".png";
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(png)!=png.size()||!file.commit())error="Save failed: "+file.errorString();
   }
  }
  return qMakePair(png,error);
 }));
}
void Controller::snapshotWindows(){
 windows={};unloadScript();
 script=std::make_unique<QTemporaryFile>(QDir::tempPath()+"/raze-snip-XXXXXX.js");if(!script->open())return;
 QByteArray code=R"JS((function(){
 const result=[];
 const order=workspace.stackingOrder;
 for(let i=0;i<order.length;++i){
  const w=order[i];
  if(w.deleted||w.hidden||w.minimized||w.tooltip||w.pid===SNIP_PID)continue;
  if(!w.onAllDesktops&&w.desktops.indexOf(workspace.currentDesktopForScreen(w.output))<0)continue;
  if(w.activities.length&&w.activities.indexOf(workspace.currentActivity)<0)continue;
  const r=w.frameGeometry;
  result.push({id:String(w.internalId),monitor:w.desktopWindow||w.dock,x:r.x,y:r.y,w:r.width,h:r.height});
 }
 callDBus('org.raze.Snip','/org/raze/Snip','org.raze.Snip','WindowSnapshot',JSON.stringify(result));
})();)JS";
 code.replace("SNIP_PID",QByteArray::number(QCoreApplication::applicationPid()));
 script->write(code);script->flush();scriptName="raze-snip-transient";
 QDBusInterface scripting("org.kde.KWin","/Scripting","org.kde.kwin.Scripting");
 QDBusReply<int> id=scripting.call("loadScript",script->fileName(),scriptName);
 if(!id.isValid()||id.value()<0){fprintf(stderr,"raze-snip: window enumeration unavailable\n");return;}
 QDBusMessage run=QDBusMessage::createMethodCall("org.kde.KWin","/Scripting/Script"+QString::number(id.value()),"org.kde.kwin.Script","run");
 QDBusConnection::sessionBus().asyncCall(run);
}
void Controller::WindowSnapshot(const QString &json){
 windows=QJsonDocument::fromJson(json.toUtf8()).array();
 if(selecting&&snipMode==2)move(pointer.x(),pointer.y());
}
void Controller::unloadScript(){
 if(!scriptName.isEmpty()){
  auto m=QDBusMessage::createMethodCall("org.kde.KWin","/Scripting","org.kde.kwin.Scripting","unloadScript");m<<scriptName;QDBusConnection::sessionBus().asyncCall(m);scriptName.clear();
 }
 script.reset();
}
void Controller::editLast(){
 if(lastImage.isNull()){fprintf(stderr,"raze-snip: no capture to edit in this session\n");return;}
 if(!editor){
  editor=new QQuickView(&engine,nullptr);editor->setTitle("Raze Snip — Edit capture");editor->setResizeMode(QQuickView::SizeRootObjectToView);editor->setSource(QUrl("qrc:/qml/Editor.qml"));editor->resize(1000,700);
  if(editor->status()==QQuickView::Error){fail("Editor QML failed to load");return;}
  connect(editor,&QQuickView::closing,this,[this]{editor->hide();});
 }
 editor->show();emit editorImage(lastImage);editor->requestActivate();
}
void Controller::editorDone(const QImage &image,bool save){if(!image.isNull()){lastImage=image;clipboard(image,save);}if(editor)editor->hide();}
void Controller::fail(const QString &s){windowCapturePending=false;hideOverlays();fprintf(stderr,"raze-snip: %s\n",qPrintable(s));if(!testDirectory.isEmpty())qGuiApp->exit(1);}
QVariantMap Controller::Status()const{
 const QMimeData *mime=KSystemClipboard::instance()->mimeData(QClipboard::Clipboard);
 const QStringList formats=mime?mime->formats():QStringList{};
 return {{"cornerGuardAvailable",cornerGuard.available()},{"cornerGuardRequested",cornerGuard.requested()},{"cornerGuardActive",cornerGuard.active()},{"selecting",selecting},{"captureMs",lastCaptureMs},{"imageWidth",lastImage.width()},{"imageHeight",lastImage.height()},{"scale",scale},{"windowCount",windows.size()},{"screens",qGuiApp->screens().size()},{"clipboardFormats",formats}};
}
void Controller::runSmokeTest(const QString &d){testDirectory=d;emit changed();start({"--area"});}
void Controller::Quit(){cancel();qGuiApp->quit();}
