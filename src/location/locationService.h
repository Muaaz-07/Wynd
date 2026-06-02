//
// Created by Muhammad Muaaz on 3/11/2026.
//
#ifndef LOCATIONSERVICE_H
#define LOCATIONSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>

class LocationService : public QObject
{
    Q_OBJECT

public:
    // Creates the service that infers the user's city from their IP address.
    explicit LocationService(QObject *parent = nullptr);

    // Starts the location lookup request.
    void detectLocation();

signals:
    // Emits the city and country once the lookup service returns a usable response.
    void cityDetected(QString city, QString country);

private:
    // Reuses a single manager for the location lookup.
    QNetworkAccessManager manager;
};

#endif
