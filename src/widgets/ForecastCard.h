//
// Created by Muhammad Muaaz on 3/11/2026.
//
#ifndef FORECASTCARD_H
#define FORECASTCARD_H

#include <QWidget>

class QLabel;

class ForecastCard : public QWidget
{
public:
    // Renders one day in the five day forecast strip.
    ForecastCard(
        QString day,
        QString temp,
        QString humidity,
        QString icon
        );

private:
    // Holds the day label shown at the top of the card.
    QLabel *dayLabel;
    // Shows the forecast temperature string.
    QLabel *tempLabel;
    // Shows the forecast humidity string.
    QLabel *humidityLabel;
    // Hosts the downloaded weather icon.
    QLabel *iconLabel;
};

#endif
