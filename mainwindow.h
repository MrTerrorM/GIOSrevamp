/**
 * @file mainwindow.h
 * @brief Główna klasa okna aplikacji GIOSrevamp.
 */

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

/**
 * @class MainWindow
 * @brief Klasa reprezentująca główne okno aplikacji do wyświetlania danych stacji pogodowych.
 *
 * Odpowiada za interfejs użytkownika, pobieranie danych z API GIOŚ, wyświetlanie mapy oraz zarządzanie trybami aplikacji.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy MainWindow.
     * @param parent Wskaźnik do nadrzędnego widgetu (domyślnie nullptr).
     */
    MainWindow(QWidget *parent = nullptr);
    /**
     * @brief Destruktor klasy MainWindow.
     */
    ~MainWindow();

private slots:
    /**
     * @brief Obsługuje zakończenie odpowiedzi HTTP z API.
     * @param reply Wskaźnik do obiektu odpowiedzi sieciowej.
     */
    void onReplyFinished(QNetworkReply *reply);
    /**
     * @brief Obsługuje zmianę tekstu w polu wyszukiwania.
     * @param text Nowy tekst w polu wyszukiwania.
     */
    void onSearchTextChanged(const QString &text);
    /**
     * @brief Obsługuje kliknięcie przycisku tytułu.
     */
    void onTitleButtonClicked();
    /**
     * @brief Obsługuje kliknięcie przycisku wyszukiwania.
     */
    void onSearchButtonClicked();
    /**
     * @brief Obsługuje odpowiedź geokodowania z API Nominatim.
     * @param reply Wskaźnik do obiektu odpowiedzi sieciowej.
     */
    void onGeocodingReplyFinished(QNetworkReply *reply);
    /**
     * @brief Obsługuje kliknięcie stacji na liście lub mapie.
     * @param stationId Identyfikator stacji.
     * @param stationName Nazwa stacji.
     * @param communeName Nazwa gminy.
     * @param provinceName Nazwa województwa.
     */
    void onStationClicked(int stationId, const QString &stationName, const QString &communeName, const QString &provinceName);
    /**
     * @brief Obsługuje aktywację stacji na liście.
     * @param item Wskaźnik do elementu listy.
     */
    void onStationActivated(QListWidgetItem *item);
    /**
     * @brief Obsługuje kliknięcie przycisku wczytywania pliku JSON.
     */
    void onLoadFileButtonClicked();
    /**
     * @brief Obsługuje kliknięcie elipsy na mapie.
     * @param stationId Identyfikator stacji powiązanej z elipsą.
     */
    void onEllipseClicked(int stationId);

private:
    /**
     * @brief Wykonuje geokodowanie podanej lokalizacji.
     * @param location Nazwa lokalizacji do geokodowania.
     */
    void geocodeLocation(const QString &location);
    /**
     * @brief Oblicza odległość między dwoma punktami geograficznymi.
     * @param lat1 Szerokość geograficzna pierwszego punktu.
     * @param lon1 Długość geograficzna pierwszego punktu.
     * @param lat2 Szerokość geograficzna drugiego punktu.
     * @param lon2 Długość geograficzna drugiego punktu.
     * @return Odległość w kilometrach.
     */
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);

    QNetworkAccessManager *networkManager; ///< Menadżer sieci do zapytań HTTP.
    QListWidget *stationListWidget; ///< Lista stacji pogodowych.
    QGraphicsView *mapView; ///< Widok mapy.
    QGraphicsScene *mapScene; ///< Scena mapy.
    CustomButton *titleButton; ///< Przycisk tytułu okna.
    QLabel *iconLabel; ///< Etykieta ikony.
    QLineEdit *searchEdit; ///< Pole wyszukiwania.
    QLineEdit *radiusEdit; ///< Pole promienia wyszukiwania.
    QPushButton *searchButton; ///< Przycisk wyszukiwania.
    QComboBox *sortComboBox; ///< Lista rozwijana sortowania.
    QPushButton *loadFileButton; ///< Przycisk wczytywania pliku.
    StationInfoCard *infoCard; ///< Karta informacyjna stacji.
    QJsonArray allStations; ///< Tablica wszystkich stacji z API.
    int currentMode; ///< Aktualny tryb aplikacji (0: Wybierz Stację, 1: Podaj Lokalizację, 2: Mapa Stacji).
    bool geocodingDone; ///< Flaga wskazująca, czy geokodowanie zakończone.
    QString sortMode; ///< Tryb sortowania listy stacji.
    double userLat; ///< Szerokość geograficzna użytkownika.
    double userLon; ///< Długość geograficzna użytkownika.
};

#endif // MAINWINDOW_H
