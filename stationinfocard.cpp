#include "stationinfocard.h"
#include <QPropertyAnimation>
#include <QDebug>
#include <QPalette>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>

StationInfoCard::StationInfoCard(QWidget *parent)
    : QFrame(parent), networkManager(new QNetworkAccessManager(this)), pendingRequests(0) {
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
    dataTable->setStyleSheet("QTableWidget { font-size: 14px; border: none; background-color: black; color: white; } QTableWidget::item { padding: 5px; }");
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

    // Ustawienie layoutu
    layout = new QVBoxLayout(this);
    QHBoxLayout *topLayout = new QHBoxLayout();
    QVBoxLayout *leftTextLayout = new QVBoxLayout();
    leftTextLayout->addWidget(titleLabel);
    topLayout->addLayout(leftTextLayout);
    topLayout->addStretch();
    topLayout->addWidget(closeButton, 0, Qt::AlignTop);
    layout->addLayout(topLayout);

    QHBoxLayout *stationNameLayout = new QHBoxLayout();
    stationNameLayout->addWidget(stationNameLabel);
    layout->addLayout(stationNameLayout);

    layout->addWidget(locationLabel);
    layout->addSpacerItem(new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Fixed)); // Dwie linijki odstępu

    // Wyśrodkowanie tabeli
    QHBoxLayout *tableLayout = new QHBoxLayout();
    tableLayout->addStretch();
    tableLayout->addWidget(dataTable);
    tableLayout->addStretch();
    layout->addLayout(tableLayout);

    layout->addStretch();
    setLayout(layout);

    // Początkowo karta jest niewidoczna
    setVisible(false);
    qDebug() << "StationInfoCard utworzona";
}

void StationInfoCard::showStationData(int stationId, const QString &stationName, const QString &communeName, const QString &provinceName) {
    qDebug() << "Pokazywanie danych dla stacji ID:" << stationId << "Nazwa:" << stationName << "Gmina:" << communeName << "Województwo:" << provinceName;

    // Ustaw nazwę stacji i lokalizację
    stationNameLabel->setText(stationName);
    locationLabel->setText(QString("%1, %2").arg(communeName, provinceName));

    // Wyczyść poprzednie dane
    sensorData.clear();
    pendingRequests = 0;
    dataTable->setColumnCount(0); // Wyczyść tabelę

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
            dataTable->setColumnCount(1);
            dataTable->setItem(0, 0, new QTableWidgetItem("Brak danych"));
            dataTable->setItem(1, 0, new QTableWidgetItem(""));
            dataTable->resizeColumnsToContents();
            adjustTableWidth();
        } else {
            QJsonArray sensors = doc.array();
            pendingRequests = sensors.size();
            qDebug() << "Znaleziono" << sensors.size() << "sensorów";

            if (sensors.isEmpty()) {
                sensorData.append("Brak danych sensorów dla tej stacji.");
                dataTable->setColumnCount(1);
                dataTable->setItem(0, 0, new QTableWidgetItem("Brak danych"));
                dataTable->setItem(1, 0, new QTableWidgetItem(""));
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
                    if (paramNames.contains(paramCode)) {
                        paramItem->setToolTip(paramNames[paramCode]);
                    } else {
                        paramItem->setToolTip(paramCode);
                    }
                    dataTable->setItem(0, column, paramItem);

                    // Pobierz dane dla sensora
                    QUrl dataUrl(QString("https://api.gios.gov.pl/pjp-api/rest/data/getData/%1").arg(sensorId));
                    qDebug() << "Wysyłanie zapytania o dane do:" << dataUrl.toString();
                    QNetworkRequest dataRequest;
                    dataRequest.setUrl(dataUrl);
                    dataRequest.setHeader(QNetworkRequest::UserAgentHeader, "MyStationFinderApp/1.0");
                    QNetworkReply *dataReply = networkManager->get(dataRequest);
                    connect(dataReply, &QNetworkReply::finished, this, [this, column]() {
                        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
                        if (reply) {
                            onDataReplyFinished(reply, column);
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
        dataTable->setColumnCount(1);
        dataTable->setItem(0, 0, new QTableWidgetItem("Błąd"));
        dataTable->setItem(1, 0, new QTableWidgetItem(""));
        dataTable->resizeColumnsToContents();
        adjustTableWidth();
    }
    reply->deleteLater();
}

void StationInfoCard::onDataReplyFinished(QNetworkReply *reply, int column) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray rawData = reply->readAll();
        qDebug() << "Odpowiedź API GIOŚ (dane sensora):" << rawData;

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
            dataTable->setItem(1, column, valueItem);
            sensorData.append(valueText);
        } else {
            qDebug() << "Błąd: Nie udało się sparsować JSON z danych sensora.";
            QTableWidgetItem *valueItem = new QTableWidgetItem("Błąd");
            valueItem->setTextAlignment(Qt::AlignCenter);
            dataTable->setItem(1, column, valueItem);
            sensorData.append("Błąd");
        }
    } else {
        qDebug() << "Błąd pobierania danych sensora:" << reply->errorString();
        QTableWidgetItem *valueItem = new QTableWidgetItem("Błąd");
        valueItem->setTextAlignment(Qt::AlignCenter);
        dataTable->setItem(1, column, valueItem);
        sensorData.append("Błąd");
    }

    reply->deleteLater();
    pendingRequests--;

    if (pendingRequests == 0) {
        qDebug() << "Wszystkie dane sensorów pobrane:" << sensorData;
        // Usuń puste kolumny
        for (int col = dataTable->columnCount() - 1; col >= 0; --col) {
            QTableWidgetItem *valueItem = dataTable->item(1, col);
            if (valueItem && (valueItem->text() == "Brak danych" || valueItem->text() == "Błąd")) {
                dataTable->removeColumn(col);
            }
        }
        dataTable->resizeColumnsToContents();
        adjustTableWidth();
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
    // Dopasuj szerokość tabeli do zawartości
    int totalWidth = 0;
    for (int col = 0; col < dataTable->columnCount(); ++col) {
        totalWidth += dataTable->columnWidth(col);
    }
    // Dodaj marginesy i padding
    totalWidth += 3; // Dodatkowy margines dla estetyki
    dataTable->setMaximumWidth(totalWidth);
    dataTable->setMinimumWidth(totalWidth);
}
