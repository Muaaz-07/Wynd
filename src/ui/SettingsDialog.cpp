#include "SettingsDialog.h"

#include "../app/AppSettings.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QUuid>

#include <algorithm>

namespace {

QString titleCaseWords(const QString &text)
{
    QString normalized = text.toLower();
    bool capitalizeNext = true;

    for (int i = 0; i < normalized.size(); ++i) {
        const QChar character = normalized.at(i);
        if (capitalizeNext && character.isLetter()) {
            normalized[i] = character.toUpper();
            capitalizeNext = false;
            continue;
        }

        if (character.isLetterOrNumber()) {
            capitalizeNext = false;
        } else if (character.isSpace()
                   || character == QLatin1Char('-')
                   || character == QLatin1Char(',')
                   || character == QLatin1Char('/')) {
            capitalizeNext = true;
        }
    }

    return normalized;
}

QString normalizedCityLabel(const QString &text)
{
    const QString simplified = text.simplified();
    if (simplified.isEmpty()) {
        return simplified;
    }

    return simplified == simplified.toLower()
        ? titleCaseWords(simplified)
        : simplified;
}

QList<int> sortedLeadTimes(QList<int> leadTimes)
{
    std::sort(leadTimes.begin(), leadTimes.end(), std::greater<int>());
    return leadTimes;
}

bool rulesEquivalent(const WeatherAlertRule &lhs, const WeatherAlertRule &rhs)
{
    return lhs.name.trimmed() == rhs.name.trimmed()
        && lhs.weatherType == rhs.weatherType
        && lhs.enabled == rhs.enabled
        && lhs.useMinTemp == rhs.useMinTemp
        && lhs.minTemp == rhs.minTemp
        && lhs.useMaxTemp == rhs.useMaxTemp
        && lhs.maxTemp == rhs.maxTemp
        && sortedLeadTimes(lhs.leadTimesHours) == sortedLeadTimes(rhs.leadTimesHours);
}

}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setProperty("themeRole", "settingsDialog");

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    scrollArea->setProperty("themeRole", "settingsScrollArea");
    pageLayout->addWidget(scrollArea);

    QWidget *scrollContentWidget = new QWidget;
    scrollContentWidget->setAttribute(Qt::WA_StyledBackground, true);
    scrollContentWidget->setProperty("themeRole", "settingsDialog");
    scrollArea->setWidget(scrollContentWidget);

    QHBoxLayout *scrollContentLayout = new QHBoxLayout(scrollContentWidget);
    scrollContentLayout->setContentsMargins(0, 0, 0, 0);
    scrollContentLayout->setSpacing(0);
    scrollContentLayout->addStretch();

    QWidget *contentWidget = new QWidget;
    contentWidget->setAttribute(Qt::WA_StyledBackground, true);
    contentWidget->setProperty("themeRole", "settingsDialog");
    contentWidget->setMinimumWidth(900);
    contentWidget->setMaximumWidth(760);
    scrollContentLayout->addWidget(contentWidget);
    scrollContentLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(24);

    QPushButton *backButton = new QPushButton;
    backButton->setProperty("themeRole", "backButton");
    backButton->setText(QString::fromUtf8("\xE2\x86\x90"));
    backButton->setToolTip(QStringLiteral("Back"));
    backButton->setFixedSize(50, 30);

    QLabel *titleLabel = new QLabel(QStringLiteral("Favorites, alerts, and reminders"));
    titleLabel->setProperty("themeRole", "settingsTitle");

    QLabel *introLabel = new QLabel(
        QStringLiteral("Save favorite cities for quick access on the dashboard, create alert rules for weather types and temperature ranges, and decide how Wynd stays available in the background.")
    );
    introLabel->setProperty("themeRole", "settingsHint");
    introLabel->setWordWrap(true);

    alertsEnabledCheck = new QCheckBox(QStringLiteral("Enable weather reminders"));
    alertsEnabledCheck->setProperty("themeRole", "settingsToggle");

    QLabel *backgroundHint = new QLabel(
        QStringLiteral("Keep Wynd in the system tray so reminders continue even when the window is hidden. Startup launches directly into the tray.")
    );
    backgroundHint->setProperty("themeRole", "settingsHint");
    backgroundHint->setWordWrap(true);

    keepRunningInTrayCheck =
        new QCheckBox(QStringLiteral("Keep Wynd running in the system tray when minimized or closed"));
    keepRunningInTrayCheck->setProperty("themeRole", "settingsToggle");

    startOnLoginCheck = new QCheckBox(QStringLiteral("Start Wynd when I sign in"));
    startOnLoginCheck->setProperty("themeRole", "settingsToggle");

    mainLayout->addWidget(backButton, 0, Qt::AlignLeft);
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(introLabel);
    mainLayout->addWidget(alertsEnabledCheck);
    mainLayout->addWidget(backgroundHint);
    mainLayout->addWidget(keepRunningInTrayCheck);
    mainLayout->addWidget(startOnLoginCheck);

    QWidget *favoritesPanel = new QWidget;
    favoritesPanel->setAttribute(Qt::WA_StyledBackground, true);
    favoritesPanel->setProperty("themeRole", "settingsCard");

    QVBoxLayout *favoritesLayout = new QVBoxLayout(favoritesPanel);
    favoritesLayout->setContentsMargins(20, 20, 20, 20);
    favoritesLayout->setSpacing(14);

    QLabel *favoritesTitle = new QLabel(QStringLiteral("Favorite cities"));
    favoritesTitle->setProperty("themeRole", "settingsSectionTitle");

    QLabel *favoritesHint = new QLabel(
        QStringLiteral("Save up to %1 cities. They will appear as clickable pills on the dashboard so you can switch weather views instantly.")
            .arg(AppSettings::MaxFavorites)
    );
    favoritesHint->setProperty("themeRole", "settingsHint");
    favoritesHint->setWordWrap(true);

    favoriteCityEdit = new QLineEdit;
    favoriteCityEdit->setProperty("themeRole", "settingsField");
    favoriteCityEdit->setPlaceholderText(QStringLiteral("Add a city, like Karachi or Paris"));
    favoriteCityEdit->setMinimumHeight(42);
    favoriteCityEdit->setMinimumWidth(280);

    addFavoriteCityButton = new QPushButton(QStringLiteral("Add city"));
    addFavoriteCityButton->setProperty("themeRole", "secondaryButton");
    addFavoriteCityButton->setMinimumHeight(42);

    QHBoxLayout *favoriteInputLayout = new QHBoxLayout;
    favoriteInputLayout->setContentsMargins(0, 0, 0, 0);
    favoriteInputLayout->setSpacing(10);
    favoriteInputLayout->addWidget(favoriteCityEdit, 1);
    favoriteInputLayout->addWidget(addFavoriteCityButton);

    favoriteCityList = new QListWidget;
    favoriteCityList->setProperty("themeRole", "settingsList");
    favoriteCityList->setSelectionMode(QAbstractItemView::SingleSelection);
    favoriteCityList->setMinimumHeight(180);

    removeFavoriteCityButton = new QPushButton(QStringLiteral("Remove selected"));
    removeFavoriteCityButton->setProperty("themeRole", "secondaryButton");
    removeFavoriteCityButton->setMinimumHeight(40);

    favoritesLayout->addWidget(favoritesTitle);
    favoritesLayout->addWidget(favoritesHint);
    favoritesLayout->addLayout(favoriteInputLayout);
    favoritesLayout->addWidget(favoriteCityList);
    favoritesLayout->addWidget(removeFavoriteCityButton, 0, Qt::AlignLeft);

    mainLayout->addWidget(favoritesPanel);

    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(16);

    QWidget *listPanel = new QWidget;
    listPanel->setAttribute(Qt::WA_StyledBackground, true);
    listPanel->setProperty("themeRole", "settingsCard");

    QVBoxLayout *listLayout = new QVBoxLayout(listPanel);
    listLayout->setContentsMargins(20, 20, 20, 20);
    listLayout->setSpacing(14);

    QLabel *savedTitle = new QLabel(QStringLiteral("Saved alerts"));
    savedTitle->setProperty("themeRole", "settingsSectionTitle");

    QLabel *savedHint = new QLabel(
        QStringLiteral("Rules match the active city's 5-day forecast and are checked every minute while Wynd is running, including from the tray.")
    );
    savedHint->setProperty("themeRole", "settingsHint");
    savedHint->setWordWrap(true);

    ruleList = new QListWidget;
    ruleList->setProperty("themeRole", "settingsList");
    ruleList->setSelectionMode(QAbstractItemView::SingleSelection);
    ruleList->setWordWrap(true);
    ruleList->setTextElideMode(Qt::ElideNone);
    ruleList->setUniformItemSizes(false);
    ruleList->setMinimumHeight(280);

    QPushButton *newRuleButton = new QPushButton(QStringLiteral("New alert"));
    newRuleButton->setProperty("themeRole", "secondaryButton");
    newRuleButton->setMinimumHeight(40);

    removeRuleButton = new QPushButton(QStringLiteral("Remove"));
    removeRuleButton->setProperty("themeRole", "dangerButton");
    removeRuleButton->setMinimumHeight(40);

    QHBoxLayout *listButtonsLayout = new QHBoxLayout;
    listButtonsLayout->setSpacing(10);
    listButtonsLayout->addWidget(newRuleButton);
    listButtonsLayout->addWidget(removeRuleButton);

    listLayout->addWidget(savedTitle);
    listLayout->addWidget(savedHint);
    listLayout->addWidget(ruleList, 1);
    listLayout->addLayout(listButtonsLayout);

    QWidget *editorPanel = new QWidget;
    editorPanel->setAttribute(Qt::WA_StyledBackground, true);
    editorPanel->setProperty("themeRole", "settingsCard");

    QVBoxLayout *editorLayout = new QVBoxLayout(editorPanel);
    editorLayout->setContentsMargins(20, 20, 20, 20);
    editorLayout->setSpacing(14);

    QLabel *editorTitle = new QLabel(QStringLiteral("Rule editor"));
    editorTitle->setProperty("themeRole", "settingsSectionTitle");

    QLabel *editorHint = new QLabel(
        QStringLiteral("Use a weather type, a temperature range, or combine both. Pick one or more reminder times before the forecast slot.")
    );
    editorHint->setProperty("themeRole", "settingsHint");
    editorHint->setWordWrap(true);

    nameEdit = new QLineEdit;
    nameEdit->setProperty("themeRole", "settingsField");
    nameEdit->setPlaceholderText(QStringLiteral("Optional name, like Commute rain alert"));
    nameEdit->setMinimumHeight(42);
    nameEdit->setMinimumWidth(320);

    weatherTypeCombo = new QComboBox;
    weatherTypeCombo->setProperty("themeRole", "settingsField");
    weatherTypeCombo->setMinimumHeight(42);
    weatherTypeCombo->setMinimumWidth(320);
    for (const auto &option : WeatherAlertManager::availableWeatherTypes()) {
        weatherTypeCombo->addItem(option.second, option.first);
    }

    minTempCheck = new QCheckBox(QStringLiteral("Minimum"));
    minTempCheck->setProperty("themeRole", "settingsToggle");
    minTempSpin = new QDoubleSpinBox;
    minTempSpin->setProperty("themeRole", "settingsField");
    minTempSpin->setMinimumHeight(42);
    minTempSpin->setMinimumWidth(110);
    minTempSpin->setRange(-50.0, 60.0);
    minTempSpin->setDecimals(0);
    minTempSpin->setSuffix(QStringLiteral("°C"));

    maxTempCheck = new QCheckBox(QStringLiteral("Maximum"));
    maxTempCheck->setProperty("themeRole", "settingsToggle");
    maxTempSpin = new QDoubleSpinBox;
    maxTempSpin->setProperty("themeRole", "settingsField");
    maxTempSpin->setMinimumHeight(42);
    maxTempSpin->setMinimumWidth(110);
    maxTempSpin->setRange(-50.0, 60.0);
    maxTempSpin->setDecimals(0);
    maxTempSpin->setSuffix(QStringLiteral("°C"));

    QWidget *tempWidget = new QWidget;
    QHBoxLayout *tempLayout = new QHBoxLayout(tempWidget);
    tempLayout->setContentsMargins(0, 0, 0, 0);
    tempLayout->setSpacing(10);
    tempLayout->addWidget(minTempCheck);
    tempLayout->addWidget(minTempSpin);
    tempLayout->addSpacing(12);
    tempLayout->addWidget(maxTempCheck);
    tempLayout->addWidget(maxTempSpin);
    tempLayout->addStretch();

    QWidget *leadTimesWidget = new QWidget;
    QGridLayout *leadTimesLayout = new QGridLayout(leadTimesWidget);
    leadTimesLayout->setContentsMargins(0, 0, 0, 0);
    leadTimesLayout->setHorizontalSpacing(18);
    leadTimesLayout->setVerticalSpacing(15);

    int leadColumn = 0;
    int leadRow = 0;
    for (int leadHours : WeatherAlertManager::availableLeadTimesHours()) {
        QCheckBox *leadCheck = new QCheckBox(WeatherAlertManager::leadTimeLabel(leadHours));
        leadCheck->setProperty("themeRole", "settingsToggle");
        leadTimeChecks.insert(leadHours, leadCheck);
        leadTimesLayout->addWidget(leadCheck, leadRow, leadColumn);

        ++leadColumn;
        if (leadColumn == 2) {
            leadColumn = 0;
            ++leadRow;
        }
    }

    ruleEnabledCheck = new QCheckBox(QStringLiteral("Rule enabled"));
    ruleEnabledCheck->setProperty("themeRole", "settingsToggle");

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(14);
    formLayout->setHorizontalSpacing(14);
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    formLayout->addRow(QStringLiteral("Name"), nameEdit);
    formLayout->addRow(QStringLiteral("Weather type"), weatherTypeCombo);
    formLayout->addRow(QStringLiteral("Temperature range"), tempWidget);
    formLayout->addRow(QStringLiteral("Remind me"), leadTimesWidget);
    formLayout->addRow(QStringLiteral("Status"), ruleEnabledCheck);

    saveRuleButton = new QPushButton(QStringLiteral("Add alert"));
    saveRuleButton->setProperty("themeRole", "primaryButton");
    saveRuleButton->setMinimumHeight(42);

    QPushButton *saveSettingsButton = new QPushButton(QStringLiteral("Save settings"));
    saveSettingsButton->setProperty("themeRole", "primaryButton");
    saveSettingsButton->setMinimumHeight(44);
    saveSettingsButton->setMinimumWidth(160);

    editorLayout->addWidget(editorTitle);
    editorLayout->addWidget(editorHint);
    editorLayout->addLayout(formLayout);
    editorLayout->addWidget(saveRuleButton, 0, Qt::AlignRight);

    contentLayout->addWidget(listPanel,1);
    contentLayout->addWidget(editorPanel,1);
    mainLayout->addLayout(contentLayout,1);
    mainLayout->addWidget(saveSettingsButton, 0, Qt::AlignRight);

    minTempSpin->setEnabled(false);
    maxTempSpin->setEnabled(false);

    connect(backButton,
            &QPushButton::clicked,
            this,
            &SettingsDialog::backRequested);
    connect(favoriteCityEdit,
            &QLineEdit::returnPressed,
            this,
            &SettingsDialog::addFavoriteCity);
    connect(addFavoriteCityButton,
            &QPushButton::clicked,
            this,
            &SettingsDialog::addFavoriteCity);
    connect(removeFavoriteCityButton,
            &QPushButton::clicked,
            this,
            &SettingsDialog::removeSelectedFavoriteCity);
    connect(favoriteCityList,
            &QListWidget::currentRowChanged,
            this,
            [this]() {
                updateActionButtons();
            });
    connect(minTempCheck,
            &QCheckBox::toggled,
            minTempSpin,
            &QWidget::setEnabled);
    connect(maxTempCheck,
            &QCheckBox::toggled,
            maxTempSpin,
            &QWidget::setEnabled);
    connect(ruleList,
            &QListWidget::currentRowChanged,
            this,
            &SettingsDialog::handleSelectionChanged);
    connect(saveRuleButton,
            &QPushButton::clicked,
            this,
            &SettingsDialog::saveRule);
    connect(removeRuleButton,
            &QPushButton::clicked,
            this,
            &SettingsDialog::removeSelectedRule);
    connect(newRuleButton,
            &QPushButton::clicked,
            this,
            &SettingsDialog::createNewRule);
    connect(saveSettingsButton,
            &QPushButton::clicked,
            this,
            &SettingsDialog::saveSettingsPage);

    clearEditor();
}

void SettingsDialog::loadSettings(const QStringList &favoriteCities,
                                  const QList<WeatherAlertRule> &rules,
                                  bool alertsEnabled,
                                  bool keepRunningInTray,
                                  bool startOnLogin)
{
    m_favoriteCities = favoriteCities;
    m_rules = rules;
    alertsEnabledCheck->setChecked(alertsEnabled);
    keepRunningInTrayCheck->setChecked(keepRunningInTray);
    startOnLoginCheck->setChecked(startOnLogin);
    favoriteCityEdit->clear();
    refreshFavoriteCityList();
    refreshRuleList();
    clearEditor();
}

QStringList SettingsDialog::favoriteCities() const
{
    return m_favoriteCities;
}

QList<WeatherAlertRule> SettingsDialog::rules() const
{
    return m_rules;
}

bool SettingsDialog::alertsEnabled() const
{
    return alertsEnabledCheck->isChecked();
}

bool SettingsDialog::keepRunningInTray() const
{
    return keepRunningInTrayCheck->isChecked();
}

bool SettingsDialog::startOnLogin() const
{
    return startOnLoginCheck->isChecked();
}

void SettingsDialog::addFavoriteCity()
{
    const QString city = normalizedFavoriteCity(favoriteCityEdit->text());
    if (city.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("Missing city"),
                             QStringLiteral("Enter a city name before adding it to favorites."));
        return;
    }

    if (containsFavoriteCity(city)) {
        QMessageBox::warning(this,
                             QStringLiteral("Already saved"),
                             QStringLiteral("That city is already in your favorites."));
        return;
    }

    if (m_favoriteCities.size() >= AppSettings::MaxFavorites) {
        QMessageBox::warning(this,
                             QStringLiteral("Favorites full"),
                             QStringLiteral("You can save up to %1 favorite cities.")
                                 .arg(AppSettings::MaxFavorites));
        return;
    }

    m_favoriteCities.append(city);
    favoriteCityEdit->clear();
    refreshFavoriteCityList();
    favoriteCityList->setCurrentRow(favoriteCityList->count() - 1);
}

void SettingsDialog::removeSelectedFavoriteCity()
{
    const int currentRow = favoriteCityList != nullptr ? favoriteCityList->currentRow() : -1;
    if (currentRow < 0 || currentRow >= m_favoriteCities.size()) {
        return;
    }

    m_favoriteCities.removeAt(currentRow);
    refreshFavoriteCityList();
}

void SettingsDialog::saveRule()
{
    saveCurrentEditorToRules();
}

void SettingsDialog::removeSelectedRule()
{
    if (currentRuleId.isEmpty()) {
        return;
    }

    for (int i = 0; i < m_rules.size(); ++i) {
        if (m_rules.at(i).id == currentRuleId) {
            m_rules.removeAt(i);
            break;
        }
    }

    refreshRuleList();
    clearEditor();
}

void SettingsDialog::createNewRule()
{
    ruleList->clearSelection();
    clearEditor();
}

void SettingsDialog::handleSelectionChanged()
{
    const int row = ruleList->currentRow();
    if (row < 0 || row >= m_rules.size()) {
        return;
    }

    loadRuleIntoEditor(m_rules.at(row));
}

void SettingsDialog::saveSettingsPage()
{
    if (hasPendingEditorChanges() && !saveCurrentEditorToRules()) {
        return;
    }

    emit settingsSaved(m_favoriteCities,
                       m_rules,
                       alertsEnabledCheck->isChecked(),
                       keepRunningInTrayCheck->isChecked(),
                       startOnLoginCheck->isChecked());
}

bool SettingsDialog::containsFavoriteCity(const QString &city) const
{
    const QString cityKey = city.simplified().toCaseFolded();
    for (const QString &favoriteCity : m_favoriteCities) {
        if (favoriteCity.simplified().toCaseFolded() == cityKey) {
            return true;
        }
    }

    return false;
}

QString SettingsDialog::normalizedFavoriteCity(const QString &city) const
{
    return normalizedCityLabel(city);
}

void SettingsDialog::refreshFavoriteCityList()
{
    const int previousRow = favoriteCityList != nullptr ? favoriteCityList->currentRow() : -1;
    QSignalBlocker blocker(favoriteCityList);
    favoriteCityList->clear();

    for (const QString &city : m_favoriteCities) {
        favoriteCityList->addItem(city);
    }

    if (favoriteCityList->count() == 0) {
        favoriteCityList->setCurrentItem(nullptr);
        favoriteCityList->clearSelection();
    } else if (previousRow < 0) {
        favoriteCityList->setCurrentItem(nullptr);
        favoriteCityList->clearSelection();
    } else {
        favoriteCityList->setCurrentRow(qBound(0, previousRow, favoriteCityList->count() - 1));
    }

    updateActionButtons();
}

WeatherAlertRule SettingsDialog::currentRuleFromEditor() const
{
    WeatherAlertRule rule;
    rule.id = currentRuleId;
    rule.name = nameEdit->text().trimmed();
    rule.weatherType = weatherTypeCombo->currentData().toString();
    rule.enabled = ruleEnabledCheck->isChecked();
    rule.useMinTemp = minTempCheck->isChecked();
    rule.minTemp = minTempSpin->value();
    rule.useMaxTemp = maxTempCheck->isChecked();
    rule.maxTemp = maxTempSpin->value();

    for (int leadHours : WeatherAlertManager::availableLeadTimesHours()) {
        QCheckBox *leadCheck = leadTimeChecks.value(leadHours, nullptr);
        if (leadCheck != nullptr && leadCheck->isChecked()) {
            rule.leadTimesHours.append(leadHours);
        }
    }

    return rule;
}

bool SettingsDialog::validateRule(const WeatherAlertRule &rule,
                                  QString *errorMessage) const
{
    if (rule.leadTimesHours.isEmpty()) {
        *errorMessage = QStringLiteral("Pick at least one reminder time.");
        return false;
    }

    if (rule.weatherType == QStringLiteral("any")
        && !rule.useMinTemp
        && !rule.useMaxTemp) {
        *errorMessage =
            QStringLiteral("Choose a weather type, a temperature range, or both.");
        return false;
    }

    if (rule.useMinTemp && rule.useMaxTemp && rule.minTemp > rule.maxTemp) {
        *errorMessage =
            QStringLiteral("Minimum temperature cannot be higher than maximum temperature.");
        return false;
    }

    return true;
}

bool SettingsDialog::saveCurrentEditorToRules()
{
    WeatherAlertRule rule = currentRuleFromEditor();

    QString errorMessage;
    if (!validateRule(rule, &errorMessage)) {
        QMessageBox::warning(this,
                             QStringLiteral("Incomplete alert"),
                             errorMessage);
        return false;
    }

    if (rule.id.isEmpty()) {
        rule.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    bool updatedExistingRule = false;
    for (int i = 0; i < m_rules.size(); ++i) {
        if (m_rules.at(i).id == rule.id) {
            m_rules[i] = rule;
            updatedExistingRule = true;
            break;
        }
    }

    if (!updatedExistingRule) {
        m_rules.append(rule);
    }

    currentRuleId = rule.id;
    refreshRuleList();

    for (int i = 0; i < ruleList->count(); ++i) {
        if (ruleList->item(i)->data(Qt::UserRole).toString() == currentRuleId) {
            ruleList->setCurrentRow(i);
            break;
        }
    }

    updateActionButtons();
    return true;
}

bool SettingsDialog::editorHasMeaningfulData() const
{
    if (!nameEdit->text().trimmed().isEmpty()) {
        return true;
    }

    if (weatherTypeCombo->currentData().toString() != QStringLiteral("any")) {
        return true;
    }

    if (minTempCheck->isChecked() || maxTempCheck->isChecked()) {
        return true;
    }

    for (QCheckBox *leadCheck : leadTimeChecks) {
        if (leadCheck != nullptr && leadCheck->isChecked()) {
            return true;
        }
    }

    return !ruleEnabledCheck->isChecked();
}

bool SettingsDialog::hasPendingEditorChanges() const
{
    if (!editorHasMeaningfulData()) {
        return false;
    }

    if (currentRuleId.isEmpty()) {
        return true;
    }

    const WeatherAlertRule currentRule = currentRuleFromEditor();
    for (const WeatherAlertRule &rule : m_rules) {
        if (rule.id == currentRuleId) {
            return !rulesEquivalent(currentRule, rule);
        }
    }

    return true;
}

void SettingsDialog::refreshRuleList()
{
    const QString selectedId = currentRuleId;
    QSignalBlocker blocker(ruleList);
    ruleList->clear();

    for (const WeatherAlertRule &rule : m_rules) {
        QListWidgetItem *item = new QListWidgetItem(ruleList);
        const QString title = rule.enabled
            ? WeatherAlertManager::ruleDisplayName(rule)
            : QStringLiteral("[Paused] ") + WeatherAlertManager::ruleDisplayName(rule);
        item->setText(title + QStringLiteral("\n") + WeatherAlertManager::ruleSummary(rule));
        item->setToolTip(WeatherAlertManager::ruleSummary(rule));
        item->setData(Qt::UserRole, rule.id);
    }

    if (!selectedId.isEmpty()) {
        for (int i = 0; i < ruleList->count(); ++i) {
            if (ruleList->item(i)->data(Qt::UserRole).toString() == selectedId) {
                ruleList->setCurrentRow(i);
                break;
            }
        }
    }
}

void SettingsDialog::loadRuleIntoEditor(const WeatherAlertRule &rule)
{
    currentRuleId = rule.id;

    const QSignalBlocker minBlocker(minTempCheck);
    const QSignalBlocker maxBlocker(maxTempCheck);

    nameEdit->setText(rule.name);

    for (int i = 0; i < weatherTypeCombo->count(); ++i) {
        if (weatherTypeCombo->itemData(i).toString() == rule.weatherType) {
            weatherTypeCombo->setCurrentIndex(i);
            break;
        }
    }

    minTempCheck->setChecked(rule.useMinTemp);
    minTempSpin->setEnabled(rule.useMinTemp);
    minTempSpin->setValue(rule.minTemp);

    maxTempCheck->setChecked(rule.useMaxTemp);
    maxTempSpin->setEnabled(rule.useMaxTemp);
    maxTempSpin->setValue(rule.maxTemp);

    for (int leadHours : WeatherAlertManager::availableLeadTimesHours()) {
        if (QCheckBox *leadCheck = leadTimeChecks.value(leadHours, nullptr)) {
            leadCheck->setChecked(rule.leadTimesHours.contains(leadHours));
        }
    }

    ruleEnabledCheck->setChecked(rule.enabled);
    updateActionButtons();
}

void SettingsDialog::clearEditor()
{
    currentRuleId.clear();
    nameEdit->clear();
    weatherTypeCombo->setCurrentIndex(0);
    minTempCheck->setChecked(false);
    minTempSpin->setEnabled(false);
    minTempSpin->setValue(0.0);
    maxTempCheck->setChecked(false);
    maxTempSpin->setEnabled(false);
    maxTempSpin->setValue(0.0);

    for (QCheckBox *leadCheck : leadTimeChecks) {
        if (leadCheck != nullptr) {
            leadCheck->setChecked(false);
        }
    }

    ruleEnabledCheck->setChecked(true);
    updateActionButtons();
}

void SettingsDialog::updateActionButtons()
{
    saveRuleButton->setText(currentRuleId.isEmpty()
                                ? QStringLiteral("Add alert")
                                : QStringLiteral("Update alert"));
    removeRuleButton->setEnabled(!currentRuleId.isEmpty());

    const bool canAddFavoriteCity = m_favoriteCities.size() < AppSettings::MaxFavorites;
    addFavoriteCityButton->setEnabled(canAddFavoriteCity);
    favoriteCityEdit->setEnabled(canAddFavoriteCity);
    favoriteCityEdit->setPlaceholderText(
        canAddFavoriteCity
            ? QStringLiteral("Add a city, like Karachi or Paris")
            : QStringLiteral("Favorite city limit reached")
    );
    removeFavoriteCityButton->setEnabled(favoriteCityList->currentRow() >= 0);
}
