#include "stationinfocard.h"
#include <QPropertyAnimation>
#include <QDebug>
#include <QPalette>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QFontMetrics>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>

StationInfoCard::StationInfoCard(QWidget *parent)
    : QFrame(parent), networkManager(new QNetworkAccessManager(this)), pendingRequests(0), currentTimeRange("Dzień") {
    // Ustawienie karty jako pełnoekranowej względem rodzica
    setAutoFillBackground(true);
    setStyleSheet("StationInfoCard { background-color: #f0f0f0; border: 1px solid #ccc; border-radius: 5px; }");

    // Dodatkowe ustawienie tła przez QPalette
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor("#f0f0f0"));
    setPalette(palette);

    // Utworzenie przycisku X
    closeButton = new QPushButton("X", this);
    closeButton->setStyleSheet("QPushButton { text-align: center; padding: 5px; font-size: 24px; font-weight: bold; background-color: #ff5555; color: white; border: none; border-radius: 3px; width: 30px; height: 30px; } QPushButton:hover { background-color: #cc4444; }");
    closeButton->setFixedSize(30, 30);
    connect(closeButton, &QPushButton::clicked, this, &StationInfoCard::onCloseButtonClicked);

    // Utworzenie przycisku zapisu
    saveButton = new QPushButton("Zapisz", this);
    saveButton->setStyleSheet("QPushButton { text-align: center; padding: 5px; font-size: 16px; font-weight: bold; background-color: #55aa55; color: white; border: none; border-radius: 3px; width: 60px; height: 30px; } QPushButton:hover { background-color: #448844; }");
    saveButton->setFixedSize(60, 30);
    connect(saveButton, &QPushButton::clicked, this, &StationInfoCard::onSaveButtonClicked);

    // Utworzenie etykiet dla tekstu
    titleLabel = new QLabel("Dane Stacji", this);
    titleLabel->setStyleSheet("font-size: 29px; font-weight: bold; color: darkblue; margin-left: 10px; margin-top: 5px;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    stationNameLabel = new QLabel(this);
    stationNameLabel->setStyleSheet("font-size: 20px; color: black; background-color: #d3d3d3; margin-left: 10px; margin-right: 10px; padding: 5px;");
    stationNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    stationNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    locationLabel = new QLabel(this);
    locationLabel->setStyleSheet("font-size: 16px; color: black; margin-right: 10px;");
    locationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Nowe etykiety dla wartości minimalnej, maksymalnej, średniej i trendu
    minValueLabel = new QLabel("Najmniejsza wartość: Brak danych", this);
    minValueLabel->setStyleSheet("font-size: 14px; color: black; margin: 10px;");
    minValueLabel->setAlignment(Qt::AlignCenter);

    maxValueLabel = new QLabel("Największa wartość: Brak danych", this);
    maxValueLabel->setStyleSheet("font-size: 14px; color: black; margin: 10px;");
    maxValueLabel->setAlignment(Qt::AlignCenter);

    averageValueLabel = new QLabel("Średnia wartość: Brak danych", this);
    averageValueLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: black; margin: 10px;");
    averageValueLabel->setAlignment(Qt::AlignCenter);

    trendLabel = new QLabel("Trend: Brak danych", this);
    trendLabel->setStyleSheet("font-size: 14px; color: black; margin: 10px;");
    trendLabel->setAlignment(Qt::AlignCenter);

    // Utworzenie tabeli dla danych sensorów
    dataTable = new QTableWidget(this);
    dataTable->setStyleSheet("QTableWidget { font-size: 14px; border: none; background-color: black; color: white; margin: 0px; padding: 0px; } QTableWidget::item { padding: 5px; color: white; }");
    dataTable->setRowCount(2); // Wiersze: kody parametrów i wartości
    dataTable->setColumnCount(0); // Kolumny będą dodawane dynamicznie
    dataTable->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    dataTable->setShowGrid(false);
    dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    dataTable->horizontalHeader()->setVisible(false);
    dataTable->verticalHeader()->setVisible(false);
    dataTable->setFixedHeight(60); // Wysokość dla dwóch wierszy

    // Inicjalizacja mapy nazw parametrów
    paramNames = {
        {"PM10", "Pył zawieszony PM10"},
        {"PM2.5", "Pył zawieszony PM2.5"},
        {"NO2", "Dwutlenek azotu"},
        {"SO2", "Dwutlenek siarki"},
        {"CO", "Tlenek węgla"},
        {"O3", "Ozon"},
        {"C6H6", "Benzen"}
    };

    // Utworzenie listy rozwijanej dla sensorów
    sensorComboBox = new QComboBox(this);
    sensorComboBox->setStyleSheet(
        "QComboBox { font-size: 14px; padding: 3px 5px; background-color: #333333; color: white; border: 1px solid #555; min-width: 60px; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox::down-arrow { image: none; } QComboBox::down-arrow:on { image: none; }"
        "QComboBox QAbstractItemView { background-color: #333333; color: white; selection-background-color: #555555; selection-color: white; }"
        );
    sensorComboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Dynamiczne ustawienie szerokości na podstawie najdłuższego kodu parametru (PM2.5)
    QFontMetrics fm(sensorComboBox->font());
    sensorComboBox->setFixedWidth(fm.boundingRect("PM2.5").width() + 30);
    connect(sensorComboBox, &QComboBox::currentTextChanged, this, &StationInfoCard::onSensorSelectionChanged);

    // Utworzenie listy rozwijanej dla zakresu czasu
    timeRangeComboBox = new QComboBox(this);
    timeRangeComboBox->setStyleSheet(
        "QComboBox { font-size: 14px; padding: 3px 5px; background-color: #333333; color: white; border: 1px solid #555; min-width: 80px; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox::down-arrow { image: none; } QComboBox::down-arrow:on { image: none; }"
        "QComboBox QAbstractItemView { background-color: #333333; color: white; selection-background-color: #555555; selection-color: white; }"
        );
    timeRangeComboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    timeRangeComboBox->addItems({QStringLiteral("Dzień"), QStringLiteral("Tydzień"), QStringLiteral("Miesiąc"), QStringLiteral("Pół roku"), QStringLiteral("Rok")});
    timeRangeComboBox->setFixedWidth(fm.boundingRect("Pół roku").width() + 30);
    connect(timeRangeComboBox, &QComboBox::currentTextChanged, this, &StationInfoCard::onTimeRangeChanged);

    // Utworzenie wykresu
    chart = new QChart();
    chartView = new QChartView(chart, this);
    chartView->setStyleSheet("background-color: black;");
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setFixedHeight(350);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setupChart();

    // Ustawienie chartView jako rodzica dla list rozwijanych
    sensorComboBox->setParent(chartView);
    timeRangeComboBox->setParent(chartView);

    // Ręczne pozycjonowanie sensorComboBox
    sensorComboBox->move(10, 10);

    // Pozycja timeRangeComboBox będzie ustawiona w updateComboBoxPositions
    updateComboBoxPositions();

    // Ustawienie layoutu
    layout = new QVBoxLayout(this);
    QHBoxLayout *topLayout = new QHBoxLayout();
    QVBoxLayout *leftTextLayout = new QVBoxLayout();
    leftTextLayout->addWidget(titleLabel);
    topLayout->addLayout(leftTextLayout);
    topLayout->addStretch();
    topLayout->addWidget(saveButton, 0, Qt::AlignTop);
    topLayout->addWidget(closeButton, 0, Qt::AlignTop);
    layout->addLayout(topLayout);

    QHBoxLayout *stationNameLayout = new QHBoxLayout();
    stationNameLayout->addWidget(stationNameLabel);
    layout->addLayout(stationNameLayout);

    layout->addWidget(locationLabel);
    layout->addSpacerItem(new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Fixed));

    // Wyśrodkowanie tabeli
    QHBoxLayout *tableLayout = new QHBoxLayout();
    tableLayout->addStretch();
    tableLayout->addWidget(dataTable);
    tableLayout->addStretch();
    tableLayout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(tableLayout);

    // Dodanie chartView do głównego layoutu
    layout->addWidget(chartView);
    // Dodanie nowych etykiet pod wykresem
    layout->addWidget(minValueLabel);
    layout->addWidget(maxValueLabel);
    layout->addWidget(averageValueLabel);
    layout->addWidget(trendLabel);
    layout->addStretch();
    setLayout(layout);

    // Początkowo karta jest niewidoczna
    setVisible(false);
    qDebug() << "StationInfoCard utworzona";
}

void StationInfoCard::setupChart() {
    chart->setTitleBrush(Qt::white);
    chart->setBackgroundBrush(Qt::black);
    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(16);
    chart->setTitleFont(titleFont);
    chart->legend()->hide();
    chart->setMargins(QMargins(10, 10, 10, 10));

    // Utwórz osie
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("dd.MM.yyyy HH:mm");
    axisX->setTitleBrush(Qt::white);
    axisX->setLabelsColor(Qt::white);
    axisX->setLinePen(QPen(Qt::white));
    axisX->setGridLinePen(QPen(Qt::white, 1, Qt::DashLine));
    axisX->setTickCount(6);
    QFont axisFont = axisX->labelsFont();
    axisFont.setPointSize(8);
    axisX->setLabelsFont(axisFont);
    axisX->setLabelsAngle(0);
    axisX->setLabelsVisible(true);
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelsColor(Qt::white);
    axisY->setLinePen(QPen(Qt::white));
    axisY->setGridLinePen(QPen(Qt::white, 1, Qt::DashLine));
    chart->addAxis(axisY, Qt::AlignLeft);
}

void StationInfoCard::showStationData(int stationId, const QString &stationName, const QString &communeName, const QString &provinceName) {
    qDebug() << "Pokazywanie danych dla stacji ID:" << stationId << "Nazwa:" << stationName << "Gmina:" << communeName << "Województwo:" << provinceName;

    // Ustaw nazwę stacji i lokalizację
    stationNameLabel->setText(stationName);
    locationLabel->setText(QString("%1, %2").arg(communeName, provinceName));

    // Wyczyść poprzednie dane
    sensorData.clear();
    sensorValues.clear();
    sensorComboBox->clear();
    pendingRequests = 0;
    dataTable->setColumnCount(0);
    chart->removeAllSeries();
    // Wyczyść nowe etykiety
    minValueLabel->setText("Najmniejsza wartość: Brak danych");
    maxValueLabel->setText("Największa wartość: Brak danych");
    averageValueLabel->setText("Średnia wartość: Brak danych");
    trendLabel->setText("Trend: Brak danych");
    // Ustaw domyślny zakres osi X
    if (!chart->axes(Qt::Horizontal).isEmpty()) {
        QDateTimeAxis *axisX = qobject_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal).first());
        QDateTime now = QDateTime::currentDateTime();
        axisX->setRange(now.addDays(-1), now);
    }

    // Pobierz listę sensorów dla stacji
    QUrl url(QString("https://api.gios.gov.pl/pjp-api/rest/station/sensors/%1").arg(stationId));
    qDebug() << "Wysyłanie zapytania do:" << url.toString();
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "MyStationFinderApp/1.0");
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSensorsReplyFinished(reply);
    });

    // Dopasuj rozmiar do rodzica i pokaż kartę
    if (parentWidget()) {
        resize(parentWidget()->size());
    }
    setVisible(true);
    raise();
    move(-width(), 0);
    animateIn();
}

void StationInfoCard::showDataFromFile(const QString &stationName, const QString &location, const QJsonArray &sensors) {
    qDebug() << "Pokazywanie danych z pliku dla stacji:" << stationName << "Lokalizacja:" << location;

    // Ustaw nazwę stacji i lokalizację
    stationNameLabel->setText(stationName);
    locationLabel->setText(location);

    // Wyczyść poprzednie dane
    sensorData.clear();
    sensorValues.clear();
    sensorComboBox->clear();
    dataTable->setColumnCount(0);
    chart->removeAllSeries();
    // Wyczyść nowe etykiety
    minValueLabel->setText("Najmniejsza wartość: Brak danych");
    maxValueLabel->setText("Największa wartość: Brak danych");
    averageValueLabel->setText("Średnia wartość: Brak danych");
    trendLabel->setText("Trend: Brak danych");

    // Ustaw domyślny zakres osi X
    if (!chart->axes(Qt::Horizontal).isEmpty()) {
        QDateTimeAxis *axisX = qobject_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal).first());
        QDateTime now = QDateTime::currentDateTime();
        axisX->setRange(now.addDays(-1), now);
    }

    // Przetwarzaj sensory z pliku
    if (sensors.isEmpty()) {
        sensorData.append("Brak danych sensorów w pliku.");
        dataTable->setRowCount(1);
        dataTable->setColumnCount(1);
        QTableWidgetItem *item = new QTableWidgetItem("Brak Danych");
        item->setForeground(Qt::white);
        dataTable->setItem(0, 0, item);
        dataTable->resizeColumnsToContents();
        adjustTableWidth();
    } else {
        dataTable->setColumnCount(sensors.size());
        int column = 0;

        for (const QJsonValue &sensorValue : sensors) {
            QJsonObject sensor = sensorValue.toObject();
            QString paramCode = sensor["paramCode"].toString();
            QString latestValue = sensor["latestValue"].toString();
            QJsonArray historicalData = sensor["historicalData"].toArray();

            // Dodaj kod parametru do pierwszego wiersza
            QTableWidgetItem *paramItem = new QTableWidgetItem(paramCode);
            paramItem->setTextAlignment(Qt::AlignCenter);
            paramItem->setForeground(Qt::white);
            if (paramNames.contains(paramCode)) {
                paramItem->setToolTip(paramNames[paramCode]);
            } else {
                paramItem->setToolTip(paramCode);
            }
            dataTable->setItem(0, column, paramItem);

            // Dodaj wartość do drugiego wiersza
            QTableWidgetItem *valueItem = new QTableWidgetItem(latestValue);
            valueItem->setTextAlignment(Qt::AlignCenter);
            valueItem->setForeground(Qt::white);
            dataTable->setItem(1, column, valueItem);

            // Dodaj paramCode do listy rozwijanej
            sensorComboBox->addItem(paramCode);

            // Zapisz dane historyczne
            sensorValues[paramCode] = historicalData;
            sensorData.append(latestValue);

            column++;
        }

        // Ustaw pierwszy sensor jako domyślny
        if (sensorComboBox->count() > 0) {
            sensorComboBox->setCurrentIndex(0);
        }

        dataTable->resizeColumnsToContents();
        adjustTableWidth();

        // Zaktualizuj wykres dla pierwszego sensora
        if (sensorComboBox->count() > 0) {
            updateChart(sensorComboBox->currentText());
        }
    }

    // Dopasuj rozmiar do rodzica i pokaż kartę
    if (parentWidget()) {
        resize(parentWidget()->size());
    }
    setVisible(true);
    raise();
    move(-width(), 0);
    animateIn();
}

void StationInfoCard::onSensorsReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray rawData = reply->readAll();
        qDebug() << "Odpowiedź API GIOŚ (sensory):" << rawData;

        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        if (doc.isNull()) {
            qDebug() << "Błąd: Nie udało się sparsować JSON z sensorów.";
            sensorData.append("Błąd: Nieprawidłowy format danych sensorów.");
            dataTable->setRowCount(1);
            dataTable->setColumnCount(1);
            QTableWidgetItem *item = new QTableWidgetItem("Obecnie Brak Danych");
            item->setForeground(Qt::white);
            dataTable->setItem(0, 0, item);
            dataTable->resizeColumnsToContents();
            adjustTableWidth();
        } else {
            QJsonArray sensors = doc.array();
            pendingRequests = sensors.size();
            qDebug() << "Znaleziono" << sensors.size() << "sensorów";

            if (sensors.isEmpty()) {
                sensorData.append("Brak danych sensorów dla tej stacji.");
                dataTable->setRowCount(1);
                dataTable->setColumnCount(1);
                QTableWidgetItem *item = new QTableWidgetItem("Obecnie Brak Danych");
                item->setForeground(Qt::white);
                dataTable->setItem(0, 0, item);
                dataTable->resizeColumnsToContents();
                adjustTableWidth();
            } else {
                dataTable->setColumnCount(sensors.size());
                int column = 0;

                for (const QJsonValue &sensorValue : sensors) {
                    QJsonObject sensor = sensorValue.toObject();
                    int sensorId = sensor["id"].toInt();
                    QString paramCode = sensor["param"].toObject()["paramCode"].toString();

                    // Dodaj kod parametru do pierwszego wiersza
                    QTableWidgetItem *paramItem = new QTableWidgetItem(paramCode);
                    paramItem->setTextAlignment(Qt::AlignCenter);
                    paramItem->setForeground(Qt::white);
                    if (paramNames.contains(paramCode)) {
                        paramItem->setToolTip(paramNames[paramCode]);
                    } else {
                        paramItem->setToolTip(paramCode);
                    }
                    dataTable->setItem(0, column, paramItem);

                    // Dodaj paramCode do listy rozwijanej
                    sensorComboBox->addItem(paramCode, paramCode);

                    // Pobierz dane dla sensora
                    QUrl dataUrl(QString("https://api.gios.gov.pl/pjp-api/rest/data/getData/%1").arg(sensorId));
                    qDebug() << "Wysyłanie zapytania o dane do:" << dataUrl.toString();
                    QNetworkRequest dataRequest;
                    dataRequest.setUrl(dataUrl);
                    dataRequest.setHeader(QNetworkRequest::UserAgentHeader, "MyStationFinderApp/1.0");
                    QNetworkReply *dataReply = networkManager->get(dataRequest);
                    connect(dataReply, &QNetworkReply::finished, this, [this, column, paramCode]() {
                        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
                        if (reply) {
                            onDataReplyFinished(reply, column, paramCode);
                        }
                    });

                    column++;
                }
                dataTable->resizeColumnsToContents();
                adjustTableWidth();
            }
        }
    } else {
        qDebug() << "Błąd pobierania sensorów:" << reply->errorString();
        sensorData.append("Błąd: Nie udało się pobrać danych sensorów.");
        dataTable->setRowCount(1);
        dataTable->setColumnCount(1);
        QTableWidgetItem *item = new QTableWidgetItem("Obecnie Brak Danych");
        item->setForeground(Qt::white);
        dataTable->setItem(0, 0, item);
        dataTable->resizeColumnsToContents();
        adjustTableWidth();
    }
    reply->deleteLater();
}

void StationInfoCard::onDataReplyFinished(QNetworkReply *reply, int column, const QString paramCode) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray rawData = reply->readAll();
        qDebug() << "Odpowiedź API GIOŚ (dane sensora, paramCode:" << paramCode << "):" << rawData;

        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        if (!doc.isNull()) {
            QJsonObject data = doc.object();
            QJsonArray values = data["values"].toArray();
            QString valueText = "Brak danych";

            if (!values.isEmpty()) {
                QJsonObject latestValue = values[0].toObject();
                if (latestValue.contains("value") && !latestValue["value"].isNull()) {
                    double value = latestValue["value"].toDouble();
                    valueText = QString::number(value, 'f', 1);
                }
            }

            QTableWidgetItem *valueItem = new QTableWidgetItem(valueText);
            valueItem->setTextAlignment(Qt::AlignCenter);
            valueItem->setForeground(Qt::white);
            dataTable->setItem(1, column, valueItem);
            sensorData.append(valueText);

            // Zapisz dane sensora
            sensorValues[paramCode] = values;

            // Jeśli to pierwszy sensor, zaktualizuj wykres
            if (sensorComboBox->count() > 0 && sensorComboBox->currentIndex() == column) {
                updateChart(paramCode);
            }
        } else {
            qDebug() << "Błąd: Nie udało się sparsować JSON z danych sensora.";
            QTableWidgetItem *valueItem = new QTableWidgetItem("Błąd");
            valueItem->setTextAlignment(Qt::AlignCenter);
            valueItem->setForeground(Qt::white);
            dataTable->setItem(1, column, valueItem);
            sensorData.append("Błąd");
        }
    } else {
        qDebug() << "Błąd pobierania danych sensora:" << reply->errorString();
        QTableWidgetItem *valueItem = new QTableWidgetItem("Błąd");
        valueItem->setTextAlignment(Qt::AlignCenter);
        valueItem->setForeground(Qt::white);
        dataTable->setItem(1, column, valueItem);
        sensorData.append("Błąd");
    }

    reply->deleteLater();
    pendingRequests--;

    if (pendingRequests == 0) {
        qDebug() << "Wszystkie dane sensorów pobrane:" << sensorData;
        qDebug() << "Stan tabeli: rows=" << dataTable->rowCount() << ", cols=" << dataTable->columnCount();

        dataTable->resizeColumnsToContents();
        adjustTableWidth();

        if (sensorComboBox->count() > 0) {
            updateChart(sensorComboBox->currentData().toString());
        }
    }
}

void StationInfoCard::onSensorSelectionChanged(const QString paramCode) {
    qDebug() << "Wybrano sensor:" << paramCode;
    updateChart(paramCode);
}

void StationInfoCard::onTimeRangeChanged(const QString timeRange) {
    qDebug() << "Wybrano zakres czasu:" << timeRange;
    currentTimeRange = timeRange;
    if (sensorComboBox->count() > 0 && !sensorComboBox->currentText().isEmpty()) {
        updateChart(sensorComboBox->currentText());
    }
}

void StationInfoCard::updateChart(const QString paramCode) {
    qDebug() << "Aktualizowanie wykresu dla paramCode:" << paramCode;

    // Wyczyść poprzednie serie i osie
    chart->removeAllSeries();
    QList<QAbstractAxis*> axesX = chart->axes(Qt::Horizontal);
    QList<QAbstractAxis*> axesY = chart->axes(Qt::Vertical);
    for (QAbstractAxis* axis : axesX) {
        chart->removeAxis(axis);
        delete axis;
    }
    for (QAbstractAxis* axis : axesY) {
        chart->removeAxis(axis);
        delete axis;
    }

    // Wyczyść etykiety
    minValueLabel->setText("Najmniejsza wartość: Brak danych");
    maxValueLabel->setText("Największa wartość: Brak danych");
    averageValueLabel->setText("Średnia wartość: Brak danych");
    trendLabel->setText("Trend: Brak danych");

    if (!sensorValues.contains(paramCode)) {
        qDebug() << "Brak danych dla paramCode:" << paramCode;
        chart->setTitle(paramNames.value(paramCode, paramCode));
        setupChart();
        return;
    }

    QLineSeries *series = new QLineSeries();
    series->setPen(QPen(QColor(135, 206, 250), 2));

    QJsonArray values = sensorValues[paramCode];
    double lastValue = 0.0;
    bool hasData = false;
    QDateTime minDateTime = QDateTime::currentDateTime();
    QDateTime maxDateTime = minDateTime;

    // Zmienne do obliczania min, max, średniej i trendu
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();
    QDateTime minValueDateTime, maxValueDateTime;
    double sumValues = 0.0;
    int validValueCount = 0;
    QVector<QPointF> regressionPoints;

    // Określ maksymalną liczbę punktów w zależności od zakresu czasu
    int maxPoints = 24;
    if (currentTimeRange == "Tydzień") {
        maxPoints = 24 * 7;
    } else if (currentTimeRange == "Miesiąc") {
        maxPoints = 24 * 30;
    } else if (currentTimeRange == "Pół roku") {
        maxPoints = 24 * 180;
    } else if (currentTimeRange == "Rok") {
        maxPoints = 24 * 365;
    }

    // Określ zakres czasu
    QDateTime now = QDateTime::currentDateTime();
    if (currentTimeRange == "Dzień") {
        minDateTime = now.addDays(-1);
    } else if (currentTimeRange == "Tydzień") {
        minDateTime = now.addDays(-7);
    } else if (currentTimeRange == "Miesiąc") {
        minDateTime = now.addDays(-30);
    } else if (currentTimeRange == "Pół roku") {
        minDateTime = now.addDays(-180);
    } else if (currentTimeRange == "Rok") {
        minDateTime = now.addDays(-365);
    }
    maxDateTime = now;

    // Iteruj od najnowszych do starszych
    int pointsAdded = 0;
    for (int i = 0; i < values.size() && pointsAdded < maxPoints; ++i) {
        QJsonObject valueObj = values[i].toObject();
        if (!valueObj.contains("date") || !valueObj.contains("value")) {
            qDebug() << "Brak pola 'date' lub 'value' w obiekcie:" << valueObj;
            continue;
        }

        QString dateStr = valueObj["date"].toString();
        qDebug() << "Przetwarzanie daty:" << dateStr;
        QDateTime dateTime = QDateTime::fromString(dateStr, "yyyy-MM-dd HH:mm:ss");
        if (!dateTime.isValid()) {
            qDebug() << "Nieprawidłowy format daty:" << dateStr;
            continue;
        }

        if (dateTime < minDateTime || dateTime > maxDateTime) {
            continue;
        }

        double value = valueObj["value"].isNull() ? lastValue : valueObj["value"].toDouble();
        if (value < 0) {
            qDebug() << "Ujemna wartość (" << value << ") zastąpiona przez 0 dla daty:" << dateStr;
            value = 0;
        }
        if (!valueObj["value"].isNull()) {
            lastValue = value;
            hasData = true;

            // Oblicz min, max, sumę do średniej
            if (value < minValue) {
                minValue = value;
                minValueDateTime = dateTime;
            }
            if (value > maxValue) {
                maxValue = value;
                maxValueDateTime = dateTime;
            }
            sumValues += value;
            validValueCount++;

            // Dodaj punkt do regresji (czas w milisekundach jako x, wartość jako y)
            regressionPoints.append(QPointF(dateTime.toMSecsSinceEpoch(), value));
        } else if (lastValue < 0) {
            lastValue = 0;
        }

        series->append(dateTime.toMSecsSinceEpoch(), value);
        qDebug() << "Dodano punkt: czas=" << dateTime.toString("dd.MM.yyyy HH:mm") << ", wartość=" << value;
        pointsAdded++;

        if (dateTime < minDateTime) minDateTime = dateTime;
        if (dateTime > maxDateTime) maxDateTime = dateTime;
    }

    // Ustaw tytuł wykresu
    chart->setTitle(paramNames.value(paramCode, paramCode));

    // Aktualizuj etykiety, jeśli są dane
    if (hasData && validValueCount > 0) {
        // Minimalna wartość z pogrubioną wartością
        minValueLabel->setText(QString("Najmniejsza wartość: <b>%1</b> w dniu %2")
                                   .arg(QString::number(minValue, 'f', 1))
                                   .arg(minValueDateTime.toString("dd.MM.yyyy HH:mm")));

        // Maksymalna wartość z pogrubioną wartością
        maxValueLabel->setText(QString("Największa wartość: <b>%1</b> w dniu %2")
                                   .arg(QString::number(maxValue, 'f', 1))
                                   .arg(maxValueDateTime.toString("dd.MM.yyyy HH:mm")));

        // Średnia wartość
        double averageValue = sumValues / validValueCount;
        averageValueLabel->setText(QString("Średnia wartość: %1")
                                       .arg(QString::number(averageValue, 'f', 1)));

        // Oblicz trend (regresja liniowa)
        double slope = 0.0;
        if (regressionPoints.size() >= 2) {
            double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumXX = 0.0;
            int n = regressionPoints.size();
            for (const QPointF &point : regressionPoints) {
                sumX += point.x();
                sumY += point.y();
                sumXY += point.x() * point.y();
                sumXX += point.x() * point.x();
            }
            slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
            qDebug() << "Nachylenie regresji (slope):" << slope;
        }

        // Ustaw trend
        if (slope > 0) {
            trendLabel->setText("Trend: Tendencja Wzrostu");
        } else if (slope < 0) {
            trendLabel->setText("Trend: Tendencja Spadku");
        } else {
            trendLabel->setText("Trend: Stabilny");
        }
    }

    if (hasData && !series->points().isEmpty()) {
        chart->addSeries(series);

        // Dodaj osie ponownie
        QDateTimeAxis *axisX = new QDateTimeAxis();
        int tickCount = 6;
        QString dateFormat = "dd.MM.yyyy HH:mm";
        if (currentTimeRange == "Dzień") {
            tickCount = 6;
            dateFormat = "dd.MM.yyyy HH:mm";
        } else if (currentTimeRange == "Tydzień") {
            tickCount = 7;
            dateFormat = "dd.MM.yyyy";
        } else if (currentTimeRange == "Miesiąc") {
            tickCount = 5;
            dateFormat = "dd.MM.yyyy";
        } else if (currentTimeRange == "Pół roku") {
            tickCount = 6;
            dateFormat = "dd.MM.yyyy";
        } else if (currentTimeRange == "Rok") {
            tickCount = 6;
            dateFormat = "MM.yyyy";
        }
        axisX->setFormat(dateFormat);
        axisX->setTitleBrush(Qt::white);
        axisX->setLabelsColor(Qt::white);
        axisX->setLinePen(QPen(Qt::white));
        axisX->setGridLinePen(QPen(Qt::white, 1, Qt::DashLine));
        axisX->setTickCount(tickCount);
        QFont axisFont = axisX->labelsFont();
        axisFont.setPointSize(8);
        axisX->setLabelsFont(axisFont);
        axisX->setLabelsAngle(0);
        axisX->setLabelsVisible(true);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis();
        axisY->setLabelsColor(Qt::white);
        axisY->setLinePen(QPen(Qt::white));
        axisY->setGridLinePen(QPen(Qt::white, 1, Qt::DashLine));
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);

        // Ustaw zakres osi Y dynamicznie, zapewniając minimum 0
        qreal minY = series->points().first().y();
        qreal maxY = minY;
        for (const QPointF &point : series->points()) {
            minY = qMin(minY, point.y());
            maxY = qMax(maxY, point.y());
        }
        minY = qMax(minY, qreal(0.0)); // Zapewniamy, że minimum to 0
        qreal margin = (maxY - minY) * 0.1;
        if (margin == 0) margin = 1.0;
        axisY->setRange(minY, maxY + margin);
        qDebug() << "Zakres osi Y: min=" << minY << ", max=" << (maxY + margin);

        // Ustaw zakres osi X
        axisX->setRange(minDateTime, maxDateTime);
        axisX->setLabelsVisible(true);
        axisX->setTickCount(tickCount);
        qDebug() << "Zakres osi X: od=" << minDateTime.toString("dd.MM.yyyy HH:mm") << ", do=" << maxDateTime.toString("dd.MM.yyyy HH:mm");

        chart->update();
        chartView->repaint();
    } else {
        qDebug() << "Brak danych do wyświetlenia na wykresie dla:" << paramCode;
        delete series;
        setupChart();
    }
}

void StationInfoCard::animateIn() {
    qDebug() << "Rozpoczynanie animacji wjazdu";
    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(500);
    animation->setStartValue(QPoint(-width(), 0));
    animation->setEndValue(QPoint(0, 0));
    animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation, &QPropertyAnimation::finished, this, []() {
        qDebug() << "Animacja wjazdu zakończona";
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void StationInfoCard::animateOut() {
    qDebug() << "Rozpoczynanie animacji wyjazdu";
    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(500);
    animation->setStartValue(QPoint(0, 0));
    animation->setEndValue(QPoint(-width(), 0));
    animation->setEasingCurve(QEasingCurve::InCubic);
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
        setVisible(false);
        emit cardClosed();
        qDebug() << "Animacja wyjazdu zakończona, karta ukryta";
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void StationInfoCard::onCloseButtonClicked() {
    qDebug() << "Przycisk Zamknij kliknięty";
    animateOut();
}

void StationInfoCard::adjustTableWidth() {
    int totalWidth = 0;
    for (int col = 0; col < dataTable->columnCount(); ++col) {
        totalWidth += dataTable->columnWidth(col);
    }
    totalWidth += 2;
    dataTable->setMaximumWidth(totalWidth);
    dataTable->setMinimumWidth(totalWidth);
}

void StationInfoCard::updateComboBoxPositions() {
    // Ustaw pozycję timeRangeComboBox po prawej stronie chartView, z odstępem od sensorComboBox
    int margin = 10;
    int xPos = chartView->width() - timeRangeComboBox->width() - margin;
    // Zapewniamy, że timeRangeComboBox nie nakłada się na sensorComboBox
    int minXPos = sensorComboBox->x() + sensorComboBox->width() + margin;
    xPos = qMax(xPos, minXPos);
    timeRangeComboBox->move(xPos, margin);
    qDebug() << "Zaktualizowano pozycje: sensorComboBox=" << sensorComboBox->pos()
             << ", timeRangeComboBox=" << timeRangeComboBox->pos()
             << ", chartView width=" << chartView->width();
}

void StationInfoCard::onSaveButtonClicked() {
    qDebug() << "Przycisk Zapisz kliknięty";

    // Przygotuj dane do zapisu
    QJsonObject stationObject;
    stationObject["stationName"] = stationNameLabel->text();
    stationObject["location"] = locationLabel->text();

    QJsonArray sensorsArray;
    for (int col = 0; col < dataTable->columnCount(); ++col) {
        QTableWidgetItem *paramItem = dataTable->item(0, col);
        QTableWidgetItem *valueItem = dataTable->item(1, col);
        if (!paramItem || !valueItem) continue;

        QString paramCode = paramItem->text();
        QString latestValue = valueItem->text();

        QJsonObject sensorObject;
        sensorObject["paramCode"] = paramCode;
        sensorObject["latestValue"] = latestValue;

        // Pobierz istniejące dane historyczne
        QJsonArray historicalData;
        if (sensorValues.contains(paramCode)) {
            historicalData = sensorValues[paramCode];
        }

        // Dodaj nowy punkt danych na początek z pełną godziną
        QDateTime currentTime = QDateTime::currentDateTime();
        // Zaokrąglij do pełnej godziny (minuty i sekundy na 0)
        currentTime.setTime(QTime(currentTime.time().hour(), 0, 0));
        QJsonObject newDataPoint;
        newDataPoint["date"] = currentTime.toString("yyyy-MM-dd HH:mm:ss");
        newDataPoint["value"] = latestValue.toDouble();
        historicalData.prepend(newDataPoint);

        sensorObject["historicalData"] = historicalData;
        sensorsArray.append(sensorObject);

        // Zaktualizuj sensorValues
        sensorValues[paramCode] = historicalData;
    }

    stationObject["sensors"] = sensorsArray;

    // Przygotuj nazwę pliku na podstawie nazwy stacji
    QString stationName = stationNameLabel->text();
    // Usuń nadmiarowe spacje
    stationName = stationName.simplified();
    // Usuń znaki specjalne, w tym przecinki i kropki
    stationName.remove(QRegularExpression("[<>:\"/\\\\|?*,.]"));
    QString filePath = QCoreApplication::applicationDirPath() + "/data/" + stationName + ".json";

    // Utwórz katalog data, jeśli nie istnieje
    QDir dir(QCoreApplication::applicationDirPath() + "/data");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Zapisz dane do pliku
    saveDataToJson(filePath);

    // Zaktualizuj wykres po zapisaniu nowych danych
    if (sensorComboBox->count() > 0 && !sensorComboBox->currentText().isEmpty()) {
        updateChart(sensorComboBox->currentText());
    }

    QMessageBox::information(this, tr("Sukces"), tr("Dane zostały zapisane do %1").arg(filePath));
}

void StationInfoCard::saveDataToJson(const QString &fileName) {
    // Przygotowanie danych stacji
    QString stationName = stationNameLabel->text();
    QString location = locationLabel->text();

    // Tworzenie obiektu stacji
    QJsonObject stationObj;
    stationObj["stationName"] = stationName;
    stationObj["location"] = location;

    // Dane sensorów
    QJsonArray sensorsArray;
    for (int col = 0; col < dataTable->columnCount(); ++col) {
        QJsonObject sensorObj;
        QString paramCode = dataTable->item(0, col)->text();
        sensorObj["paramCode"] = paramCode;
        sensorObj["paramName"] = paramNames.value(paramCode, paramCode);
        sensorObj["latestValue"] = dataTable->item(1, col)->text();

        // Dane historyczne
        QJsonArray historicalData;
        if (sensorValues.contains(paramCode)) {
            QJsonArray existingData = sensorValues[paramCode];
            QSet<QString> existingDates;
            for (const QJsonValue &val : existingData) {
                existingDates.insert(val.toObject()["date"].toString());
            }

            // Dodaj nowy punkt danych tylko, jeśli data jest unikalna
            QDateTime currentTime = QDateTime::currentDateTime();
            // Zaokrąglij do pełnej godziny (minuty i sekundy na 0)
            currentTime.setTime(QTime(currentTime.time().hour(), 0, 0));
            QString newDate = currentTime.toString("yyyy-MM-dd HH:mm:ss");
            if (!existingDates.contains(newDate)) {
                QJsonObject newDataPoint;
                newDataPoint["date"] = newDate;
                newDataPoint["value"] = dataTable->item(1, col)->text().toDouble();
                existingData.prepend(newDataPoint); // Dodaj na początek
            }
            historicalData = existingData;
        } else {
            QDateTime currentTime = QDateTime::currentDateTime();
            // Zaokrąglij do pełnej godziny (minuty i sekundy na 0)
            currentTime.setTime(QTime(currentTime.time().hour(), 0, 0));
            QJsonObject newDataPoint;
            newDataPoint["date"] = currentTime.toString("yyyy-MM-dd HH:mm:ss");
            newDataPoint["value"] = dataTable->item(1, col)->text().toDouble();
            historicalData.prepend(newDataPoint); // Dodaj na początek
        }
        sensorObj["historicalData"] = historicalData;

        sensorsArray.append(sensorObj);
    }
    stationObj["sensors"] = sensorsArray;

    // Odczyt istniejącego pliku, jeśli istnieje
    QJsonObject rootObj;
    QJsonArray stationsArray;
    QFile file(fileName);
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Nie można otworzyć pliku do odczytu:" << fileName;
            QMessageBox::warning(this, tr("Błąd"), tr("Nie można otworzyć pliku do odczytu: %1").arg(file.errorString()));
            return;
        }

        QByteArray existingData = file.readAll();
        file.close();

        QJsonDocument existingDoc = QJsonDocument::fromJson(existingData);
        if (!existingDoc.isNull() && existingDoc.isObject()) {
            rootObj = existingDoc.object();
            if (rootObj.contains("stations") && rootObj["stations"].isArray()) {
                stationsArray = rootObj["stations"].toArray();
            }
        } else {
            qDebug() << "Nieprawidłowy format istniejącego pliku JSON. Tworzenie nowego.";
        }
    }

    // Szukaj istniejącej stacji
    int stationIndex = -1;
    for (int i = 0; i < stationsArray.size(); ++i) {
        QJsonObject existingStation = stationsArray[i].toObject();
        if (existingStation["stationName"].toString() == stationName) {
            stationIndex = i;
            break;
        }
    }

    // Aktualizacja lub dodanie stacji
    if (stationIndex >= 0) {
        // Stacja istnieje - aktualizuj sensory
        QJsonObject existingStation = stationsArray[stationIndex].toObject();
        QJsonArray existingSensors = existingStation["sensors"].toArray();

        // Przetwarzaj każdy nowy sensor
        QJsonArray updatedSensors;
        for (const QJsonValue &newSensorValue : sensorsArray) {
            QJsonObject newSensor = newSensorValue.toObject();
            QString paramCode = newSensor["paramCode"].toString();
            QJsonObject updatedSensor = newSensor;
            bool sensorExists = false;

            // Szukaj istniejącego sensora
            for (const QJsonValue &existingSensorValue : existingSensors) {
                QJsonObject existingSensor = existingSensorValue.toObject();
                if (existingSensor["paramCode"].toString() == paramCode) {
                    sensorExists = true;
                    // Aktualizuj latestValue
                    updatedSensor["latestValue"] = newSensor["latestValue"];

                    // Scal dane historyczne, unikając duplikacji
                    QJsonArray existingHistorical = existingSensor["historicalData"].toArray();
                    QJsonArray newHistorical = newSensor["historicalData"].toArray();
                    QSet<QString> existingDates;
                    for (const QJsonValue &val : existingHistorical) {
                        existingDates.insert(val.toObject()["date"].toString());
                    }

                    // Dodaj tylko nowe unikalne wpisy historyczne na początek
                    QJsonArray mergedHistorical;
                    // Najpierw dodaj nowe dane
                    for (const QJsonValue &newVal : newHistorical) {
                        QString date = newVal.toObject()["date"].toString();
                        if (!existingDates.contains(date)) {
                            mergedHistorical.append(newVal);
                            existingDates.insert(date);
                        }
                    }
                    // Następnie dodaj istniejące dane
                    for (const QJsonValue &existingVal : existingHistorical) {
                        mergedHistorical.append(existingVal);
                    }
                    updatedSensor["historicalData"] = mergedHistorical;
                    break;
                }
            }

            if (!sensorExists) {
                // Nowy sensor - dodaj w całości
                updatedSensor = newSensor;
            }

            updatedSensors.append(updatedSensor);
        }

        // Zaktualizuj listę sensorów dla stacji
        existingStation["sensors"] = updatedSensors;
        existingStation["location"] = location; // Zaktualizuj lokalizację
        stationsArray[stationIndex] = existingStation;
    } else {
        // Nowa stacja - dodaj do tablicy
        stationsArray.append(stationObj);
    }

    // Zaktualizuj główny obiekt JSON
    rootObj["stations"] = stationsArray;

    // Zapis do pliku
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Nie można otworzyć pliku do zapisu:" << fileName;
        QMessageBox::warning(this, tr("Błąd"), tr("Nie można zapisać pliku: %1").arg(file.errorString()));
        return;
    }

    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "Dane zapisane do pliku:" << fileName;
}
