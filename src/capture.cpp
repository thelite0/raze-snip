#include "capture.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QColorSpace>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cmath>
Capture::Capture(QObject *parent):QObject(parent) {
 timeout.setSingleShot(true); timeout.setInterval(10000);
 connect(&timeout,&QTimer::timeout,this,[this]{fail("KWin capture timed out after 10 seconds");});
}
Capture::~Capture(){cleanup();}
void Capture::cleanup(){timeout.stop(); notifier.reset(); if(fd>=0)close(fd); fd=-1;}
void Capture::fail(const QString &s){cleanup(); bytes.clear(); emit failed(s);}
void Capture::start(const QString &method,const QVariantList &arguments){
 if(busy())return;
 int pipes[2]; if(pipe2(pipes,O_CLOEXEC)<0){emit failed("Cannot create screenshot pipe");return;}
 fd=pipes[0]; fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);
 bytes.clear(); metadata.clear(); eof=false; replied=false;
 notifier=std::make_unique<QSocketNotifier>(fd,QSocketNotifier::Read,this);
 connect(notifier.get(),&QSocketNotifier::activated,this,[this]{drain();});
 auto msg=QDBusMessage::createMethodCall("org.kde.KWin","/org/kde/KWin/ScreenShot2","org.kde.KWin.ScreenShot2",method);
 QVariantList args=arguments;
 args << QVariantMap{{"native-resolution",true},{"include-cursor",false},{"hide-caller-windows",true},{"include-decoration",true},{"include-shadow",false}} << QVariant::fromValue(QDBusUnixFileDescriptor(pipes[1]));
 msg.setArguments(args);
 auto *watcher=new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(msg,10000),this);
 close(pipes[1]); timeout.start();
 connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,watcher]{
  QDBusPendingReply<QVariantMap> reply=*watcher; watcher->deleteLater();
  if(fd<0)return;
  if(reply.isError()){fail(reply.error().name()+": "+reply.error().message());return;}
  metadata=reply.value(); replied=true; finish();
 });
}
void Capture::drain(){
 char buffer[65536];
 for(;;){
  const ssize_t n=read(fd,buffer,sizeof buffer);
  if(n>0){bytes.append(buffer,n); if(bytes.size()>512*1024*1024){fail("Screenshot exceeds 512 MiB safety limit");return;}}
  else if(n==0){eof=true; notifier->setEnabled(false); break;}
  else if(errno==EINTR)continue;
  else if(errno==EAGAIN)break;
  else {fail("Screenshot pipe read failed");return;}
 }
 finish();
}
void Capture::finish(){
 if(!replied||!eof||fd<0)return;
 const quint64 width=metadata.value("width").toULongLong(),height=metadata.value("height").toULongLong(),stride=metadata.value("stride").toULongLong();
 const auto format=static_cast<QImage::Format>(metadata.value("format").toUInt());
 const qreal scale=metadata.value("scale",1.0).toDouble();
 if(metadata.value("type")!="raw"||!width||!height||width>32768||height>32768||stride>512*1024||stride*height!=quint64(bytes.size())||format<=QImage::Format_Invalid||format>=QImage::NImageFormats||!std::isfinite(scale)||scale<=0){fail("Invalid KWin raw image metadata");return;}
 QImage view(reinterpret_cast<const uchar*>(bytes.constData()),int(width),int(height),qsizetype(stride),format);
 if(view.isNull()){fail("Unsupported screenshot image layout");return;}
 QImage image=view.copy(); image.setDevicePixelRatio(1); image.setColorSpace(QColorSpace::SRgb);
 cleanup(); bytes.clear(); emit ready(image,scale);
}
