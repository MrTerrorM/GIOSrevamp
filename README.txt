GIOSrevamp - Aplikacja do monitorowania stacji pomiarowych
=========================================================

Opis projektu
-------------
GIOSrevamp to aplikacja napisana w Qt, która pozwala użytkownikom na:
- Pobieranie danych z publicznych API GIOŚ (Główny Inspektorat Ochrony Środowiska).
- Wyświetlanie danych sensorów w formie tabeli i wykresów historycznych.
- Zapisywanie danych do plików JSON.
- Wczytywanie zapisanych danych z plików JSON.

Wymagania
---------
Opisane w pliku requirements.txt

Instalacja
----------
1. Zainstaluj QtCreator
2. Pobierz projekt GIOSrevamp
3. Otwórz plik projektu (GIOSrevamp.pro) w Qt Creator.
4. Skompiluj i uruchom aplikację.

Użycie
------
1. Uruchom aplikację.
2. Wybierz stację pomiarową z mapy lub listy (jeśli dostępna).
3. Wyświetl dane sensorów (np. NO2, PM10) w tabeli.
4. Wybierz zakres czasowy (dzień, tydzień, miesiąc, pół roku, rok) za pomocą listy rozwijanej.
5. Przeglądaj wykresy historyczne dla wybranego sensora.
6. Kliknij "Zapisz", aby zapisać dane do pliku JSON w katalogu "data".
   - Plik będzie nazwany na podstawie nazwy stacji (np. "Swieradow-Zdroj.json").
7. Aby wczytać zapisane dane, wybierz opcję wczytywania z pliku (jeśli dostępna).

Autor
-----
Piotr Mądrzak
Data: 2025-04-22
