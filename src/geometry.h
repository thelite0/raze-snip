#pragma once
#include <QRectF>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <cmath>
namespace SnipGeometry {
inline QRect pixelRect(const QRectF &global, const QRect &workspace, qreal scale, const QSize &imageSize) {
 const QRectF local = global.normalized().translated(-workspace.topLeft());
 const int x = std::floor(local.left()*scale), y = std::floor(local.top()*scale);
 const int right = std::ceil(local.right()*scale), bottom = std::ceil(local.bottom()*scale);
 return QRect(x,y,right-x,bottom-y).intersected(QRect(QPoint(),imageSize));
}
inline QImage crop(const QImage &image, const QRect &workspace, qreal scale, const QRectF &region, const QPainterPath &path = {}) {
 const QRect pixels = pixelRect(region,workspace,scale,image.size());
 QImage result = image.copy(pixels).convertToFormat(QImage::Format_ARGB32_Premultiplied);
 result.setDevicePixelRatio(1);
 if(!path.isEmpty() && !result.isNull()) {
  QImage mask(result.size(),QImage::Format_ARGB32_Premultiplied); mask.fill(Qt::transparent);
  QPainter p(&mask); p.setRenderHint(QPainter::Antialiasing); p.translate(-pixels.x(),-pixels.y()); p.scale(scale,scale); p.translate(-workspace.x(),-workspace.y()); p.fillPath(path,Qt::white); p.end();
  QPainter cut(&result); cut.setCompositionMode(QPainter::CompositionMode_DestinationIn); cut.drawImage(0,0,mask);
 }
 return result;
}
}
