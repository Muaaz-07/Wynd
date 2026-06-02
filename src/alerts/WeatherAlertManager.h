#ifndef WEATHERALERTMANAGER_H
#define WEATHERALERTMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QVariantMap>

class QSystemTrayIcon;
class QTimer;

struct WeatherAlertRule
{
    QString id;
    QString name;
    QString weatherType = QStringLiteral("any");
    bool enabled = true;
    bool useMinTemp = false;
    double minTemp = 0.0;
    bool useMaxTemp = false;
    double maxTemp = 0.0;
    QList<int> leadTimesHours;
};

class WeatherAlertManager : public QObject
{
    Q_OBJECT

public:
    explicit WeatherAlertManager(QObject *parent = nullptr);

    QList<WeatherAlertRule> rules() const;
    void setRules(const QList<WeatherAlertRule> &rules);

    bool alertsEnabled() const;
    void setAlertsEnabled(bool enabled);
    QSystemTrayIcon *trayIcon() const;

    static QList<QPair<QString, QString>> availableWeatherTypes();
    static QList<int> availableLeadTimesHours();
    static QString weatherTypeLabel(const QString &weatherType);
    static QString leadTimeLabel(int leadHours);
    static QString ruleDisplayName(const WeatherAlertRule &rule);
    static QString ruleSummary(const WeatherAlertRule &rule);

public slots:
    void updateForecastTimeline(QString locationLabel, QList<QVariantMap> timeline);

signals:
    void alertRaised(QString title, QString message);

private:
    void loadSettings();
    void saveSettings() const;
    void pruneDeliveredReminders();
    void evaluateDueReminders();
    bool ruleMatchesSlot(const WeatherAlertRule &rule, const QVariantMap &slot) const;
    QString buildReminderId(const WeatherAlertRule &rule,
                            const QVariantMap &slot,
                            int leadHours) const;
    QString buildReminderMessage(const WeatherAlertRule &rule,
                                 const QVariantMap &slot,
                                 int leadHours) const;
    QString ruleSignature(const WeatherAlertRule &rule) const;

    QList<WeatherAlertRule> m_rules;
    bool m_alertsEnabled = true;
    QString m_locationLabel;
    QList<QVariantMap> m_timeline;
    QMap<QString, qint64> m_deliveredReminderExpiries;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QTimer *m_checkTimer = nullptr;
};

#endif
