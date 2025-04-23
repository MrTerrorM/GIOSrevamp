/**
 * @file custombutton.h
 * @brief Klasa reprezentująca niestandardowy przycisk z efektami wizualnymi.
 */

#ifndef CUSTOMBUTTON_H
#define CUSTOMBUTTON_H

#include <QPushButton>
#include <QEnterEvent>
#include <QFocusEvent>

/**
 * @class CustomButton
 * @brief Klasa dziedzicząca po QPushButton, dodająca efekty wizualne przy najechaniu i fokusie.
 *
 * Przycisk zmienia tekst, dodając strzałki (">" i "<") przy najechaniu myszą lub uzyskaniu fokusu.
 */
class CustomButton : public QPushButton {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy CustomButton.
     * @param text Tekst wyświetlany na przycisku.
     * @param parent Wskaźnik do nadrzędnego widgetu (domyślnie nullptr).
     */
    CustomButton(const QString &text, QWidget *parent = nullptr) : QPushButton(text, parent) {
        originalText = text; // Przechowujemy oryginalny tekst
        setMouseTracking(true); // Włączamy śledzenie myszy
    }

protected:
    /**
     * @brief Obsługuje zdarzenie najechania myszą.
     * @param event Wskaźnik do obiektu zdarzenia.
     */
    void enterEvent(QEnterEvent *event) override {
        // Dodajemy strzałki przy najechaniu
        setText(">" + originalText + "<");
        QPushButton::enterEvent(event);
    }

    /**
     * @brief Obsługuje zdarzenie opuszczenia myszą.
     * @param event Wskaźnik do obiektu zdarzenia.
     */
    void leaveEvent(QEvent *event) override {
        // Przywracamy oryginalny tekst przy opuszczeniu
        setText(originalText);
        QPushButton::leaveEvent(event);
    }

    /**
     * @brief Obsługuje zdarzenie uzyskania fokusu.
     * @param event Wskaźnik do obiektu zdarzenia fokusu.
     */
    void focusInEvent(QFocusEvent *event) override {
        // Dodajemy strzałki przy fokusie
        setText(">" + originalText + "<");
        QPushButton::focusInEvent(event);
    }

    /**
     * @brief Obsługuje zdarzenie utraty fokusu.
     * @param event Wskaźnik do obiektu zdarzenia fokusu.
     */
    void focusOutEvent(QFocusEvent *event) override {
        // Przywracamy oryginalny tekst przy utracie fokusu
        setText(originalText);
        QPushButton::focusOutEvent(event);
    }

public:
    /**
     * @brief Aktualizuje oryginalny tekst przycisku.
     * @param newText Nowy tekst do wyświetlenia.
     */
    // Metoda do aktualizacji oryginalnego tekstu (np. przy zmianie trybu)
    void updateOriginalText(const QString &newText) {
        originalText = newText;
        setText(newText); // Ustawiamy nowy tekst bez strzałek
    }

private:
    QString originalText; ///< Oryginalny tekst przycisku.
};

#endif // CUSTOMBUTTON_H
