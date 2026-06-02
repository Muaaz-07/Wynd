#include "ForecastCard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QVBoxLayout>

// Builds a compact forecast card with async icon loading.
ForecastCard::ForecastCard(
    QString day,
    QString temp,
    QString humidity,
    QString icon
) : QWidget()
{
    setAttribute(Qt::WA_StyledBackground, true);
    setProperty("themeRole", "forecastCard");

    QVBoxLayout *mainLayout = new QVBoxLayout;

    dayLabel = new QLabel(day);
    dayLabel->setProperty("themeRole", "forecastDay");
    dayLabel->setAlignment(Qt::AlignCenter);

    iconLabel = new QLabel("...");
    iconLabel->setProperty("themeRole", "forecastIcon");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(58, 58);

    if (icon.trimmed().isEmpty()) {
        iconLabel->setText("--");
    } else {
        // Forecast icons are loaded separately so the card can render immediately.
        const QString iconUrl =
            "https://openweathermap.org/img/wn/" +
            icon +
            "@2x.png";

        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkRequest request((QUrl(iconUrl)));

        QObject::connect(manager, &QNetworkAccessManager::finished, iconLabel, [this](QNetworkReply *reply) {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pix;
                pix.loadFromData(reply->readAll());
                iconLabel->setPixmap(pix.scaled(34, 34, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                iconLabel->setText("--");
            }
            reply->deleteLater();
        });

        manager->get(request);
    }

    QHBoxLayout *bottomLayout = new QHBoxLayout;

    tempLabel = new QLabel(temp);
    tempLabel->setProperty("themeRole", "forecastTemp");
    tempLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    humidityLabel = new QLabel(humidity);
    humidityLabel->setProperty("themeRole", "forecastHumidity");
    humidityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    bottomLayout->addWidget(tempLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(humidityLabel);

    mainLayout->addWidget(dayLabel);
    mainLayout->addWidget(iconLabel, 0, Qt::AlignHCenter);
    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);
    setMinimumWidth(120);
    setMaximumWidth(170);
    setMinimumHeight(160);
}
