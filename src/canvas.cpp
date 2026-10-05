#include "canvas.h"
#include "controller.h"
#include <QPainter>
#include <QFont>
#include <cmath>
FreezeCanvas::FreezeCanvas(QQuickItem *parent):QQuickPaintedItem(parent){setOpaquePainting(true);connect(this,&FreezeCanvas::layoutChanged,this,[this]{update();});}
QObject *FreezeCanvas::controller()const{return c;}
void FreezeCanvas::setController(QObject *o){
 if(c)disconnect(c,nullptr,this,nullptr);
 c=qobject_cast<Controller*>(o);
 if(c)connect(c,&Controller::changed,this,[this]{update();});
 emit controllerChanged();update();
}
void FreezeCanvas::paint(QPainter *p){
 if(!c||c->snapshot().isNull()){p->fillRect(boundingRect(),Qt::black);return;}
 const qreal scale=c->captureScale();const auto origin=c->workspace().topLeft();
 const QRectF source((sx-origin.x())*scale,(sy-origin.y())*scale,width()*scale,height()*scale);
 p->drawImage(boundingRect(),c->snapshot(),source);
 QPainterPath outside;outside.addRect(boundingRect());
 QPainterPath selected=c->selectedPath().translated(-sx,-sy);
 p->setRenderHint(QPainter::Antialiasing);
 outside=outside.subtracted(selected);
 p->fillPath(outside,QColor(0,0,0,qRound(255*dim)));
 if(!selected.isEmpty()){
  QPen border(Qt::white,1);border.setCosmetic(true);p->setPen(border);p->setBrush(Qt::NoBrush);p->drawPath(selected);
 }
}
EditorCanvas::EditorCanvas(QQuickItem *parent):QQuickPaintedItem(parent){connect(this,&EditorCanvas::settingsChanged,this,[this]{update();});}
QRectF EditorCanvas::targetRect()const{
 if(current.isNull())return {};
 const qreal s=qMin((width()-32)/current.width(),(height()-32)/current.height());
 const QSizeF size=current.size()*qMax(qreal(.01),s);return QRectF((width()-size.width())/2,(height()-size.height())/2,size.width(),size.height());
}
QPointF EditorCanvas::imagePoint(qreal x,qreal y)const{const QRectF t=targetRect();if(t.isEmpty())return {};return QPointF((x-t.x())*current.width()/t.width(),(y-t.y())*current.height()/t.height());}
void EditorCanvas::paint(QPainter *p){
 if(current.isNull())return;
 QImage display=current;if(drawing){display=before.copy();render(display);}
 p->setRenderHint(QPainter::SmoothPixmapTransform);p->drawImage(targetRect(),display);
}
void EditorCanvas::load(const QImage &i){current=i.convertToFormat(QImage::Format_ARGB32_Premultiplied);undoStack.clear();redoStack.clear();number=1;drawing=false;emit historyChanged();update();}
void EditorCanvas::remember(){undoStack<<current;if(undoStack.size()>30)undoStack.removeFirst();redoStack.clear();}
void EditorCanvas::down(qreal x,qreal y){
 if(current.isNull()||!targetRect().contains(x,y))return;
 start=end=imagePoint(x,y);
 if(tool==7){ink=current.pixelColor(qBound(0,qRound(start.x()),current.width()-1),qBound(0,qRound(start.y()),current.height()-1));emit settingsChanged();return;}
 before=current;drawing=true;path={};path.moveTo(start);update();
}
void EditorCanvas::move(qreal x,qreal y){if(!drawing)return;end=imagePoint(x,y);if(tool==2)path.lineTo(end);update();}
void EditorCanvas::up(qreal x,qreal y){if(!drawing)return;move(x,y);remember();current=before.copy();render(current);if(tool==5)++number;drawing=false;before={};emit historyChanged();update();}
void EditorCanvas::render(QImage &i)const{
 QPainter p(&i);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(ink,stroke,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
 QRectF rect=QRectF(start,end).normalized();
 switch(tool){
 case 0:{p.drawLine(start,end);const double a=std::atan2(end.y()-start.y(),end.x()-start.x());const double len=qMax(qreal(12),stroke*4);QPointF a1=end-QPointF(std::cos(a-.5)*len,std::sin(a-.5)*len),a2=end-QPointF(std::cos(a+.5)*len,std::sin(a+.5)*len);p.drawLine(end,a1);p.drawLine(end,a2);break;}
 case 1:p.drawRect(rect);break;
 case 2:p.drawPath(path);break;
 case 3:{QFont font("sans-serif");font.setPixelSize(qMax(14,qRound(stroke*7)));p.setFont(font);p.drawText(start,text);break;}
 case 4:{QRect r=rect.toAlignedRect().intersected(i.rect());if(!r.isEmpty()){p.end();QImage pixels=i.copy(r).scaled(qMax(1,r.width()/12),qMax(1,r.height()/12),Qt::IgnoreAspectRatio,Qt::SmoothTransformation).scaled(r.size(),Qt::IgnoreAspectRatio,Qt::FastTransformation);QPainter cut(&i);cut.drawImage(r.topLeft(),pixels);}break;}
 case 5:{p.setBrush(ink);p.drawEllipse(start,14+stroke,14+stroke);p.setPen(Qt::white);QFont font("sans-serif");font.setBold(true);font.setPixelSize(18);p.setFont(font);p.drawText(QRectF(start.x()-18,start.y()-18,36,36),Qt::AlignCenter,QString::number(number));break;}
 case 6:{QRect r=rect.toAlignedRect().intersected(i.rect());if(!r.isEmpty()){p.end();QImage small=i.copy(r).scaled(qMax(1,r.width()/16),qMax(1,r.height()/16),Qt::IgnoreAspectRatio,Qt::SmoothTransformation).scaled(r.size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);QPainter cut(&i);cut.drawImage(r.topLeft(),small);}break;}
 }
}
void EditorCanvas::undo(){if(undoStack.isEmpty())return;redoStack<<current;current=undoStack.takeLast();emit historyChanged();update();}
void EditorCanvas::redo(){if(redoStack.isEmpty())return;undoStack<<current;current=redoStack.takeLast();emit historyChanged();update();}
