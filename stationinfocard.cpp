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
    bool allDataMissing = true;

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
                    allDataMissing = false;
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
    if (sensorComboBox->count() > 0) {
        updateChart(sensorComboBox->currentData().toString());
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
    // Otwórz okno dialogowe do wyboru pliku
    QString fileName = QFileDialog::getSaveFileName(this, tr("Zapisz dane stacji"), "", tr("Pliki JSON (*.json)"));
    if (!fileName.isEmpty()) {
        saveDataToJson(fileName);
    }
}

void StationInfoCard::saveDataToJson(const QString &fileName) {
    QJsonObject rootObj;

    // Metadane
    rootObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rootObj["stationName"] = stationNameLabel->text();
    rootObj["location"] = locationLabel->text();
    rootObj["selectedParameter"] = sensorComboBox->currentData().toString();
    rootObj["timeRange"] = currentTimeRange;

    // Dane sensorów
    QJsonArray sensorsArray;
    for (int col = 0; col < dataTable->columnCount(); ++col) {
        QJsonObject sensorObj;
        QString paramCode = dataTable->item(0, col)->text();
        sensorObj["paramCode"] = paramCode;
        sensorObj["paramName"] = paramNames.value(paramCode, paramCode);
        sensorObj["latestValue"] = dataTable->item(1, col)->text();

        // Dane historyczne
        if (sensorValues.contains(paramCode)) {
            QJsonArray valuesArray = sensorValues[paramCode];
            sensorObj["historicalData"] = valuesArray;
        } else {
            sensorObj["historicalData"] = QJsonArray();
        }
        sensorsArray.append(sensorObj);
    }
    rootObj["sensors"] = sensorsArray;

    // Tworzenie dokumentu JSON
    QJsonDocument doc(rootObj);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Nie można otworzyć pliku do zapisu:" << fileName;
        QMessageBox::warning(this, tr("Błąd"), tr("Nie można zapisać pliku: %1").arg(file.errorString()));
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "Dane zapisane do pliku:" << fileName;
    QMessageBox::information(this, tr("Sukces"), tr("Dane zostały zapisane do %1").arg(fileName));
}
