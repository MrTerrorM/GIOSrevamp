#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QLineEdit>
#include <QIcon>
#include <QtMath>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), networkManager(new QNetworkAccessManager(this)), currentMode(0), geocodingDone(false) {

    // Ustawienie ikony okna
    setWindowIcon(QIcon(":/icons/hatsune.png"));

    // Tworzenie widżetu listy
    stationListWidget = new QListWidget(this);
    connect(stationListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (currentMode == 0) { // Tylko w trybie "Wybierz Stację"
            QString text = item->text();
            qDebug() << "Kliknięto stację:" << text;
            int stationId = text.split(" - ").first().remove("ID: ").toInt();
            QString stationName = text.split(" - ").last().split(" (").first(); // Wyodrębnij nazwę stacji

            // Znajdź stację w allStations, aby uzyskać communeName i provinceName
            QString communeName, provinceName;
            for (const QJsonValue &value : allStations) {
                QJsonObject station = value.toObject();
                if (station["id"].toInt() == stationId) {
                    QJsonObject city = station["city"].toObject();
                    QJsonObject commune = city["commune"].toObject();
                    communeName = commune["communeName"].toString();
                    provinceName = commune["provinceName"].toString();
                    break;
                }
            }

            qDebug() << "Wyodrębnione ID stacji:" << stationId << "Nazwa:" << stationName
                     << "Gmina:" << communeName << "Województwo:" << provinceName;
            onStationClicked(stationId, stationName, communeName, provinceName);
        }
    });

    // Tworzenie widżetu mapy i sceny
    mapView = new QGraphicsView(this);
    mapScene = new QGraphicsScene(this);
    mapView->setScene(mapScene);
    mapView->setVisible(false);

    // Tworzenie przycisku z tytułem
    titleButton = new CustomButton("Wybierz Stację", this);
    titleButton->setFlat(true);
    titleButton->setStyleSheet("font-size: 24px; font-weight: bold; color: darkblue; background-color: #f0f0f0; margin: 10px; padding: 5px;");
    titleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Tworzenie etykiety dla ikony
    iconLabel = new QLabel(this);
    iconLabel->setPixmap(QPixmap(":/icons/station_icon.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setFixedSize(48, 48);
    iconLabel->setAlignment(Qt::AlignCenter);

    // Tworzenie pola wyszukiwania
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Wpisz miejscowość...");
    searchEdit->setStyleSheet("padding: 5px; font-size: 14px;");

    // Tworzenie pola dla promienia
    radiusEdit = new QLineEdit(this);
    radiusEdit->setPlaceholderText("Promień (km)...");
    radiusEdit->setStyleSheet("padding: 5px; font-size: 14px;");
    radiusEdit->setVisible(false);

    // Tworzenie przycisku Szukaj
    searchButton = new QPushButton("Szukaj", this);
    searchButton->setStyleSheet("padding: 5px; font-size: 14px; background-color: #4CAF50; color: white; border: none;");
    searchButton->setVisible(false);

    // Tworzenie karty informacyjnej jako nakładki
    infoCard = new StationInfoCard(this); // Bez kontenera, bezpośrednio w MainWindow
    infoCard->setVisible(false); // Początkowo niewidoczna
    connect(infoCard, &StationInfoCard::cardClosed, this, [this]() {
        stationListWidget->setEnabled(true); // Włącz interakcję z listą po zamknięciu karty
        qDebug() << "Karta zamknięta, interakcja z listą włączona";
    });

    // Ustawienie layoutu
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleButton);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(searchButton);

    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(radiusEdit);
    mainLayout->addWidget(stationListWidget);
    mainLayout->addWidget(mapView);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    // Ustawienie rozmiaru okna
    resize(600, 800);

    // Wysłanie zapytania HTTP do API GIOŚ
    QUrl url("http://api.gios.gov.pl/pjp-api/rest/station/findAll");
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "MyStationFinderApp/1.0");
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });

    // Połączenie sygnałów
    connect(searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(titleButton, &QPushButton::clicked, this, &MainWindow::onTitleButtonClicked);
    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearchButtonClicked);
}

MainWindow::~MainWindow() {}

void MainWindow::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray rawData = reply->readAll();
        qDebug() << "Surowa odpowiedź API GIOŚ (lista stacji):" << rawData;

        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        if (doc.isNull()) {
            qDebug() << "Błąd: Nie udało się sparsować JSON z GIOŚ.";
            QMessageBox::critical(this, "Błąd", "Nieprawidłowy format danych JSON z GIOŚ.");
            reply->deleteLater();
            return;
        }

        allStations = doc.array();
        qDebug() << "Pobrano" << allStations.size() << "stacji";
        if (!allStations.isEmpty()) {
            QJsonObject firstStation = allStations[0].toObject();
            qDebug() << "Przykładowa stacja:" << firstStation;
        }

        // Wyczyść scenę mapy
        mapScene->clear();

        // Ustaw granice geograficzne Polski
        const double lonMin = 14.24;
        const double lonMax = 22.59;
        const double latMin = 49.09;
        const double latMax = 54.61;

        // Ustal rozmiar sceny
        const double mapWidth = 600;
        const double mapHeight = 465;
        mapScene->setSceneRect(0, 0, mapWidth, mapHeight);

        // Dodaj obraz mapy jako tło
        QPixmap mapPixmap(":/images/poland_map.png");
        if (mapPixmap.isNull()) {
            qDebug() << "Błąd: Nie udało się załadować obrazu mapy!";
            QMessageBox::critical(this, "Błąd", "Nie udało się załadować obrazu mapy.");
            reply->deleteLater();
            return;
        }

        mapPixmap = mapPixmap.scaled(mapWidth, mapHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QGraphicsPixmapItem *mapItem = new QGraphicsPixmapItem(mapPixmap);
        mapItem->setPos(0, 0);
        mapScene->addItem(mapItem);

        // Oblicz wartości dla projekcji Mercator (oś Y)
        auto mercatorY = [](double lat) {
            const double R = 6371.0;
            double latRad = qDegreesToRadians(lat);
            return R * log(tan(M_PI / 4 + latRad / 2));
        };

        double mercatorMin = mercatorY(latMin);
        double mercatorMax = mercatorY(latMax);
        double mercatorRange = mercatorMax - mercatorMin;

        // Narysuj kropki dla stacji
        for (const QJsonValue &value : allStations) {
            QJsonObject station = value.toObject();
            if (station.contains("gegrLat") && station.contains("gegrLon") && station.contains("stationName")) {
                double lat = station["gegrLat"].toString().toDouble();
                double lon = station["gegrLon"].toString().toDouble();
                QString name = station["stationName"].toString();

                double x = ((lon - lonMin) / (lonMax - lonMin)) * mapWidth;
                double mercatorLat = mercatorY(lat);
                double y = ((mercatorMax - mercatorLat) / mercatorRange) * mapHeight;

                QGraphicsEllipseItem *dot = new QGraphicsEllipseItem(x - 5, y - 5, 10, 10);
                dot->setBrush(QBrush(Qt::red));
                dot->setPen(QPen(Qt::black, 1));
                dot->setToolTip(name);
                dot->setZValue(1);
                mapScene->addItem(dot);
            }
        }

        onSearchTextChanged(searchEdit->text());
    } else {
        qDebug() << "Błąd pobierania listy stacji:" << reply->errorString();
        QMessageBox::critical(this, "Błąd", "Nie udało się pobrać danych z GIOŚ: " + reply->errorString());
    }
    reply->deleteLater();
}

void MainWindow::onSearchTextChanged(const QString &text) {
    stationListWidget->clear();

    if (currentMode == 0) {
        QString searchText = text.trimmed().toLower();
        for (const QJsonValue &value : allStations) {
            QJsonObject station = value.toObject();
            if (station.contains("city") && station["city"].isObject()) {
                QJsonObject city = station["city"].toObject();
                QString cityName = city["name"].toString().toLower();
                if (cityName.contains(searchText) || searchText.isEmpty()) {
                    QString stationName = station["stationName"].toString();
                    QString stationId = QString::number(station["id"].toInt());
                    stationListWidget->addItem(QString("ID: %1 - %2").arg(stationId, stationName));
                }
            }
        }
    } else if (currentMode == 1) {
        // W trybie "Podaj Lokalizację" lista jest aktualizowana po kliknięciu "Szukaj"
    } else if (currentMode == 2) {
        // W trybie "Mapa Stacji" nie aktualizujemy listy
    }
}

void MainWindow::onTitleButtonClicked() {
    currentMode = (currentMode + 1) % 3;
    stationListWidget->clear();
    infoCard->setVisible(false); // Ukryj kartę przy zmianie trybu
    stationListWidget->setEnabled(true); // Włącz interakcję z listą

    switch (currentMode) {
    case 0:
        titleButton->updateOriginalText("Wybierz Stację");
        iconLabel->setPixmap(QPixmap(":/icons/station_icon.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        searchEdit->setPlaceholderText("Wpisz miejscowość...");
        searchEdit->setVisible(true);
        searchButton->setVisible(false);
        radiusEdit->setVisible(false);
        stationListWidget->setVisible(true);
        mapView->setVisible(false);
        onSearchTextChanged(searchEdit->text());
        break;
    case 1:
        titleButton->updateOriginalText("Podaj Lokalizację");
        iconLabel->setPixmap(QPixmap(":/icons/location_icon.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        searchEdit->setPlaceholderText("Wpisz lokalizację...");
        searchEdit->setVisible(true);
        searchButton->setVisible(true);
        radiusEdit->setVisible(true);
        stationListWidget->setVisible(true);
        mapView->setVisible(false);
        break;
    case 2:
        titleButton->updateOriginalText("Mapa Stacji");
        iconLabel->setPixmap(QPixmap(":/icons/map_icon.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        searchEdit->setVisible(false);
        searchButton->setVisible(false);
        radiusEdit->setVisible(false);
        stationListWidget->setVisible(false);
        mapView->setVisible(true);
        break;
    }
}

void MainWindow::onSearchButtonClicked() {
    QString location = searchEdit->text().trimmed();
    if (location.length() >= 5) {
        geocodingDone = false;
        geocodeLocation(location);
    } else {
        QMessageBox::warning(this, "Błąd", "Wprowadź co najmniej 5 znaków, aby wyszukać lokalizację.");
    }
}

void MainWindow::geocodeLocation(const QString &location) {
    QString encodedLocation = QUrl::toPercentEncoding(location);
    QString urlString = QString("https://nominatim.openstreetmap.org/search?q=%1&format=json&limit=1").arg(encodedLocation);

    QUrl url(urlString);
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "MyStationFinderApp/1.0");

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onGeocodingReplyFinished(reply);
    });
}

void MainWindow::onGeocodingReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray rawData = reply->readAll();
        qDebug() << "Surowa odpowiedź Nominatim:" << rawData;

        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        QJsonArray results = doc.array();

        if (!results.isEmpty()) {
            QJsonObject result = results[0].toObject();
            if (result.contains("lat") && result.contains("lon")) {
                userLat = result["lat"].toString().toDouble();
                userLon = result["lon"].toString().toDouble();
                geocodingDone = true;

                stationListWidget->clear();

                bool ok;
                double radiusKm = radiusEdit->text().toDouble(&ok);
                if (!ok || radiusKm <= 0) {
                    radiusKm = 10.0;
                }

                for (const QJsonValue &value : allStations) {
                    QJsonObject station = value.toObject();
                    if (station.contains("gegrLat") && station.contains("gegrLon")) {
                        double stationLat = station["gegrLat"].toString().toDouble();
                        double stationLon = station["gegrLon"].toString().toDouble();
                        double distance = calculateDistance(userLat, userLon, stationLat, stationLon);

                        if (distance <= radiusKm) {
                            QString stationName = station["stationName"].toString();
                            QString stationId = QString::number(station["id"].toInt());
                            stationListWidget->addItem(QString("ID: %1 - %2 (%3 km)")
                                                           .arg(stationId)
                                                           .arg(stationName)
                                                           .arg(distance, 0, 'f', 1));
                        }
                    }
                }

                if (stationListWidget->count() == 0) {
                    QMessageBox::information(this, "Informacja", "Brak stacji w zadanym promieniu.");
                }
            } else {
                QMessageBox::warning(this, "Błąd", "Nieprawidłowe dane lokalizacji z Nominatim.");
            }
        } else {
            QMessageBox::warning(this, "Błąd", "Nie znaleziono lokalizacji.");
        }
    } else {
        QMessageBox::critical(this, "Błąd", "Nie udało się pobrać danych lokalizacji: " + reply->errorString());
    }
    reply->deleteLater();
}

double MainWindow::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0;

    double lat1Rad = qDegreesToRadians(lat1);
    double lon1Rad = qDegreesToRadians(lon1);
    double lat2Rad = qDegreesToRadians(lat2);
    double lon2Rad = qDegreesToRadians(lon2);

    double dLat = lat2Rad - lat1Rad;
    double dLon = lon2Rad - lon1Rad;

    double a = qSin(dLat / 2) * qSin(dLat / 2) +
               qCos(lat1Rad) * qCos(lat2Rad) *
                   qSin(dLon / 2) * qSin(dLon / 2);
    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    double distance = R * c;

    return distance;
}

void MainWindow::onStationClicked(int stationId, const QString &stationName, const QString &communeName, const QString &provinceName) {
    qDebug() << "Wywołano onStationClicked dla ID:" << stationId << "Nazwa:" << stationName
             << "Gmina:" << communeName << "Województwo:" << provinceName;
    stationListWidget->setEnabled(false); // Wyłącz interakcję z listą stacji
    infoCard->setVisible(true); // Pokaż kartę jako nakładkę
    infoCard->showStationData(stationId, stationName, communeName, provinceName);
}
