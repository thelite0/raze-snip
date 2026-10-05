#include "geometry.h"
#include <QtTest>
class GeometryTest:public QObject {
 Q_OBJECT
private slots:
 void negativeOrigin(){QCOMPARE(SnipGeometry::pixelRect(QRectF(-1850,50,100,80),QRect(-1920,0,4480,1440),1,QSize(4480,1440)),QRect(70,50,100,80));}
 void reverseDrag(){QCOMPARE(SnipGeometry::pixelRect(QRectF(QPointF(200,180),QPointF(100,80)),QRect(0,0,1920,1080),1,QSize(1920,1080)),QRect(100,80,100,100));}
 void fractionalScale(){QCOMPARE(SnipGeometry::pixelRect(QRectF(-50,10,101,81),QRect(-100,0,300,200),1.25,QSize(375,250)),QRect(62,12,127,102));}
 void crossMonitor(){QCOMPARE(SnipGeometry::pixelRect(QRectF(-20,10,100,50),QRect(-1920,0,4480,1440),1.5,QSize(6720,2160)),QRect(2850,15,150,75));}
 void transparentFreeform(){QImage base(100,100,QImage::Format_ARGB32_Premultiplied);base.fill(Qt::red);QPainterPath triangle;triangle.moveTo(0,0);triangle.lineTo(100,0);triangle.lineTo(0,100);triangle.closeSubpath();QImage crop=SnipGeometry::crop(base,QRect(0,0,100,100),1,QRectF(0,0,100,100),triangle);QCOMPARE(crop.pixelColor(10,10),QColor(Qt::red));QCOMPARE(crop.pixelColor(90,90).alpha(),0);}
 void clipBounds(){QCOMPARE(SnipGeometry::pixelRect(QRectF(-10,-10,30,30),QRect(0,0,100,100),1,QSize(100,100)),QRect(0,0,20,20));}
};
QTEST_MAIN(GeometryTest)
#include "geometry-test.moc"
