//
// Created by Muhammad Muaaz on 3/11/2026.
//
#include "locationService.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

// Owns the network manager used for the one-shot IP lookup request.
LocationService::LocationService(QObject *parent)
    : QObject(parent)
{
}

// Detects the current city so the app has a sensible default on launch.
void LocationService::detectLocation()
{
    QString url = "http://ip-api.com/json";

    QNetworkReply *reply =
        manager.get(QNetworkRequest(QUrl(url)));

    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        // A failed location lookup simply skips auto-fill and leaves manual search available.
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            return;
        }

        const QByteArray data = reply->readAll();

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            reply->deleteLater();
            return;
        }

        const QJsonObject obj = document.object();
        const QString city = obj.value("city").toString().trimmed();
        const QString country = obj.value("country").toString().trimmed();
        if (city.isEmpty()) {
            reply->deleteLater();
            return;
        }

        emit cityDetected(city, country);

        reply->deleteLater();
    });
}
