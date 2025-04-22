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
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), networkManager(new QNetworkAccessManager(this)), currentMode(0), geocodingDone(false), sortMode("none") {

    // Ustawienie ikony okna
    setWindowIcon(QIcon(":/icons/hatsune.png"));

    // Tworzenie widżetu listy
    stationListWidget = new QListWidget(this);
    connect(stationListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (currentMode == 0 || currentMode == 1) { // Obsługa w trybie "Wybierz Stację" i "Podaj Lokalizację"
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
    connect(stationListWidget, &QListWidget::itemActivated, this, &MainWindow::onStationActivated);

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
    searchButton->setStyleSheet("padding: 5px; font-size: 14px; background-color: #4CAF50; color: white; border: none; min-width: 80px;");
    searchButton->setVisible(false);

    // Tworzenie listy rozwijanej dla sortowania
    sortComboBox = new QComboBox(this);
    sortComboBox->addItems({"Bez sortowania", "Sortuj po ID"});
    sortComboBox->setStyleSheet("padding: 5px; font-size: 14px;");
    connect(sortComboBox, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        if (text == "Sortuj po ID") {
            sortMode = "id";
        } else if (text == "Sortuj po km") {
            sortMode = "distance";
        } else {
            sortMode = "none";
        }
        onSearchTextChanged(searchEdit->text()); // Odśwież listę w trybie "Wybierz Stację"
        if (currentMode == 1 && geocodingDone) {
            // Odśwież listę w trybie "Podaj Lokalizację"
            onSearchButtonClicked();
        }
    });

    // Tworzenie przycisku Odczytaj Dane z Pliku
    loadFileButton = new QPushButton("Odczytaj Dane z Pliku", this);
    loadFileButton->setStyleSheet("padding: 5px; font-size: 16px; font-weight: bold; background-color: #6BB8D8; color: white; border: none; height: 30px; min-width: 200px;");
    loadFileButton->setVisible(true); // Guzik widoczny od razu w trybie "Wybierz Stację"
    connect(loadFileButton, &QPushButton::clicked, this, &MainWindow::onLoadFileButtonClicked);

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
    searchLayout->addWidget(sortComboBox);

    QHBoxLayout *radiusLayout = new QHBoxLayout();
    radiusLayout->addWidget(radiusEdit);
    radiusLayout->addWidget(searchButton);
    radiusLayout->addStretch();

    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(searchLayout);
    mainLayout->addLayout(radiusLayout);
    mainLayout->addWidget(stationListWidget);
    mainLayout->addWidget(loadFileButton);
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

        // Ustaw rozmiar sceny
        const double mapWidth = 600;
        const double mapHeight = 465;
        mapScene->setSceneRect(0, 0, mapWidth, mapHeight);

        // Wczytaj i przeskaluj mapę
        QPixmap mapPixmap(":/images/poland_map.png");
        if (mapPixmap.isNull()) {
            qDebug() << "Błąd: Nie udało się załadować obrazu mapy!";
            QMessageBox::critical(this, "Błąd", "Nie udało się załadować obrazu mapy.");
            reply->deleteLater();
            return;
        }

        mapPixmap = mapPixmap.scaled(mapWidth, mapHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QGraphicsPixmapItem *mapItem = new QGraphicsPixmapItem(mapPixmap);

        // Oblicz przesunięcie, jeśli mapa nie wypełnia całej sceny
        double mapPixmapWidth = mapPixmap.width();
        double mapPixmapHeight = mapPixmap.height();
        double offsetX = (mapWidth - mapPixmapWidth) / 2.0;
        double offsetY = (mapHeight - mapPixmapHeight) / 2.0;
        mapItem->setPos(offsetX, offsetY);
        mapScene->addItem(mapItem);

        // Oblicz rzeczywiste granice stacji
        double actualLonMin = std::numeric_limits<double>::max();
        double actualLonMax = std::numeric_limits<double>::min();
        double actualLatMin = std::numeric_limits<double>::max();
        double actualLatMax = std::numeric_limits<double>::min();

        for (const QJsonValue &value : allStations) {
            QJsonObject station = value.toObject();
            if (station.contains("gegrLat") && station.contains("gegrLon")) {
                double lat = station["gegrLat"].toString().toDouble();
                double lon = station["gegrLon"].toString().toDouble();
                if (lon < actualLonMin) actualLonMin = lon;
                if (lon > actualLonMax) actualLonMax = lon;
                if (lat < actualLatMin) actualLatMin = lat;
                if (lat > actualLatMax) actualLatMax = lat;
            }
        }
        qDebug() << "Actual lon range:" << actualLonMin << "to" << actualLonMax;
        qDebug() << "Actual lat range:" << actualLatMin << "to" << actualLatMax;

        // Oblicz wartości dla projekcji Mercator (oś Y)
        auto mercatorY = [](double lat) {
            const double R = 6371.0;
            double latRad = qDegreesToRadians(lat);
            return R * log(tan(M_PI / 4 + latRad / 2));
        };

        double mercatorMin = mercatorY(actualLatMin);
        double mercatorMax = mercatorY(actualLatMax);
        double mercatorRange = mercatorMax - mercatorMin;

        // Skaluj kropki do rozmiaru mapy (nie sceny)
        double scaleX = mapPixmapWidth / (actualLonMax - actualLonMin);
        double scaleY = mapPixmapHeight / mercatorRange;

        // Narysuj kropki dla stacji
        for (const QJsonValue &value : allStations) {
            QJsonObject station = value.toObject();
            if (station.contains("gegrLat") && station.contains("gegrLon") && station.contains("stationName")) {
                double lat = station["gegrLat"].toString().toDouble();
                double lon = station["gegrLon"].toString().toDouble();
                QString name = station["stationName"].toString();
                int stationId = station["id"].toInt();

                // Oblicz pozycję kropki w skali mapy
                double x = (lon - actualLonMin) * scaleX;
                double mercatorLat = mercatorY(lat);
                double y = (mercatorMax - mercatorLat) * scaleY;

                // Dodaj przesunięcie, aby kropki były wyrównane z mapą
                x += offsetX;
                y += offsetY;

                ClickableEllipseItem *dot = new ClickableEllipseItem(stationId, x - 5, y - 5, 10, 10);
                dot->setBrush(QBrush(Qt::red));
                dot->setPen(QPen(Qt::black, 1));
                dot->setToolTip(name);
                dot->setZValue(1);
                mapScene->addItem(dot);

                connect(dot, &ClickableEllipseItem::clicked, this, &MainWindow::onEllipseClicked);
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
        QList<QPair<int, QString>> stationItems; // Para: ID, tekst elementu

        for (const QJsonValue &value : allStations) {
            QJsonObject station = value.toObject();
            if (station.contains("city") && station["city"].isObject()) {
                QJsonObject city = station["city"].toObject();
                QString cityName = city["name"].toString().toLower();
                if (cityName.contains(searchText) || searchText.isEmpty()) {
                    QString stationName = station["stationName"].toString();
                    int stationId = station["id"].toInt();
                    QString itemText = QString("ID: %1 - %2").arg(stationId).arg(stationName);
                    stationItems.append(qMakePair(stationId, itemText));
                }
            }
        }

        // Sortowanie, jeśli wybrano sortowanie po ID
        if (sortMode == "id") {
            std::sort(stationItems.begin(), stationItems.end(),
                      [](const QPair<int, QString> &a, const QPair<int, QString> &b) {
                          return a.first < b.first;
                      });
        }

        // Dodaj posortowane elementy do listy
        for (const auto &item : stationItems) {
            stationListWidget->addItem(item.second);
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

    sortComboBox->clear();
    if (currentMode == 0) {
        sortComboBox->addItems({"Bez sortowania", "Sortuj po ID"});
    } else if (currentMode == 1) {
        sortComboBox->addItems({"Bez sortowania", "Sortuj po ID", "Sortuj po km"});
    } else {
        sortComboBox->addItems({"Bez sortowania"});
    }
    sortMode = "none";

    switch (currentMode) {
    case 0:
        titleButton->updateOriginalText("Wybierz Stację");
        iconLabel->setPixmap(QPixmap(":/icons/station_icon.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        searchEdit->setPlaceholderText("Wpisz miejscowość...");
        searchEdit->setVisible(true);
        searchButton->setVisible(false);
        radiusEdit->setVisible(false);
        stationListWidget->setVisible(true);
        loadFileButton->setVisible(true);
        mapView->setVisible(false);
        sortComboBox->setVisible(true);
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
        loadFileButton->setVisible(false);
        mapView->setVisible(false);
        sortComboBox->setVisible(true);
        break;
    case 2:
        titleButton->updateOriginalText("Mapa Stacji");
        iconLabel->setPixmap(QPixmap(":/icons/map_icon.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        searchEdit->setVisible(false);
        searchButton->setVisible(false);
        radiusEdit->setVisible(false);
        stationListWidget->setVisible(false);
        loadFileButton->setVisible(false);
        mapView->setVisible(true);
        sortComboBox->setVisible(false);
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

                QList<QPair<QPair<double, int>, QString>> stationItems; // Para: (odległość/ID, ID), tekst elementu

                for (const QJsonValue &value : allStations) {
                    QJsonObject station = value.toObject();
                    if (station.contains("gegrLat") && station.contains("gegrLon")) {
                        double stationLat = station["gegrLat"].toString().toDouble();
                        double stationLon = station["gegrLon"].toString().toDouble();
                        double distance = calculateDistance(userLat, userLon, stationLat, stationLon);

                        if (distance <= radiusKm) {
                            QString stationName = station["stationName"].toString();
                            int stationId = station["id"].toInt();
                            QString itemText = QString("ID: %1 - %2 (%3 km)")
                                                   .arg(stationId)
                                                   .arg(stationName)
                                                   .arg(distance, 0, 'f', 1);
                            double sortKey = (sortMode == "distance") ? distance : stationId;
                            stationItems.append(qMakePair(qMakePair(sortKey, stationId), itemText));
                        }
                    }
                }

                // Sortowanie
                if (sortMode == "id" || sortMode == "distance") {
                    std::sort(stationItems.begin(), stationItems.end(),
                              [](const QPair<QPair<double, int>, QString> &a, const QPair<QPair<double, int>, QString> &b) {
                                  return a.first.first < b.first.first;
                              });
                }

                // Dodaj posortowane elementy do listy
                for (const auto &item : stationItems) {
                    stationListWidget->addItem(item.second);
                }

                if (stationItems.isEmpty()) {
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

void MainWindow::onEllipseClicked(int stationId) {
    if (currentMode == 2) { // Obsługa tylko w trybie "Mapa Stacji"
        qDebug() << "Kliknięto kropkę stacji o ID:" << stationId;

        // Znajdź stację w allStations, aby uzyskać dane
        QString stationName, communeName, provinceName;
        for (const QJsonValue &value : allStations) {
            QJsonObject station = value.toObject();
            if (station["id"].toInt() == stationId) {
                stationName = station["stationName"].toString();
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

void MainWindow::onStationActivated(QListWidgetItem *item) {
    if (currentMode == 0 || currentMode == 1) { // Obsługa w trybie "Wybierz Stację" i "Podaj Lokalizację"
        QString text = item->text();
        qDebug() << "Aktywowano stację:" << text;
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
}

void MainWindow::onLoadFileButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Otwórz plik JSON"), QCoreApplication::applicationDirPath(), tr("Pliki JSON (*.json)"));
    if (fileName.isEmpty()) {
        qDebug() << "Odczyt anulowany przez użytkownika.";
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Błąd"), tr("Nie można otworzyć pliku: %1").arg(file.errorString()));
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(fileData);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::warning(this, tr("Błąd"), tr("Nieprawidłowy format pliku JSON."));
        return;
    }

    QJsonObject rootObj = doc.object();
    if (!rootObj.contains("stations") || !rootObj["stations"].isArray()) {
        QMessageBox::warning(this, tr("Błąd"), tr("Plik nie zawiera danych stacji."));
        return;
    }

    QJsonArray stationsArray = rootObj["stations"].toArray();
    if (stationsArray.isEmpty()) {
        QMessageBox::warning(this, tr("Błąd"), tr("Plik nie zawiera żadnych stacji."));
        return;
    }

    // Pobierz pierwszą stację z pliku
    QJsonObject stationObj = stationsArray[0].toObject();
    QString stationName = stationObj["stationName"].toString();
    QString location = stationObj["location"].toString();
    QJsonArray sensorsArray = stationObj["sensors"].toArray();

    if (stationName.isEmpty() || sensorsArray.isEmpty()) {
        QMessageBox::warning(this, tr("Błąd"), tr("Plik zawiera nieprawidłowe lub niekompletne dane stacji."));
        return;
    }

    // Wyłącz interakcję z listą stacji
    stationListWidget->setEnabled(false);
    // Pokaż kartę z danymi z pliku
    infoCard->setVisible(true);
    infoCard->showDataFromFile(stationName, location, sensorsArray);
}
