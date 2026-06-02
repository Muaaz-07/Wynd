#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QMap>
#include <QStringList>
#include <QWidget>

#include "../alerts/WeatherAlertManager.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QListWidget;
class QPushButton;

class SettingsDialog : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    void loadSettings(const QStringList &favoriteCities,
                      const QList<WeatherAlertRule> &rules,
                      bool alertsEnabled,
                      bool keepRunningInTray,
                      bool startOnLogin);
    QStringList favoriteCities() const;
    QList<WeatherAlertRule> rules() const;
    bool alertsEnabled() const;
    bool keepRunningInTray() const;
    bool startOnLogin() const;

signals:
    void backRequested();
    void settingsSaved(QStringList favoriteCities,
                       QList<WeatherAlertRule> rules,
                       bool alertsEnabled,
                       bool keepRunningInTray,
                       bool startOnLogin);

private slots:
    void addFavoriteCity();
    void removeSelectedFavoriteCity();
    void saveRule();
    void removeSelectedRule();
    void createNewRule();
    void handleSelectionChanged();
    void saveSettingsPage();

private:
    bool containsFavoriteCity(const QString &city) const;
    QString normalizedFavoriteCity(const QString &city) const;
    void refreshFavoriteCityList();
    WeatherAlertRule currentRuleFromEditor() const;
    bool validateRule(const WeatherAlertRule &rule, QString *errorMessage) const;
    bool saveCurrentEditorToRules();
    bool editorHasMeaningfulData() const;
    bool hasPendingEditorChanges() const;
    void refreshRuleList();
    void loadRuleIntoEditor(const WeatherAlertRule &rule);
    void clearEditor();
    void updateActionButtons();

    QStringList m_favoriteCities;
    QList<WeatherAlertRule> m_rules;

    QCheckBox *alertsEnabledCheck = nullptr;
    QCheckBox *keepRunningInTrayCheck = nullptr;
    QCheckBox *startOnLoginCheck = nullptr;
    QListWidget *favoriteCityList = nullptr;
    QListWidget *ruleList = nullptr;
    QLineEdit *favoriteCityEdit = nullptr;
    QLineEdit *nameEdit = nullptr;
    QComboBox *weatherTypeCombo = nullptr;
    QCheckBox *minTempCheck = nullptr;
    QDoubleSpinBox *minTempSpin = nullptr;
    QCheckBox *maxTempCheck = nullptr;
    QDoubleSpinBox *maxTempSpin = nullptr;
    QCheckBox *ruleEnabledCheck = nullptr;
    QPushButton *addFavoriteCityButton = nullptr;
    QPushButton *removeFavoriteCityButton = nullptr;
    QPushButton *saveRuleButton = nullptr;
    QPushButton *removeRuleButton = nullptr;
    QMap<int, QCheckBox *> leadTimeChecks;
    QString currentRuleId;
};

#endif
