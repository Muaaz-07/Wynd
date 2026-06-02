#include "WeatherAPI.h"

#include <QDateTime>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocale>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimeZone>
#include <QUrl>

#include <algorithm>

namespace {

QString noDataFoundMessage()
{
    return QStringLiteral("No data found");
}

bool hasSuccessCode(const QJsonObject &obj)
{
    const QJsonValue codValue = obj.value(QStringLiteral("cod"));
    const int cod = codValue.isString() ? codValue.toString().toInt() : codValue.toInt();
    return cod == 200;
}

bool parseJsonObject(const QByteArray &data, QJsonObject *obj)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    *obj = document.object();
    return true;
}

QString titleCaseWords(const QString &text)
{
    QStringList words = text.simplified().split(' ', Qt::SkipEmptyParts);
    for (QString &word : words) {
        word = word.left(1).toUpper() + word.mid(1);
    }

    return words.join(' ');
}

QString formatTemperatureText(double temperature)
{
    return QStringLiteral("%1°").arg(QString::number(temperature, 'f', 0));
}

QString normalizeWeatherType(const QString &mainCondition)
{
    const QString normalized = mainCondition.trimmed().toLower();
    if (normalized == QStringLiteral("rain") || normalized == QStringLiteral("drizzle")) {
        return QStringLiteral("rain");
    }

    if (normalized == QStringLiteral("snow")) {
        return QStringLiteral("snow");
    }

    if (normalized == QStringLiteral("thunderstorm")) {
        return QStringLiteral("thunderstorm");
    }

    if (normalized == QStringLiteral("clear")) {
        return QStringLiteral("clear");
    }

    if (normalized == QStringLiteral("clouds")) {
        return QStringLiteral("clouds");
    }

    if (normalized == QStringLiteral("mist")
        || normalized == QStringLiteral("smoke")
        || normalized == QStringLiteral("haze")
        || normalized == QStringLiteral("dust")
        || normalized == QStringLiteral("fog")
        || normalized == QStringLiteral("sand")
        || normalized == QStringLiteral("ash")
        || normalized == QStringLiteral("squall")
        || normalized == QStringLiteral("tornado")) {
        return QStringLiteral("atmosphere");
    }

    return QStringLiteral("other");
}

bool extractWeatherDetails(const QJsonObject &obj,
                           QString *mainCondition,
                           QString *description,
                           QString *icon)
{
    const QJsonValue weatherValue = obj.value(QStringLiteral("weather"));
    if (!weatherValue.isArray()) {
        return false;
    }

    const QJsonArray weatherArray = weatherValue.toArray();
    if (weatherArray.isEmpty() || !weatherArray.at(0).isObject()) {
        return false;
    }

    const QJsonObject weatherObject = weatherArray.at(0).toObject();

    if (mainCondition != nullptr) {
        *mainCondition = weatherObject.value(QStringLiteral("main")).toString();
    }

    if (description != nullptr) {
        *description = weatherObject.value(QStringLiteral("description")).toString();
    }

    if (icon != nullptr) {
        *icon = weatherObject.value(QStringLiteral("icon")).toString();
    }

    return true;
}

bool extractMainDetails(const QJsonObject &obj, double *temp, int *humidity)
{
    const QJsonValue mainValue = obj.value(QStringLiteral("main"));
    if (!mainValue.isObject()) {
        return false;
    }

    const QJsonObject mainObject = mainValue.toObject();

    if (temp != nullptr) {
        *temp = mainObject.value(QStringLiteral("temp")).toDouble();
    }

    if (humidity != nullptr) {
        *humidity = mainObject.value(QStringLiteral("humidity")).toInt();
    }

    return true;
}

QString territoryName(QString countryCode)
{
    const QLocale::Territory territory = QLocale::codeToTerritory(countryCode.trimmed());
    if (territory != QLocale::AnyTerritory) {
        return QLocale::territoryToString(territory);
    }

    return countryCode.trimmed();
}

QString buildLocationLabelFromParts(const QString &city, const QString &countryCode)
{
    const QString trimmedCity = city.trimmed();
    const QString country = territoryName(countryCode);

    if (trimmedCity.isEmpty()) {
        return country;
    }

    if (country.isEmpty()) {
        return trimmedCity;
    }

    return trimmedCity + QStringLiteral(", ") + country;
}

QString buildLocationLabel(const QJsonObject &obj)
{
    return buildLocationLabelFromParts(
        obj.value(QStringLiteral("name")).toString(),
        obj.value(QStringLiteral("sys")).toObject().value(QStringLiteral("country")).toString()
    );
}

bool buildTimelineItem(const QJsonValue &value,
                       int timezoneOffsetSeconds,
                       QVariantMap *timelineItem)
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject item = value.toObject();
    const qint64 eventSecs = item.value(QStringLiteral("dt")).toVariant().toLongLong();
    if (eventSecs <= 0) {
        return false;
    }

    double temperature = 0.0;
    int humidity = 0;
    QString mainCondition;
    QString condition;
    QString icon;
    if (!extractMainDetails(item, &temperature, &humidity)
        || !extractWeatherDetails(item, &mainCondition, &condition, &icon)) {
        return false;
    }

    const QDateTime eventUtc = QDateTime::fromSecsSinceEpoch(eventSecs, QTimeZone::UTC);
    const QDateTime locationTime = eventUtc.addSecs(timezoneOffsetSeconds);

    timelineItem->insert(QStringLiteral("eventUtc"), eventUtc);
    timelineItem->insert(QStringLiteral("displayDate"), locationTime.toString(QStringLiteral("ddd, MMM d")));
    timelineItem->insert(QStringLiteral("displayTime"), locationTime.toString(QStringLiteral("h:mm AP")));
    timelineItem->insert(QStringLiteral("day"), locationTime.toString(QStringLiteral("ddd")));
    timelineItem->insert(QStringLiteral("time"), locationTime.toString(QStringLiteral("h:mm AP")));
    timelineItem->insert(QStringLiteral("tempText"), formatTemperatureText(temperature));
    timelineItem->insert(QStringLiteral("humidityText"), QString::number(humidity) + QStringLiteral("%"));
    timelineItem->insert(QStringLiteral("icon"), icon);
    timelineItem->insert(QStringLiteral("condition"), titleCaseWords(condition));
    timelineItem->insert(QStringLiteral("weatherType"), normalizeWeatherType(mainCondition));
    timelineItem->insert(QStringLiteral("temperature"), temperature);

    return true;
}

}

WeatherAPI::WeatherAPI(QObject *parent)
    : QObject(parent)
{
}

void WeatherAPI::fetchWeather(QString city)
{
    city = city.trimmed();
    if (city.isEmpty()) {
        emit weatherError(noDataFoundMessage());
        return;
    }

    const QString url =
        QStringLiteral("https://api.openweathermap.org/data/2.5/weather?q=")
        + QString(QUrl::toPercentEncoding(city))
        + QStringLiteral("&appid=") + apiKey
        + QStringLiteral("&units=metric");

    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit weatherError(noDataFoundMessage());
            reply->deleteLater();
            return;
        }

        const QByteArray data = reply->readAll();

        QJsonObject obj;
        if (!parseJsonObject(data, &obj) || !hasSuccessCode(obj)) {
            emit weatherError(noDataFoundMessage());
            reply->deleteLater();
            return;
        }

        QString condition;
        QString icon;
        const QString locationLabel = buildLocationLabel(obj);
        double temp = 0.0;
        int humidity = 0;

        if (!extractWeatherDetails(obj, nullptr, &condition, &icon)
            || !extractMainDetails(obj, &temp, &humidity)
            || !obj.value(QStringLiteral("wind")).isObject()) {
            emit weatherError(noDataFoundMessage());
            reply->deleteLater();
            return;
        }

        const QJsonObject windObject = obj.value(QStringLiteral("wind")).toObject();
        const double wind = windObject.value(QStringLiteral("speed")).toDouble();

        double precip = 0.0;
        if (obj.value(QStringLiteral("rain")).isObject()) {
            precip = obj.value(QStringLiteral("rain")).toObject().value(QStringLiteral("1h")).toDouble();
        } else if (obj.value(QStringLiteral("snow")).isObject()) {
            precip = obj.value(QStringLiteral("snow")).toObject().value(QStringLiteral("1h")).toDouble();
        }

        emit weatherReady(
            locationLabel,
            condition,
            temp,
            humidity,
            wind,
            icon,
            precip
        );

        reply->deleteLater();
    });
}

void WeatherAPI::fetchForecast(QString city)
{
    city = city.trimmed();
    if (city.isEmpty()) {
        emit weatherError(noDataFoundMessage());
        return;
    }

    const QString url =
        QStringLiteral("https://api.openweathermap.org/data/2.5/forecast?q=")
        + QString(QUrl::toPercentEncoding(city))
        + QStringLiteral("&appid=") + apiKey
        + QStringLiteral("&units=metric");

    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit weatherError(noDataFoundMessage());
            reply->deleteLater();
            return;
        }

        const QByteArray data = reply->readAll();

        QJsonObject obj;
        if (!parseJsonObject(data, &obj) || !hasSuccessCode(obj)) {
            emit weatherError(noDataFoundMessage());
            reply->deleteLater();
            return;
        }

        const QJsonValue listValue = obj.value(QStringLiteral("list"));
        if (!listValue.isArray()) {
            emit weatherError(noDataFoundMessage());
            reply->deleteLater();
            return;
        }

        const QJsonArray list = listValue.toArray();
        if (list.isEmpty()) {
            emit weatherError(noDataFoundMessage());
            reply->deleteLater();
            return;
        }

        const QJsonObject cityObject = obj.value(QStringLiteral("city")).toObject();
        const QString locationLabel = buildLocationLabelFromParts(
            cityObject.value(QStringLiteral("name")).toString(),
            cityObject.value(QStringLiteral("country")).toString()
        );
        const int timezoneOffsetSeconds = cityObject.value(QStringLiteral("timezone")).toInt();

        QList<QVariantMap> timeline;
        for (const QJsonValue &forecastValue : list) {
            QVariantMap slot;
            if (buildTimelineItem(forecastValue, timezoneOffsetSeconds, &slot)) {
                timeline.append(slot);
            }
        }

        QList<QVariantMap> forecast;
        for (int i = 0; i < timeline.size() && forecast.size() < 5; i += 8) {
            const QVariantMap slot = timeline.at(i);
            QVariantMap card;
            card.insert(QStringLiteral("day"), slot.value(QStringLiteral("day")));
            card.insert(QStringLiteral("temp"), slot.value(QStringLiteral("tempText")));
            card.insert(QStringLiteral("humidity"), slot.value(QStringLiteral("humidityText")));
            card.insert(QStringLiteral("icon"), slot.value(QStringLiteral("icon")));
            forecast.append(card);
        }

        QList<QVariantMap> hourly;
        const int hourlyCount = std::min(5, static_cast<int>(timeline.size()));
        for (int i = 0; i < hourlyCount; ++i) {
            const QVariantMap slot = timeline.at(i);
            QVariantMap row;
            row.insert(QStringLiteral("time"), slot.value(QStringLiteral("time")));
            row.insert(QStringLiteral("temp"), slot.value(QStringLiteral("tempText")));
            row.insert(QStringLiteral("icon"), slot.value(QStringLiteral("icon")));
            hourly.append(row);
        }

        if (timeline.isEmpty()) {
            emit weatherError(noDataFoundMessage());
            reply->deleteLater();
            return;
        }

        emit hourlyReady(hourly);
        emit forecastReady(forecast);
        emit forecastTimelineReady(locationLabel, timeline);

        reply->deleteLater();
    });
}

QByteArray WeatherAPI::downloadIcon(QString url)
{
    if (url.trimmed().isEmpty()) {
        return {};
    }

    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray data = reply->readAll();

    reply->deleteLater();

    return data;
}
