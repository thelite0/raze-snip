#include "controller.h"
#include "canvas.h"
#include <QGuiApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QTimer>
#include <QtQml>
#include <cstdio>
int main(int argc,char **argv){
 QGuiApplication app(argc,argv);
 app.setApplicationName("raze-snip");app.setApplicationDisplayName("Raze Snip");app.setOrganizationName("Raze");app.setApplicationVersion(RAZE_SNIP_VERSION);app.setDesktopFileName("org.raze.snip");app.setQuitOnLastWindowClosed(false);
 if(app.platformName()!="wayland"&&app.platformName()!="offscreen"){fprintf(stderr,"raze-snip requires a native Wayland session\n");return 1;}
 QCommandLineParser parser;parser.setApplicationDescription("Win+Shift+S capture overlay for KWin Wayland");parser.addHelpOption();parser.addVersionOption();
 for(const QString option:{"area","screen","workspace","save","copy","annotate","edit","daemon","install-shortcut","quit"})parser.addOption(QCommandLineOption(option,option=="edit"?"Edit the last captured image in this session":option=="daemon"?"Stay ready without taking a screenshot":option=="install-shortcut"?"Assign Meta+Shift+S, replacing its existing binding":option));
 parser.addOption({"smoke-test","Capture and briefly exercise the overlay; write diagnostic images to a directory","directory"});parser.process(app);
 auto bus=QDBusConnection::sessionBus();
 if(!bus.registerService("org.raze.Snip")){
  QDBusInterface remote("org.raze.Snip","/org/raze/Snip","org.raze.Snip",bus);
  QDBusReply<void> reply;
  if(parser.isSet("quit"))reply=remote.call("Quit");
  else if(!parser.isSet("daemon"))reply=remote.call("Capture",QStringList(app.arguments().mid(1)));
  else return 0;
  if(!reply.isValid()){fprintf(stderr,"raze-snip: %s\n",qPrintable(reply.error().message()));return 1;}return 0;
 }
 qmlRegisterType<FreezeCanvas>("RazeSnip",1,0,"FreezeCanvas");
 qmlRegisterType<EditorCanvas>("RazeSnip",1,0,"EditorCanvas");
 Controller controller;
 controller.warm();
 bus.registerObject("/org/raze/Snip",&controller,QDBusConnection::ExportScriptableSlots);
 if(parser.isSet("quit"))return 0;
 if(!parser.isSet("smoke-test"))controller.registerShortcut(parser.isSet("install-shortcut"));
 QTimer::singleShot(0,&controller,[&]{
  if(parser.isSet("smoke-test"))controller.runSmokeTest(parser.value("smoke-test"));
  else if(!parser.isSet("daemon")&&!parser.isSet("install-shortcut"))controller.start(app.arguments().mid(1));
 });
 return app.exec();
}
