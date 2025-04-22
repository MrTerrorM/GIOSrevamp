#ifndef CLICKABLEELLIPSEITEM_H
#define CLICKABLEELLIPSEITEM_H

#include <QGraphicsEllipseItem>
#include <QGraphicsSceneMouseEvent>
#include <QObject>

class ClickableEllipseItem : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT

public:
    ClickableEllipseItem(int stationId, qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent = nullptr)
        : QObject(nullptr), QGraphicsEllipseItem(x, y, width, height, parent), stationId(stationId) {
        setFlag(QGraphicsItem::ItemIsSelectable, true);
    }

signals:
    void clicked(int stationId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked(stationId);
        }
        QGraphicsEllipseItem::mousePressEvent(event);
    }

private:
    int stationId;
};

#endif // CLICKABLEELLIPSEITEM_H
