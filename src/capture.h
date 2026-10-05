#pragma once
#include <QObject>
#include <QImage>
#include <QSocketNotifier>
#include <QTimer>
#include <QVariantMap>
#include <memory>
class Capture : public QObject {
 Q_OBJECT
public:
 explicit Capture(QObject *parent=nullptr);
 ~Capture() override;
 void start(const QString &method, const QVariantList &arguments={});
 bool busy() const { return fd >= 0; }
signals:
 void ready(const QImage &image,qreal scale);
 void failed(const QString &message);
private:
 void drain(); void finish(); void fail(const QString &reason); void cleanup();
 int fd=-1;
 QByteArray bytes;
 QVariantMap metadata;
 bool eof=false, replied=false;
 std::unique_ptr<QSocketNotifier> notifier;
 QTimer timeout;
};
