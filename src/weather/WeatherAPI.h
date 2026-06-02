//
// Created by Muhammad Muaaz on 3/11/2026.
//
#ifndef WEATHERAPI_H
#define WEATHERAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantMap>

class WeatherAPI : public QObject
{
    Q_OBJECT

public:
    // Owns the shared network manager used for weather requests and icon fetches.
    explicit WeatherAPI(QObject *parent = nullptr);

    // Requests current conditions for the given city name.
    void fetchWeather(QString city);

    // Requests the five day and hourly forecast payloads for the city.
    void fetchForecast(QString city);

    // Downloads a small weather icon image synchronously for label display.
    QByteArray downloadIcon(QString url);

signals:

    // Emits the parsed current conditions used by the hero card and stat cards.
    void weatherReady(
        QString locationLabel,
        QString condition,
        double temp,
        int humidity,
        double wind,
        QString icon,
        double precip
    );

    // Emits the compact hourly dataset shown in the right-hand panel.
    void hourlyReady(QList<QVariantMap> hourly);
    // Emits the condensed five day dataset used for the forecast cards.
    void forecastReady(QList<QVariantMap> forecast);
    // Emits the timestamped forecast slots used for alert matching and reminders.
    void forecastTimelineReady(QString locationLabel, QList<QVariantMap> timeline);
    // Emits a user-facing error when a weather request cannot be fulfilled.
    void weatherError(QString errorMessage);

private:
    // Reuses one manager so requests share the same Qt event-loop lifecycle.
    QNetworkAccessManager manager;

    // Stores the OpenWeather API key used by the app's network calls.
    QString apiKey = "YOUR_API_KEY_HERE";
};

#endif
