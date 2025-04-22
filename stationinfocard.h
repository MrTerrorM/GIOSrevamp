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
#include <QComboBox>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>

class StationInfoCard : public QFrame {
    Q_OBJECT

public:
    explicit StationInfoCard(QWidget *parent = nullptr);
    void showStationData(int stationId, const QString &stationName, const QString &communeName, const QString &provinceName);
    void showDataFromFile(const QString &stationName, const QString &location, const QJsonArray &sensors);

signals:
    void cardClosed();

private slots:
    void onCloseButtonClicked();
    void onSensorsReplyFinished(QNetworkReply *reply);
    void onDataReplyFinished(QNetworkReply *reply, int column, const QString paramCode);
    void onSensorSelectionChanged(const QString paramCode);
    void onTimeRangeChanged(const QString timeRange);
    void onSaveButtonClicked();

private:
    QNetworkAccessManager *networkManager;
    QTableWidget *dataTable;
    QLabel *titleLabel;
    QLabel *stationNameLabel;
    QLabel *locationLabel;
    QLabel *minValueLabel;      // Nowa etykieta dla minimalnej wartości
    QLabel *maxValueLabel;      // Nowa etykieta dla maksymalnej wartości
    QLabel *averageValueLabel;  // Nowa etykieta dla średniej wartości
    QLabel *trendLabel;         // Nowa etykieta dla trendu
    QPushButton *closeButton;
    QPushButton *saveButton;
    QComboBox *sensorComboBox;
    QComboBox *timeRangeComboBox;
    QChart *chart;
    QChartView *chartView;
    QVBoxLayout *layout;
    QStringList sensorData;
    QMap<QString, QJsonArray> sensorValues;
    QMap<QString, QString> paramNames;
    int pendingRequests;
    QString currentTimeRange;

    void setupChart();
    void animateIn();
    void animateOut();
    void adjustTableWidth();
    void updateChart(const QString paramCode);
    void updateComboBoxPositions();
    void saveDataToJson(const QString &fileName);
};

#endif // STATIONINFOCARD_H
