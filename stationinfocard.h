#ifndef STATIONINFOCARD_H
#define STATIONINFOCARD_H

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>

class StationInfoCard : public QFrame {
    Q_OBJECT

public:
    StationInfoCard(QWidget *parent = nullptr);
    void showStationData(int stationId, const QString &stationName, const QString &communeName, const QString &provinceName);
    void animateIn();
    void animateOut();

signals:
    void cardClosed(); // Sygnał emitowany po zamknięciu karty

private slots:
    void onCloseButtonClicked();
    void onSensorsReplyFinished(QNetworkReply *reply);
    void onDataReplyFinished(QNetworkReply *reply, int column);

private:
    void adjustTableWidth(); // Nowa metoda do dopasowania szerokości tabeli

    QNetworkAccessManager *networkManager;
    QLabel *titleLabel;
    QLabel *stationNameLabel;
    QLabel *locationLabel;
    QTableWidget *dataTable;
    QPushButton *closeButton;
    QVBoxLayout *layout;
    int pendingRequests;
    QStringList sensorData;
    QMap<QString, QString> paramNames;
};

#endif // STATIONINFOCARD_H
