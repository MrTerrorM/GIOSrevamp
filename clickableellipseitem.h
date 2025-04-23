/**
 * @file clickableellipseitem.h
 * @brief Klasa reprezentująca klikalną elipsę na mapie stacji pogodowych.
 */

#ifndef CLICKABLEELLIPSEITEM_H
#define CLICKABLEELLIPSEITEM_H

#include <QGraphicsEllipseItem>
#include <QGraphicsSceneMouseEvent>
#include <QObject>

/**
 * @class ClickableEllipseItem
 * @brief Klasa dziedzicząca po QGraphicsEllipseItem, umożliwiająca klikanie elipsy i wysyłanie sygnału z ID stacji.
 *
 * Reprezentuje elipsę na mapie, która po kliknięciu emituje sygnał z identyfikatorem powiązanej stacji pogodowej.
 */
class ClickableEllipseItem : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy ClickableEllipseItem.
     * @param stationId Identyfikator stacji pogodowej.
     * @param x Pozycja x elipsy na scenie.
     * @param y Pozycja y elipsy na scenie.
     * @param width Szerokość elipsy.
     * @param height Wysokość elipsy.
     * @param parent Wskaźnik do nadrzędnego elementu graficznego (domyślnie nullptr).
     */
    ClickableEllipseItem(int stationId, qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent = nullptr)
        : QObject(nullptr), QGraphicsEllipseItem(x, y, width, height, parent), stationId(stationId) {
        setFlag(QGraphicsItem::ItemIsSelectable, true);
    }

signals:
    /**
     * @brief Sygnał emitowany po kliknięciu elipsy.
     * @param stationId Identyfikator klikniętej stacji.
     */
    void clicked(int stationId);

protected:
    /**
     * @brief Obsługuje zdarzenie kliknięcia myszą.
     * @param event Wskaźnik do obiektu zdarzenia myszy.
     */
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked(stationId);
        }
        QGraphicsEllipseItem::mousePressEvent(event);
    }

private:
    int stationId; ///< Identyfikator stacji powiązanej z elipsą.
};

#endif // CLICKABLEELLIPSEITEM_H
