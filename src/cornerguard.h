#pragma once
#include <QObject>
#include <QPointF>
struct zwp_pointer_constraints_v1;
struct zwp_confined_pointer_v1;
class QWindow;

// KWin suppresses its electric borders while a Wayland pointer constraint is
// active. Only constrain near exposed corners; never confine across an output seam.
class CornerGuard final : public QObject {
public:
 explicit CornerGuard(QObject *parent=nullptr);
 ~CornerGuard() override;
 void update(QWindow *window, const QPointF &local);
 void release();
 bool requested() const { return constraint != nullptr; }
 bool active() const { return confined; }
 bool available() const { return manager != nullptr; }
private:
 bool eventFilter(QObject *watched, QEvent *event) override;
 zwp_pointer_constraints_v1 *manager=nullptr;
 zwp_confined_pointer_v1 *constraint=nullptr;
 QWindow *target=nullptr;
 bool confined=false;
};
