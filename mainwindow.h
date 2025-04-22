#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include "custombutton.h"
#include "stationinfocard.h"
#include "clickableellipseitem.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onReplyFinished(QNetworkReply *reply);
    void onSearchTextChanged(const QString &text);
    void onTitleButtonClicked();
    void onSearchButtonClicked();
    void onGeocodingReplyFinished(QNetworkReply *reply);
    void onStationClicked(int stationId, const QString &stationName, const QString &communeName, const QString &provinceName);
    void onStationActivated(QListWidgetItem *item);
    void onLoadFileButtonClicked();
    void onEllipseClicked(int stationId);

private:
    void geocodeLocation(const QString &location);
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);

    QNetworkAccessManager *networkManager;
    QListWidget *stationListWidget;
    QGraphicsView *mapView;
    QGraphicsScene *mapScene;
    CustomButton *titleButton;
    QLabel *iconLabel;
    QLineEdit *searchEdit;
    QLineEdit *radiusEdit;
    QPushButton *searchButton;
    QComboBox *sortComboBox;
    QPushButton *loadFileButton;
    StationInfoCard *infoCard;
    QJsonArray allStations;
    int currentMode;
    bool geocodingDone;
    QString sortMode;
    double userLat, userLon;
};

#endif // MAINWINDOW_H
