#include "cornerguard.h"
#include "pointer-constraints-client.h"
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QMouseEvent>
#include <QtGui/qguiapplication_platform.h>
#include <qpa/qplatformnativeinterface.h>
#include <wayland-client.h>
#include <cstring>

CornerGuard::CornerGuard(QObject *parent):QObject(parent) {
 auto *native=qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
 if(!native)return;
 auto *registry=wl_display_get_registry(native->display());
 static const wl_registry_listener listener={
  [](void *data,wl_registry *registry,uint32_t name,const char *interface,uint32_t){
   auto *self=static_cast<CornerGuard*>(data);
   if(std::strcmp(interface,zwp_pointer_constraints_v1_interface.name)==0)
    self->manager=static_cast<zwp_pointer_constraints_v1*>(wl_registry_bind(registry,name,&zwp_pointer_constraints_v1_interface,1));
  }, [](void*,wl_registry*,uint32_t){}
 };
 wl_registry_add_listener(registry,&listener,this);
 wl_display_roundtrip(native->display());
 wl_registry_destroy(registry);
 qGuiApp->installEventFilter(this);
}
CornerGuard::~CornerGuard(){release();if(manager)zwp_pointer_constraints_v1_destroy(manager);}
void CornerGuard::release(){
 if(constraint)zwp_confined_pointer_v1_destroy(constraint);
 constraint=nullptr;target=nullptr;confined=false;
}
bool CornerGuard::eventFilter(QObject *watched,QEvent *event){
 auto *window=qobject_cast<QWindow*>(watched);
 if(window&&window->property("captureOverlay").toBool()){
  if(event->type()==QEvent::Enter)update(window,static_cast<QEnterEvent*>(event)->position());
  else if(event->type()==QEvent::MouseMove)update(window,static_cast<QMouseEvent*>(event)->position());
  else if(event->type()==QEvent::Hide&&window==target)release();
 }
 return false;
}
void CornerGuard::update(QWindow *window,const QPointF &local){
 if(!manager||!window||!window->isVisible())return;
 const auto geometry=window->screen()->geometry();
 const QPoint global=geometry.topLeft()+local.toPoint();
 bool nearCorner=false;
 for(const QPoint corner:{geometry.topLeft(),geometry.topRight(),geometry.bottomLeft(),geometry.bottomRight()}){
  if(qAbs(global.x()-corner.x())>24||qAbs(global.y()-corner.y())>24)continue;
  const int dx=corner.x()==geometry.left()?-1:1;
  const int dy=corner.y()==geometry.top()? -1:1;
  bool seam=false;
  for(auto *screen:qGuiApp->screens())if(screen!=window->screen()){
   const QRect other=screen->geometry();
   seam|=other.contains(corner+QPoint(dx,0))||other.contains(corner+QPoint(0,dy));
  }
  if(!seam){nearCorner=true;break;}
 }
 if(!nearCorner){release();return;}
 if(constraint&&target==window)return;
 release();
 auto *native=qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
 auto *surface=static_cast<wl_surface*>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface",window));
 if(!surface||!native->pointer())return;
 target=window;
 // KWin permits constraints only on the active surface, including layer surfaces.
 if(QGuiApplication::focusWindow()!=window)window->requestActivate();
 constraint=zwp_pointer_constraints_v1_confine_pointer(manager,surface,native->pointer(),nullptr,ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
 static const zwp_confined_pointer_v1_listener listener={
  [](void *data,zwp_confined_pointer_v1*){static_cast<CornerGuard*>(data)->confined=true;},
  [](void *data,zwp_confined_pointer_v1*){static_cast<CornerGuard*>(data)->confined=false;}
 };
 zwp_confined_pointer_v1_add_listener(constraint,&listener,this);
 wl_surface_commit(surface);
 wl_display_flush(native->display());
}
