#ifndef CUSTOMBUTTON_H
#define CUSTOMBUTTON_H

#include <QPushButton>
#include <QEnterEvent> // Dodajemy nagłówek dla QEnterEvent

class CustomButton : public QPushButton {
    Q_OBJECT

public:
    CustomButton(const QString &text, QWidget *parent = nullptr) : QPushButton(text, parent) {
        originalText = text; // Przechowujemy oryginalny tekst
        setMouseTracking(true); // Włączamy śledzenie myszy
    }

protected:
    void enterEvent(QEnterEvent *event) override { // Zmieniamy QEvent na QEnterEvent
        // Dodajemy strzałki przy najechaniu
        setText(">" + originalText + "<");
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        // Przywracamy oryginalny tekst przy opuszczeniu
        setText(originalText);
        QPushButton::leaveEvent(event);
    }

public:
    // Metoda do aktualizacji oryginalnego tekstu (np. przy zmianie trybu)
    void updateOriginalText(const QString &newText) {
        originalText = newText;
        setText(newText); // Ustawiamy nowy tekst bez strzałek
    }

private:
    QString originalText; // Przechowujemy oryginalny tekst
};

#endif // CUSTOMBUTTON_H
