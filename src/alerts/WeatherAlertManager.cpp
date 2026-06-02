#include "WeatherAlertManager.h"

#include "../app/AppSettings.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStringList>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>

#include <algorithm>

namespace {

constexpr int kReminderGracePeriodSeconds = 90 * 60;
constexpr int kReminderExpiryPaddingSeconds = 12 * 60 * 60;

QString titleCaseWords(const QString &text)
{
    QStringList words = text.simplified().split(' ', Qt::SkipEmptyParts);
    for (QString &word : words) {
        word = word.left(1).toUpper() + word.mid(1);
    }

    return words.join(' ');
}

QString formatTemperatureNumber(double temperature)
{
    return QString::number(temperature, 'f', 0);
}

QString temperatureCriteriaText(const WeatherAlertRule &rule)
{
    if (rule.useMinTemp && rule.useMaxTemp) {
        return QStringLiteral("temperature %1°C to %2°C")
            .arg(formatTemperatureNumber(rule.minTemp))
            .arg(formatTemperatureNumber(rule.maxTemp));
    }

    if (rule.useMinTemp) {
        return QStringLiteral("temperature at or above %1°C")
            .arg(formatTemperatureNumber(rule.minTemp));
    }

    if (rule.useMaxTemp) {
        return QStringLiteral("temperature at or below %1°C")
            .arg(formatTemperatureNumber(rule.maxTemp));
    }

    return QString();
}

QString capitalizeFirstWord(QString text)
{
    for (int i = 0; i < text.size(); ++i) {
        if (!text.at(i).isSpace()) {
            text[i] = text.at(i).toUpper();
            break;
        }
    }

    return text;
}

QString relativeLeadText(int leadHours)
{
    if (leadHours <= 0) {
        return QStringLiteral("now");
    }

    if (leadHours == 24) {
        return QStringLiteral("in 1 day");
    }

    return QStringLiteral("in %1 hours").arg(leadHours);
}

QJsonObject ruleToJson(const WeatherAlertRule &rule)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), rule.id);
    obj.insert(QStringLiteral("name"), rule.name.trimmed());
    obj.insert(QStringLiteral("weatherType"), rule.weatherType);
    obj.insert(QStringLiteral("enabled"), rule.enabled);
    obj.insert(QStringLiteral("useMinTemp"), rule.useMinTemp);
    obj.insert(QStringLiteral("minTemp"), rule.minTemp);
    obj.insert(QStringLiteral("useMaxTemp"), rule.useMaxTemp);
    obj.insert(QStringLiteral("maxTemp"), rule.maxTemp);

    QJsonArray leadTimesArray;
    for (int leadHours : rule.leadTimesHours) {
        leadTimesArray.append(leadHours);
    }
    obj.insert(QStringLiteral("leadTimesHours"), leadTimesArray);

    return obj;
}

WeatherAlertRule ruleFromJson(const QJsonObject &obj)
{
    WeatherAlertRule rule;
    rule.id = obj.value(QStringLiteral("id")).toString();
    rule.name = obj.value(QStringLiteral("name")).toString();
    rule.weatherType = obj.value(QStringLiteral("weatherType")).toString(QStringLiteral("any"));
    rule.enabled = obj.value(QStringLiteral("enabled")).toBool(true);
    rule.useMinTemp = obj.value(QStringLiteral("useMinTemp")).toBool(false);
    rule.minTemp = obj.value(QStringLiteral("minTemp")).toDouble();
    rule.useMaxTemp = obj.value(QStringLiteral("useMaxTemp")).toBool(false);
    rule.maxTemp = obj.value(QStringLiteral("maxTemp")).toDouble();

    const QJsonArray leadTimesArray = obj.value(QStringLiteral("leadTimesHours")).toArray();
    for (const QJsonValue &leadValue : leadTimesArray) {
        rule.leadTimesHours.append(leadValue.toInt());
    }

    return rule;
}

}

WeatherAlertManager::WeatherAlertManager(QObject *parent)
    : QObject(parent)
{
    loadSettings();

    m_trayIcon = new QSystemTrayIcon(this);
    QIcon trayIcon = QApplication::windowIcon();
    if (trayIcon.isNull()) {
        trayIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }

    m_trayIcon->setIcon(trayIcon);
    m_trayIcon->setToolTip(QStringLiteral("Wynd weather reminders"));
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon->show();
    }

    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(60 * 1000);
    connect(m_checkTimer,
            &QTimer::timeout,
            this,
            &WeatherAlertManager::evaluateDueReminders);
    m_checkTimer->start();
}

QList<WeatherAlertRule> WeatherAlertManager::rules() const
{
    return m_rules;
}

void WeatherAlertManager::setRules(const QList<WeatherAlertRule> &rules)
{
    m_rules = rules;
    saveSettings();
    evaluateDueReminders();
}

bool WeatherAlertManager::alertsEnabled() const
{
    return m_alertsEnabled;
}

void WeatherAlertManager::setAlertsEnabled(bool enabled)
{
    m_alertsEnabled = enabled;
    saveSettings();
    evaluateDueReminders();
}

QSystemTrayIcon *WeatherAlertManager::trayIcon() const
{
    return m_trayIcon;
}

QList<QPair<QString, QString>> WeatherAlertManager::availableWeatherTypes()
{
    return {
        {QStringLiteral("any"), QStringLiteral("Any condition")},
        {QStringLiteral("rain"), QStringLiteral("Rain or drizzle")},
        {QStringLiteral("snow"), QStringLiteral("Snow")},
        {QStringLiteral("thunderstorm"), QStringLiteral("Thunderstorm")},
        {QStringLiteral("clear"), QStringLiteral("Clear skies")},
        {QStringLiteral("clouds"), QStringLiteral("Cloudy")},
        {QStringLiteral("atmosphere"), QStringLiteral("Fog, mist or haze")}
    };
}

QList<int> WeatherAlertManager::availableLeadTimesHours()
{
    return {24, 12, 6, 3, 0};
}

QString WeatherAlertManager::weatherTypeLabel(const QString &weatherType)
{
    for (const auto &option : availableWeatherTypes()) {
        if (option.first == weatherType) {
            return option.second;
        }
    }

    return QStringLiteral("Weather");
}

QString WeatherAlertManager::leadTimeLabel(int leadHours)
{
    if (leadHours == 24) {
        return QStringLiteral("1 day before");
    }

    if (leadHours <= 0) {
        return QStringLiteral("At forecast time");
    }

    return QStringLiteral("%1 hours before").arg(leadHours);
}

QString WeatherAlertManager::ruleDisplayName(const WeatherAlertRule &rule)
{
    const QString trimmedName = rule.name.trimmed();
    if (!trimmedName.isEmpty()) {
        return trimmedName;
    }

    if (rule.weatherType != QStringLiteral("any")) {
        if (rule.useMinTemp || rule.useMaxTemp) {
            return weatherTypeLabel(rule.weatherType) + QStringLiteral(" temperature alert");
        }

        return weatherTypeLabel(rule.weatherType) + QStringLiteral(" alert");
    }

    if (rule.useMinTemp || rule.useMaxTemp) {
        return QStringLiteral("Temperature alert");
    }

    return QStringLiteral("Weather alert");
}

QString WeatherAlertManager::ruleSummary(const WeatherAlertRule &rule)
{
    QStringList matchParts;
    if (rule.weatherType != QStringLiteral("any")) {
        matchParts.append(weatherTypeLabel(rule.weatherType));
    }

    const QString temperatureText = temperatureCriteriaText(rule);
    if (!temperatureText.isEmpty()) {
        matchParts.append(temperatureText);
    }

    if (matchParts.isEmpty()) {
        matchParts.append(QStringLiteral("Any condition"));
    }

    QStringList reminderParts;
    for (int leadHours : availableLeadTimesHours()) {
        if (rule.leadTimesHours.contains(leadHours)) {
            reminderParts.append(leadTimeLabel(leadHours));
        }
    }

    if (reminderParts.isEmpty()) {
        reminderParts.append(QStringLiteral("No reminders"));
    }

    const QString disabledSuffix = rule.enabled
        ? QString()
        : QStringLiteral(" Disabled.");

    const QString description = capitalizeFirstWord(matchParts.join(QStringLiteral(", ")));

    return QStringLiteral("%1.\nReminders: %2.%3")
        .arg(description)
        .arg(reminderParts.join(QStringLiteral(", ")))
        .arg(disabledSuffix);
}

void WeatherAlertManager::updateForecastTimeline(QString locationLabel,
                                                 QList<QVariantMap> timeline)
{
    m_locationLabel = locationLabel.trimmed();
    m_timeline = timeline;
    pruneDeliveredReminders();
    evaluateDueReminders();
}

void WeatherAlertManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::AlertsGroup));

    m_alertsEnabled = settings.value(QString::fromLatin1(AppSettings::AlertsEnabledKey), true).toBool();

    const QJsonDocument rulesDocument =
        QJsonDocument::fromJson(settings.value(QString::fromLatin1(AppSettings::AlertRulesKey)).toByteArray());
    if (rulesDocument.isArray()) {
        const QJsonArray rulesArray = rulesDocument.array();
        for (const QJsonValue &ruleValue : rulesArray) {
            if (!ruleValue.isObject()) {
                continue;
            }

            m_rules.append(ruleFromJson(ruleValue.toObject()));
        }
    }

    const QJsonDocument deliveredDocument =
        QJsonDocument::fromJson(
            settings.value(QString::fromLatin1(AppSettings::DeliveredReminderExpiriesKey)).toByteArray()
        );
    if (deliveredDocument.isObject()) {
        const QJsonObject deliveredObject = deliveredDocument.object();
        for (auto it = deliveredObject.begin(); it != deliveredObject.end(); ++it) {
            m_deliveredReminderExpiries.insert(it.key(),
                                               static_cast<qint64>(it.value().toDouble()));
        }
    }

    settings.endGroup();

    pruneDeliveredReminders();
}

void WeatherAlertManager::saveSettings() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::AlertsGroup));
    settings.setValue(QString::fromLatin1(AppSettings::AlertsEnabledKey), m_alertsEnabled);

    QJsonArray rulesArray;
    for (const WeatherAlertRule &rule : m_rules) {
        rulesArray.append(ruleToJson(rule));
    }
    settings.setValue(QString::fromLatin1(AppSettings::AlertRulesKey),
                      QJsonDocument(rulesArray).toJson(QJsonDocument::Compact));

    QJsonObject deliveredObject;
    for (auto it = m_deliveredReminderExpiries.begin();
         it != m_deliveredReminderExpiries.end();
         ++it) {
        deliveredObject.insert(it.key(), static_cast<double>(it.value()));
    }
    settings.setValue(QString::fromLatin1(AppSettings::DeliveredReminderExpiriesKey),
                      QJsonDocument(deliveredObject).toJson(QJsonDocument::Compact));

    settings.endGroup();
}

void WeatherAlertManager::pruneDeliveredReminders()
{
    const qint64 nowSecs = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    QStringList expiredIds;

    for (auto it = m_deliveredReminderExpiries.begin();
         it != m_deliveredReminderExpiries.end();
         ++it) {
        if (it.value() < nowSecs) {
            expiredIds.append(it.key());
        }
    }

    for (const QString &id : expiredIds) {
        m_deliveredReminderExpiries.remove(id);
    }

    if (!expiredIds.isEmpty()) {
        saveSettings();
    }
}

void WeatherAlertManager::evaluateDueReminders()
{
    if (!m_alertsEnabled || m_rules.isEmpty() || m_timeline.isEmpty()) {
        return;
    }

    pruneDeliveredReminders();

    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    bool settingsChanged = false;

    struct PendingReminder
    {
        QString id;
        QString title;
        QString message;
        qint64 expirySecs = 0;
        qint64 reminderSecs = 0;
    };

    QList<PendingReminder> dueReminders;

    for (const WeatherAlertRule &rule : m_rules) {
        if (!rule.enabled) {
            continue;
        }

        for (const QVariantMap &slot : m_timeline) {
            if (!ruleMatchesSlot(rule, slot)) {
                continue;
            }

            const QDateTime eventUtc = slot.value(QStringLiteral("eventUtc")).toDateTime();
            if (!eventUtc.isValid()) {
                continue;
            }

            for (int leadHours : rule.leadTimesHours) {
                const QDateTime reminderUtc = eventUtc.addSecs(-leadHours * 60 * 60);
                if (nowUtc < reminderUtc
                    || nowUtc > eventUtc.addSecs(kReminderGracePeriodSeconds)) {
                    continue;
                }

                const QString reminderId = buildReminderId(rule, slot, leadHours);
                if (m_deliveredReminderExpiries.contains(reminderId)) {
                    continue;
                }

                PendingReminder reminder;
                reminder.id = reminderId;
                reminder.title = ruleDisplayName(rule);
                reminder.message = buildReminderMessage(rule, slot, leadHours);
                reminder.expirySecs =
                    eventUtc.addSecs(kReminderExpiryPaddingSeconds).toSecsSinceEpoch();
                reminder.reminderSecs = reminderUtc.toSecsSinceEpoch();
                dueReminders.append(reminder);
            }
        }
    }

    std::sort(dueReminders.begin(), dueReminders.end(),
              [](const PendingReminder &lhs, const PendingReminder &rhs) {
                  return lhs.reminderSecs < rhs.reminderSecs;
              });

    const int reminderLimit = std::min(4, static_cast<int>(dueReminders.size()));
    for (int i = 0; i < reminderLimit; ++i) {
        const PendingReminder &reminder = dueReminders.at(i);
        emit alertRaised(reminder.title, reminder.message);

        if (m_trayIcon != nullptr && m_trayIcon->isVisible()) {
            m_trayIcon->showMessage(reminder.title,
                                    reminder.message,
                                    QSystemTrayIcon::Information,
                                    12000);
        }

        m_deliveredReminderExpiries.insert(reminder.id, reminder.expirySecs);
        settingsChanged = true;
    }

    if (settingsChanged) {
        saveSettings();
    }
}

bool WeatherAlertManager::ruleMatchesSlot(const WeatherAlertRule &rule,
                                          const QVariantMap &slot) const
{
    if (rule.weatherType != QStringLiteral("any")) {
        const QString slotWeatherType = slot.value(QStringLiteral("weatherType")).toString();
        if (slotWeatherType != rule.weatherType) {
            return false;
        }
    }

    const double temperature = slot.value(QStringLiteral("temperature")).toDouble();
    if (rule.useMinTemp && temperature < rule.minTemp) {
        return false;
    }

    if (rule.useMaxTemp && temperature > rule.maxTemp) {
        return false;
    }

    return true;
}

QString WeatherAlertManager::buildReminderId(const WeatherAlertRule &rule,
                                             const QVariantMap &slot,
                                             int leadHours) const
{
    const QDateTime eventUtc = slot.value(QStringLiteral("eventUtc")).toDateTime();
    return QStringLiteral("%1|%2|%3|%4")
        .arg(rule.id)
        .arg(ruleSignature(rule))
        .arg(eventUtc.toSecsSinceEpoch())
        .arg(leadHours);
}

QString WeatherAlertManager::buildReminderMessage(const WeatherAlertRule &rule,
                                                  const QVariantMap &slot,
                                                  int leadHours) const
{
    const QString condition = titleCaseWords(slot.value(QStringLiteral("condition")).toString());
    const QString location = m_locationLabel.isEmpty()
        ? QStringLiteral("the current location")
        : m_locationLabel;
    const QString displayDate = slot.value(QStringLiteral("displayDate")).toString();
    const QString displayTime = slot.value(QStringLiteral("displayTime")).toString();
    const QString temperature = slot.value(QStringLiteral("tempText")).toString();

    QString headline;
    if (rule.weatherType == QStringLiteral("any")) {
        headline = QStringLiteral("A matching temperature");
    } else if (rule.useMinTemp || rule.useMaxTemp) {
        headline = condition.isEmpty()
            ? weatherTypeLabel(rule.weatherType) + QStringLiteral(" with a matching temperature")
            : condition + QStringLiteral(" with a matching temperature");
    } else {
        headline = condition.isEmpty()
            ? weatherTypeLabel(rule.weatherType)
            : condition;
    }

    QString message = QStringLiteral("%1 is forecast %2 for %3 at %4 in %5. Temperature: %6.")
        .arg(headline)
        .arg(relativeLeadText(leadHours))
        .arg(displayDate)
        .arg(displayTime)
        .arg(location)
        .arg(temperature.isEmpty() ? QStringLiteral("-- C") : temperature);

    if (rule.weatherType == QStringLiteral("any") && !condition.isEmpty()) {
        message += QStringLiteral(" Condition: %1.").arg(condition);
    }

    return message;
}

QString WeatherAlertManager::ruleSignature(const WeatherAlertRule &rule) const
{
    QList<int> leadTimes = rule.leadTimesHours;
    std::sort(leadTimes.begin(), leadTimes.end(), std::greater<int>());

    QStringList leadParts;
    for (int leadHours : leadTimes) {
        leadParts.append(QString::number(leadHours));
    }

    const QString serialized = QStringLiteral("%1|%2|%3|%4|%5|%6|%7")
        .arg(rule.weatherType)
        .arg(rule.useMinTemp ? QString::number(rule.minTemp, 'f', 2) : QStringLiteral("-"))
        .arg(rule.useMaxTemp ? QString::number(rule.maxTemp, 'f', 2) : QStringLiteral("-"))
        .arg(rule.enabled ? QStringLiteral("1") : QStringLiteral("0"))
        .arg(leadParts.join(QStringLiteral(",")))
        .arg(rule.name.trimmed())
        .arg(rule.id);

    return QString::fromLatin1(
        QCryptographicHash::hash(serialized.toUtf8(), QCryptographicHash::Sha1).toHex()
    );
}
