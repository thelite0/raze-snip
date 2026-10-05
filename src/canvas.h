#pragma once
#include <QQuickPaintedItem>
#include <QImage>
#include <QPainterPath>
#include <QColor>
class Controller;
class FreezeCanvas : public QQuickPaintedItem {
 Q_OBJECT
 Q_PROPERTY(QObject* controller READ controller WRITE setController NOTIFY controllerChanged)
 Q_PROPERTY(qreal screenX MEMBER sx NOTIFY layoutChanged)
 Q_PROPERTY(qreal screenY MEMBER sy NOTIFY layoutChanged)
 Q_PROPERTY(qreal dim MEMBER dim NOTIFY layoutChanged)
public:
 explicit FreezeCanvas(QQuickItem *parent=nullptr);
 QObject *controller()const;
 void setController(QObject *object);
 void paint(QPainter *p) override;
signals:
 void controllerChanged(); void layoutChanged();
private:
 Controller *c=nullptr; qreal sx=0,sy=0,dim=0.6;
};
class EditorCanvas : public QQuickPaintedItem {
 Q_OBJECT
 Q_PROPERTY(int tool MEMBER tool NOTIFY settingsChanged)
 Q_PROPERTY(QColor ink MEMBER ink NOTIFY settingsChanged)
 Q_PROPERTY(qreal stroke MEMBER stroke NOTIFY settingsChanged)
 Q_PROPERTY(QString text MEMBER text NOTIFY settingsChanged)
 Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
 Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
public:
 explicit EditorCanvas(QQuickItem *parent=nullptr);
 void paint(QPainter *p)override;
 bool canUndo()const{return !undoStack.isEmpty();}
 bool canRedo()const{return !redoStack.isEmpty();}
 Q_INVOKABLE void load(const QImage &image);
 Q_INVOKABLE void down(qreal x,qreal y);
 Q_INVOKABLE void move(qreal x,qreal y);
 Q_INVOKABLE void up(qreal x,qreal y);
 Q_INVOKABLE void undo(); Q_INVOKABLE void redo();
 Q_INVOKABLE QImage image()const{return current;}
signals:
 void settingsChanged(); void historyChanged();
private:
 QPointF imagePoint(qreal x,qreal y)const;
 QRectF targetRect()const;
 void render(QImage &image)const;
 void remember();
 QImage current,before;
 QList<QImage> undoStack,redoStack;
 QPainterPath path;
 QPointF start,end;
 bool drawing=false;
 int tool=0,number=1;
 QColor ink="#ff453a";
 qreal stroke=3;
 QString text="Text";
};
