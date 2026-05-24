//
// Created by Muhammad Muaaz on 3/11/2026.
//

#include "MainWindow.h"

#include "../app/AppSettings.h"
#include "../alerts/WeatherAlertManager.h"
#include "../widgets/CurvedSurfaceWidget.h"
#include "../widgets/ForecastCard.h"
#include "SettingsDialog.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QEasingCurve>
#include <QEvent>
#include <QFile>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPauseAnimation>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QSequentialAnimationGroup>
#include <QSettings>
#include <QStackedWidget>
#include <QStyleHints>
#include <QStringList>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollBar>

#include <functional>

namespace {

QString titleCaseWords(const QString &text)
{
    QStringList words = text.simplified().split(' ', Qt::SkipEmptyParts);
    for (QString &word : words) {
        word = word.left(1).toUpper() + word.mid(1);
    }

    return words.join(' ');
}

QString normalizedCityQuery(const QString &text)
{
    return text.simplified();
}

QString favoriteCityComparisonKey(const QString &text)
{
    return normalizedCityQuery(text).toCaseFolded();
}

QString favoriteCityLabel(const QString &text)
{
    const QString normalizedText = normalizedCityQuery(text);
    if (normalizedText.isEmpty()) {
        return normalizedText;
    }

    return normalizedText == normalizedText.toLower()
        ? titleCaseWords(normalizedText)
        : normalizedText;
}

QStringList sanitizedFavoriteCities(const QStringList &cities)
{
    QStringList result;
    QSet<QString> seenCities;

    for (const QString &city : cities) {
        const QString formattedCity = favoriteCityLabel(city);
        const QString cityKey = favoriteCityComparisonKey(formattedCity);
        if (cityKey.isEmpty() || seenCities.contains(cityKey)) {
            continue;
        }

        seenCities.insert(cityKey);
        result.append(formattedCity);
        if (result.size() >= AppSettings::MaxFavorites) {
            break;
        }
    }

    return result;
}

class FavoriteCityPillButton : public QPushButton
{
public:
    explicit FavoriteCityPillButton(const QString &text, QWidget *parent = nullptr)
        : QPushButton(text, parent)
    {
        setCursor(Qt::PointingHandCursor);
        setCheckable(true);
        setFlat(true);
    }

    void setDarkThemeEnabled(bool enabled)
    {
        if (m_darkThemeEnabled == enabled) {
            return;
        }

        m_darkThemeEnabled = enabled;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QRectF shadowRect = rect().adjusted(2.0, 8.0, -2.0, -2.0);
        QRectF pillRect = rect().adjusted(1.0, 1.0, -1.0, -8.0);

        const bool hovered = underMouse();
        const QColor shadowColor = m_darkThemeEnabled
            ? QColor(4, 20, 28, isChecked() ? 120 : 84)
            : QColor(11, 88, 116, isChecked() ? 52 : 34);

        painter.setPen(Qt::NoPen);
        painter.setBrush(shadowColor);
        painter.drawRoundedRect(shadowRect, 18.0, 18.0);

        QColor fillColor;
        QColor borderColor;
        QColor textColor;

        if (m_darkThemeEnabled) {
            fillColor = isChecked()
                ? QColor(239, 248, 250)
                : hovered ? QColor(20, 86, 104) : QColor(18, 56, 68);
            borderColor = isChecked()
                ? QColor(239, 248, 250)
                : hovered ? QColor(115, 215, 231, 140) : QColor(130, 202, 219, 70);
            textColor = isChecked() ? QColor(10, 95, 124) : QColor(239, 248, 250);
        } else {
            fillColor = isChecked()
                ? QColor(13, 120, 149)
                : hovered ? QColor(242, 251, 253) : QColor(255, 255, 255);
            borderColor = isChecked()
                ? QColor(24, 182, 200, 60)
                : hovered ? QColor(125, 211, 252) : QColor(219, 228, 236);
            textColor = isChecked() ? QColor(255, 255, 255) : QColor(11, 88, 116);
        }

        painter.setPen(QPen(borderColor, 1.0));
        painter.setBrush(fillColor);
        painter.drawRoundedRect(pillRect, 18.0, 18.0);

        QFont textFont = font();
        textFont.setWeight(QFont::DemiBold);
        painter.setFont(textFont);
        painter.setPen(textColor);
        painter.drawText(pillRect, Qt::AlignCenter, text());
    }

private:
    bool m_darkThemeEnabled = true;
};

QString formatTemperatureText(double temperature)
{
    return QStringLiteral("%1°C").arg(QString::number(temperature, 'f', 0));
}

QString startupRegistryPath()
{
    return QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
}

QString startupValueName()
{
    const QString applicationName = QCoreApplication::applicationName().trimmed();
    return applicationName.isEmpty() ? QStringLiteral("Wynd") : applicationName;
}

QString startupCommand()
{
    const QString executablePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    return QStringLiteral("\"%1\" --tray").arg(executablePath);
}

bool isStartupEnabledInWindows()
{
#ifdef Q_OS_WIN
    QSettings startupSettings(startupRegistryPath(), QSettings::NativeFormat);
    return !startupSettings.value(startupValueName()).toString().trimmed().isEmpty();
#else
    return false;
#endif
}

void setStartupEnabledInWindows(bool enabled)
{
#ifdef Q_OS_WIN
    QSettings startupSettings(startupRegistryPath(), QSettings::NativeFormat);
    if (enabled) {
        startupSettings.setValue(startupValueName(), startupCommand());
        return;
    }

    startupSettings.remove(startupValueName());
#else
    Q_UNUSED(enabled);
#endif
}

void animateOpacitySwap(QObject *owner,
                        QGraphicsOpacityEffect *effect,
                        int fadeOutDuration,
                        int pauseDuration,
                        int fadeInDuration,
                        qreal fadeOutTarget,
                        QEasingCurve fadeOutCurve,
                        QEasingCurve fadeInCurve,
                        const std::function<void()> &midpointAction,
                        const std::function<void()> &finishedAction = {})
{
    QSequentialAnimationGroup *sequence = new QSequentialAnimationGroup(owner);

    QPropertyAnimation *fadeOut = new QPropertyAnimation(effect, "opacity", sequence);
    fadeOut->setDuration(fadeOutDuration);
    fadeOut->setStartValue(effect->opacity());
    fadeOut->setEndValue(fadeOutTarget);
    fadeOut->setEasingCurve(fadeOutCurve);

    sequence->addAnimation(fadeOut);
    if (pauseDuration > 0) {
        sequence->addAnimation(new QPauseAnimation(pauseDuration, sequence));
    }

    QPropertyAnimation *fadeIn = new QPropertyAnimation(effect, "opacity", sequence);
    fadeIn->setDuration(fadeInDuration);
    fadeIn->setStartValue(fadeOutTarget);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(fadeInCurve);
    sequence->addAnimation(fadeIn);

    QObject::connect(fadeOut, &QPropertyAnimation::finished, owner, [midpointAction]() {
        midpointAction();
    });

    if (finishedAction) {
        QObject::connect(sequence, &QSequentialAnimationGroup::finished, owner, [finishedAction]() {
            finishedAction();
        });
    }

    QObject::connect(sequence,
                     &QSequentialAnimationGroup::finished,
                     sequence,
                     &QObject::deleteLater);
    sequence->start();
}

}

MainWindow::MainWindow(bool startHiddenInTrayOnLaunch, QWidget *parent)
    : QMainWindow(parent)
{
    Q_UNUSED(startHiddenInTrayOnLaunch);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

    centralWidgetRoot = new QWidget;
    centralWidgetRoot->setObjectName("centralWidgetRoot");
    centralWidgetRoot->setAttribute(Qt::WA_StyledBackground, true);
    setCentralWidget(centralWidgetRoot);

    contentOpacityEffect = new QGraphicsOpacityEffect(this);
    contentOpacityEffect->setOpacity(1.0);
    centralWidgetRoot->setGraphicsEffect(contentOpacityEffect);

    QVBoxLayout *rootLayout = new QVBoxLayout(centralWidgetRoot);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    windowTitleBar = new QWidget;
    windowTitleBar->setAttribute(Qt::WA_StyledBackground, true);
    windowTitleBar->setProperty("themeRole", "windowTitleBar");
    windowTitleBar->setFixedHeight(38);

    QHBoxLayout *windowTitleLayout = new QHBoxLayout(windowTitleBar);
    windowTitleLayout->setContentsMargins(12, 0, 8, 0);
    windowTitleLayout->setSpacing(8);

    windowTitleLabel = new QLabel(QStringLiteral("Wynd"));
    windowTitleLabel->setProperty("themeRole", "windowTitleText");

    minimizeButton = new QPushButton(QStringLiteral("–"));
    minimizeButton->setProperty("themeRole", "windowControlButton");
    minimizeButton->setFixedSize(36, 28);

    maximizeButton = new QPushButton(QStringLiteral("□"));
    maximizeButton->setProperty("themeRole", "windowControlButton");
    maximizeButton->setFixedSize(36, 28);

    closeButton = new QPushButton(QStringLiteral("×"));
    closeButton->setProperty("themeRole", "windowCloseButton");
    closeButton->setFixedSize(36, 28);

    windowTitleLayout->addWidget(windowTitleLabel);
    windowTitleLayout->addStretch();
    windowTitleLayout->addWidget(minimizeButton);
    windowTitleLayout->addWidget(maximizeButton);
    windowTitleLayout->addWidget(closeButton);

    rootLayout->addWidget(windowTitleBar);

    pageStack = new QStackedWidget;
    rootLayout->addWidget(pageStack);

    QScrollArea *dashboardScrollArea = new QScrollArea;
    dashboardScrollArea->setWidgetResizable(true);
    dashboardScrollArea->setFrameShape(QFrame::NoFrame);
    dashboardScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    dashboardScrollArea->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    dashboardScrollArea->setProperty("themeRole", "dashboardScrollArea");
    dashboardPage = dashboardScrollArea;

    QWidget *dashboardContentWidget = new QWidget;
    dashboardContentWidget->setAttribute(Qt::WA_StyledBackground, true);
    dashboardContentWidget->setProperty("themeRole", "dashboardPage");
    dashboardContentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    dashboardScrollArea->setWidget(dashboardContentWidget);

    settingsPage = new SettingsDialog;
    settingsPage->setAttribute(Qt::WA_StyledBackground, true);

    pageStack->addWidget(dashboardPage);
    pageStack->addWidget(settingsPage);
    pageStack->setCurrentWidget(dashboardPage);

    backgroundLabel = new QLabel(this);
    backgroundLabel->setObjectName("backgroundLabel");
    backgroundLabel->setGeometry(0, 0, 420, 350);
    backgroundLabel->setAlignment(Qt::AlignCenter);
    backgroundLabel->setFixedSize(150, 150);

    titleLabel = new QLabel("How's the sky looking today?");
    titleLabel->setProperty("themeRole", "appTitle");
    titleLabel->setAlignment(Qt::AlignHCenter);

    cityInput = new QLineEdit;
    cityInput->setProperty("themeRole", "searchField");
    cityInput->setPlaceholderText("Search for a place...");
    cityInput->setFixedWidth(300);

    searchButton = new QPushButton("Search");
    searchButton->setProperty("themeRole", "primaryButton");
    searchButton->setFixedWidth(100);

    currentCityLabel = new QLabel("Detecting City...");
    currentCityLabel->setProperty("themeRole", "cityLabel");

    subtitleLabel = new QLabel(currentDateLabelText());
    subtitleLabel->setProperty("themeRole", "cityDate");

    tempLabel = new QLabel("--°C");
    tempLabel->setProperty("themeRole", "temperatureLabel");

    conditionLabel = new QLabel("-");
    conditionLabel->setProperty("themeRole", "conditionValue");
    conditionLabel->setAlignment(Qt::AlignLeft);
    conditionLabel->setWordWrap(true);

    forecastLayout = new QHBoxLayout;
    forecastLayout->setSpacing(16);

    QVBoxLayout *mainLayout = new QVBoxLayout(dashboardContentWidget);
    mainLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(12);

    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->setSpacing(8);
    searchLayout->addStretch();
    searchLayout->addWidget(cityInput);
    searchLayout->addWidget(searchButton);
    searchLayout->addStretch();

    QVBoxLayout *topLayout = new QVBoxLayout;
    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    settingsButton = new QPushButton("Settings");
    settingsButton->setProperty("themeRole", "iconButton");
    settingsButton->setText(QString::fromUtf8("\xE2\x9A\x99"));
    settingsButton->setToolTip(QStringLiteral("Settings"));
    settingsButton->setFixedSize(36, 36);
    settingsButton->setCursor(Qt::PointingHandCursor);

    themeButton = new QPushButton;
    themeButton->setProperty("themeRole", "iconButton");
    themeButton->setFixedSize(36, 36);
    themeButton->setCursor(Qt::PointingHandCursor);

    themeButtonOpacityEffect = new QGraphicsOpacityEffect(this);
    themeButtonOpacityEffect->setOpacity(1.0);
    themeButton->setGraphicsEffect(themeButtonOpacityEffect);

    headerLayout->addWidget(settingsButton);
    headerLayout->addWidget(themeButton);

    topLayout->addLayout(headerLayout);
    topLayout->addSpacing(15);
    topLayout->addLayout(searchLayout);
    topLayout->addSpacing(20);

    mainLayout->addLayout(topLayout);

    QHBoxLayout *splitLayout = new QHBoxLayout;
    splitLayout->setSpacing(20);

    leftColumnLayout = new QVBoxLayout;
    leftColumnLayout->setSpacing(0);

    blueCard = new QWidget;
    blueCard->setAttribute(Qt::WA_StyledBackground, true);
    blueCard->setProperty("themeRole", "heroCard");
    blueCard->setMinimumHeight(220);
    blueCard->setMaximumWidth(1100);

    QHBoxLayout *blueCardLayout = new QHBoxLayout(blueCard);
    QVBoxLayout *heroTextLayout = new QVBoxLayout;
    heroTextLayout->setSpacing(6);
    heroTextLayout->addStretch();
    heroTextLayout->addWidget(currentCityLabel);
    heroTextLayout->addWidget(subtitleLabel);
    heroTextLayout->addStretch();

    blueCardLayout->addLayout(heroTextLayout);
    blueCardLayout->addStretch();
    blueCardLayout->addWidget(backgroundLabel);
    blueCardLayout->addWidget(tempLabel);
    leftColumnLayout->addWidget(blueCard);

    leftContentSurface = new CurvedSurfaceWidget;

    QHBoxLayout *statsLayout = new QHBoxLayout;
    statsLayout->setSpacing(20);

    const auto createStatHeader = [](const QString &iconText,
                                     const char *iconKind,
                                     const QString &titleText) {
        QHBoxLayout *headerLayout = new QHBoxLayout;
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(8);

        QLabel *iconLabel = new QLabel(iconText);
        iconLabel->setProperty("themeRole", "statIcon");
        iconLabel->setProperty("iconKind", iconKind);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setFixedSize(30, 30);

        QLabel *titleLabel = new QLabel(titleText);
        titleLabel->setProperty("themeRole", "statHeaderTitle");
        titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        headerLayout->addWidget(iconLabel);
        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();

        return headerLayout;
    };

    card1 = new QWidget;
    card1->setAttribute(Qt::WA_StyledBackground, true);
    card1->setProperty("themeRole", "statCard");
    card1->setMinimumWidth(200);
    card1->setMaximumWidth(300);
    card1->setFixedHeight(120);

    QVBoxLayout *lay1 = new QVBoxLayout(card1);
    lay1->addLayout(createStatHeader(QString::fromUtf8("\xE2\x98\x81"),
                                     "condition",
                                     "Condition"));
    lay1->addWidget(conditionLabel);
    statsLayout->addWidget(card1);

    card2 = new QWidget;
    card2->setAttribute(Qt::WA_StyledBackground, true);
    card2->setProperty("themeRole", "statCard");
    card2->setMinimumWidth(200);
    card2->setMaximumWidth(300);
    card2->setFixedHeight(120);

    QVBoxLayout *lay2 = new QVBoxLayout(card2);
    lay2->addLayout(createStatHeader(QString::fromUtf8("\xF0\x9F\x92\xA7"),
                                     "humidity",
                                     "Humidity"));
    humidityLabel = new QLabel("-- %");
    humidityLabel->setProperty("themeRole", "statValue");
    humidityLabel->setAlignment(Qt::AlignLeft);
    lay2->addWidget(humidityLabel);
    statsLayout->addWidget(card2);

    card3 = new QWidget;
    card3->setAttribute(Qt::WA_StyledBackground, true);
    card3->setProperty("themeRole", "statCard");
    card3->setMinimumWidth(200);
    card3->setMaximumWidth(300);
    card3->setFixedHeight(120);

    QVBoxLayout *lay3 = new QVBoxLayout(card3);
    lay3->addLayout(createStatHeader(QString::fromUtf8("\xF0\x9F\x8D\x83"),
                                     "wind",
                                     "Wind"));
    windLabel = new QLabel("-- km/h");
    windLabel->setProperty("themeRole", "statValue");
    windLabel->setAlignment(Qt::AlignLeft);
    lay3->addWidget(windLabel);
    statsLayout->addWidget(card3);

    card4 = new QWidget;
    card4->setAttribute(Qt::WA_StyledBackground, true);
    card4->setProperty("themeRole", "statCard");
    card4->setMinimumWidth(200);
    card4->setMaximumWidth(300);
    card4->setFixedHeight(120);

    QVBoxLayout *lay4 = new QVBoxLayout(card4);
    lay4->addLayout(createStatHeader(QString::fromUtf8("\xE2\x98\x94"),
                                     "precipitation",
                                     "Precipitation"));
    precipLabel = new QLabel("0 mm");
    precipLabel->setProperty("themeRole", "statValue");
    precipLabel->setAlignment(Qt::AlignLeft);
    lay4->addWidget(precipLabel);
    statsLayout->addWidget(card4);

    QVBoxLayout *leftContentLayout = new QVBoxLayout(leftContentSurface);
    leftContentLayout->setContentsMargins(20, 20, 20, 20);
    leftContentLayout->setSpacing(0);
    favoriteCitiesPanel = new QWidget;
    favoriteCitiesPanel->setAttribute(Qt::WA_StyledBackground, true);
    favoriteCitiesPanel->setProperty("themeRole", "favoriteCitiesPanel");

    QVBoxLayout *favoriteCitiesPanelLayout = new QVBoxLayout(favoriteCitiesPanel);
    favoriteCitiesPanelLayout->setContentsMargins(0, 0, 0, 0);
    favoriteCitiesPanelLayout->setSpacing(10);

    QLabel *favoriteCitiesTitle = new QLabel("Favorite cities");
    favoriteCitiesTitle->setProperty("themeRole", "sectionTitle");

    favoriteCitiesEmptyLabel =
        new QLabel("Add favorite cities from Settings to switch locations in one click.");
    favoriteCitiesEmptyLabel->setProperty("themeRole", "sectionHint");
    favoriteCitiesEmptyLabel->setWordWrap(true);

    favoriteCitiesScrollArea = new QScrollArea;
    favoriteCitiesScrollArea->setWidgetResizable(true);
    favoriteCitiesScrollArea->setFrameShape(QFrame::NoFrame);
    favoriteCitiesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    favoriteCitiesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    favoriteCitiesScrollArea->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    favoriteCitiesScrollArea->setProperty("themeRole", "favoriteCitiesScrollArea");
    favoriteCitiesScrollArea->setFixedHeight(70);
    favoriteCitiesScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    favoriteCitiesPillArea = new QWidget;
    favoriteCitiesPillArea->setAttribute(Qt::WA_StyledBackground, true);
    favoriteCitiesPillArea->setProperty("themeRole", "favoriteCitiesPillArea");
    favoriteCitiesPillArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    favoriteCitiesLayout = new QHBoxLayout(favoriteCitiesPillArea);
    favoriteCitiesLayout->setContentsMargins(2, 6, 8, 10);
    if (width() < 1400) {
        favoriteCitiesLayout->setSpacing(12);
    } else {
        favoriteCitiesLayout->setSpacing(16);
    }
    favoriteCitiesScrollArea->setWidget(favoriteCitiesPillArea);

    favoriteCitiesPanelLayout->addWidget(favoriteCitiesTitle);
    favoriteCitiesPanelLayout->addWidget(favoriteCitiesEmptyLabel);
    favoriteCitiesPanelLayout->addWidget(favoriteCitiesScrollArea);

    leftContentLayout->addWidget(favoriteCitiesPanel);
    leftContentLayout->addSpacing(12);
    leftContentLayout->addLayout(statsLayout);

    QLabel *dailyTitle = new QLabel("Daily forecast");
    dailyTitle->setProperty("themeRole", "sectionTitle");
    leftContentLayout->addWidget(dailyTitle);
    leftContentLayout->addLayout(forecastLayout);

    leftColumnLayout->addWidget(leftContentSurface);

    QVBoxLayout *rightCol = new QVBoxLayout;

    rightCard = new QWidget;
    rightCard->setAttribute(Qt::WA_StyledBackground, true);
    rightCard->setProperty("themeRole", "hourlyCard");
    rightCard->setMinimumWidth(300);
    rightCard->setMaximumWidth(400);

    QVBoxLayout *rightCardLayout = new QVBoxLayout(rightCard);
    QLabel *hourlyTitle = new QLabel("Hourly forecast");
    hourlyTitle->setProperty("themeRole", "hourlySectionTitle");
    rightCardLayout->addWidget(hourlyTitle);

    hourlyLayout = new QVBoxLayout;
    rightCardLayout->addLayout(hourlyLayout);
    rightCol->addWidget(rightCard);

    splitLayout->addLayout(leftColumnLayout, 2);
    splitLayout->addLayout(rightCol, 1);
    mainLayout->addLayout(splitLayout);

    api = new WeatherAPI(this);
    locationService = new LocationService(this);
    alertManager = new WeatherAlertManager(this);
    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(15 * 60 * 1000);

    connect(locationService,
            &LocationService::cityDetected,
            this,
            &MainWindow::autoCityDetected);

    connect(searchButton,
            &QPushButton::clicked,
            this,
            &MainWindow::searchWeather);
    connect(cityInput,
            &QLineEdit::returnPressed,
            this,
            &MainWindow::searchWeather);

    connect(api,
            &WeatherAPI::weatherReady,
            this,
            &MainWindow::displayCurrentWeather);
    connect(api,
            &WeatherAPI::forecastReady,
            this,
            &MainWindow::displayForecast);
    connect(api,
            &WeatherAPI::forecastTimelineReady,
            alertManager,
            &WeatherAlertManager::updateForecastTimeline);
    connect(api,
            &WeatherAPI::hourlyReady,
            this,
            &MainWindow::displayHourly);
    connect(api,
            &WeatherAPI::weatherError,
            this,
            &MainWindow::handleWeatherError);

    connect(themeButton,
            &QPushButton::clicked,
            this,
            &MainWindow::toggleTheme);
    connect(settingsButton,
            &QPushButton::clicked,
            this,
            &MainWindow::openSettings);
    connect(refreshTimer,
            &QTimer::timeout,
            this,
            &MainWindow::refreshActiveCity);
    connect(alertManager,
            &WeatherAlertManager::alertRaised,
            this,
            [this](const QString &title, const QString &message) {
                const QString prefix = title.trimmed().isEmpty()
                    ? QString()
                    : title.trimmed() + QStringLiteral(": ");
                statusBar()->showMessage(prefix + message, 15000);
            });
    connect(settingsPage,
            &SettingsDialog::backRequested,
            this,
            [this]() {
                switchToPage(dashboardPage);
            });
    connect(settingsPage,
            &SettingsDialog::settingsSaved,
            this,
            [this](const QStringList &favoriteCitiesValue,
                   const QList<WeatherAlertRule> &rules,
                   bool alertsEnabled,
                   bool keepRunningInTrayValue,
                   bool startOnLoginValue) {
                favoriteCities = sanitizedFavoriteCities(favoriteCitiesValue);
                saveFavoriteCities();
                rebuildFavoriteCityPills();
                alertManager->setAlertsEnabled(alertsEnabled);
                alertManager->setRules(rules);
                keepRunningInTray = keepRunningInTrayValue;
                startOnLogin = startOnLoginValue;
                saveWindowPreferences();
                syncStartupRegistration();
                switchToPage(dashboardPage);
                statusBar()->showMessage(QStringLiteral("Settings saved."), 5000);
            });

    connect(minimizeButton, &QPushButton::clicked, this, [this]() {
        if (shouldPersistInTray()) {
            hideToTray(true);
            return;
        }

        showMinimized();
    });
    connect(maximizeButton, &QPushButton::clicked, this, [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);

    loadThemePreference();
    loadWindowPreferences();
    loadFavoriteCities();
    setupTrayIntegration();
    syncStartupRegistration();
    applyTheme();
    statusBar()->setSizeGripEnabled(false);
    refreshTimer->start();
    locationService->detectLocation();

    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    setWindowTitle("Wynd");
    resize(1300,800);
}

void MainWindow::applyTheme()
{
    const QString themePath = isDarkTheme
        ? QStringLiteral(":/styles/dark.qss")
        : QStringLiteral(":/styles/light.qss");

    QFile styleFile(themePath);
    if (styleFile.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    } else {
        qApp->setStyleSheet(QString());
    }

    updateThemeButtonIcon();
    subtitleLabel->setText(currentDateLabelText());
    leftContentSurface->setDarkPaletteEnabled(isDarkTheme);
    leftContentSurface->setElevatedCurveEnabled(true);
    leftColumnLayout->setSpacing(isDarkTheme ? 0 : 18);
    updateFavoriteCitiesStripGeometry();
    refreshFavoriteCityPillPresentation();
}

bool MainWindow::supportsTrayMode() const
{
    return trayIcon != nullptr && QSystemTrayIcon::isSystemTrayAvailable();
}

QString MainWindow::currentDateLabelText() const
{
    return QDate::currentDate().toString("dddd, MMM d, yyyy");
}

void MainWindow::updateThemeButtonIcon()
{
    themeButton->setText(
        isDarkTheme
            ? QString::fromUtf8("\xE2\x98\x80")
            : QString::fromUtf8("\xE2\x98\xBE")
    );
    themeButton->setToolTip(
        isDarkTheme
            ? QStringLiteral("Switch to light theme")
            : QStringLiteral("Switch to dark theme")
    );
}

void MainWindow::loadThemePreference()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::AppearanceGroup));
    const QString savedTheme =
        settings.value(QString::fromLatin1(AppSettings::ThemeKey)).toString().trimmed().toLower();
    settings.endGroup();

    if (savedTheme == QString::fromLatin1(AppSettings::DarkThemeValue)) {
        isDarkTheme = true;
        return;
    }

    if (savedTheme == QString::fromLatin1(AppSettings::LightThemeValue)) {
        isDarkTheme = false;
        return;
    }

    isDarkTheme = systemPrefersDarkTheme();
}

void MainWindow::saveThemePreference() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::AppearanceGroup));
    settings.setValue(QString::fromLatin1(AppSettings::ThemeKey),
                      isDarkTheme
                          ? QString::fromLatin1(AppSettings::DarkThemeValue)
                          : QString::fromLatin1(AppSettings::LightThemeValue));
    settings.endGroup();
}

void MainWindow::loadWindowPreferences()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::WindowGroup));
    keepRunningInTray =
        settings.value(QString::fromLatin1(AppSettings::KeepRunningInTrayKey), true).toBool();
    trayHintShown =
        settings.value(QString::fromLatin1(AppSettings::HasShownTrayHintKey), false).toBool();
    startOnLogin =
        settings.contains(QString::fromLatin1(AppSettings::RunOnStartupKey))
            ? settings.value(QString::fromLatin1(AppSettings::RunOnStartupKey), false).toBool()
            : isStartupEnabledInWindows();
    settings.endGroup();
}

void MainWindow::saveWindowPreferences() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::WindowGroup));
    settings.setValue(QString::fromLatin1(AppSettings::KeepRunningInTrayKey), keepRunningInTray);
    settings.setValue(QString::fromLatin1(AppSettings::RunOnStartupKey), startOnLogin);
    settings.setValue(QString::fromLatin1(AppSettings::HasShownTrayHintKey), trayHintShown);
    settings.endGroup();
}

void MainWindow::loadFavoriteCities()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::SearchGroup));
    favoriteCities = sanitizedFavoriteCities(
        settings.value(QString::fromLatin1(AppSettings::FavoritesKey)).toStringList()
    );
    settings.endGroup();
    rebuildFavoriteCityPills();
}

void MainWindow::saveFavoriteCities() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::SearchGroup));
    settings.setValue(QString::fromLatin1(AppSettings::FavoritesKey), favoriteCities);
    settings.endGroup();
}

void MainWindow::syncStartupRegistration() const
{
    setStartupEnabledInWindows(startOnLogin);
}

void MainWindow::setupTrayIntegration()
{
    trayIcon = alertManager->trayIcon();
    if (!QSystemTrayIcon::isSystemTrayAvailable() || trayIcon == nullptr) {
        return;
    }

    trayMenu = new QMenu(this);
    openTrayAction = trayMenu->addAction(QStringLiteral("Open Wynd"));
    refreshTrayAction = trayMenu->addAction(QStringLiteral("Refresh weather"));
    trayMenu->addSeparator();
    quitTrayAction = trayMenu->addAction(QStringLiteral("Quit"));

    trayIcon->setContextMenu(trayMenu);

    connect(openTrayAction,
            &QAction::triggered,
            this,
            &MainWindow::restoreFromTray);
    connect(refreshTrayAction,
            &QAction::triggered,
            this,
            &MainWindow::refreshActiveCity);
    connect(quitTrayAction,
            &QAction::triggered,
            this,
            &MainWindow::quitFromTray);
    connect(trayIcon,
            &QSystemTrayIcon::activated,
            this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger
                    || reason == QSystemTrayIcon::DoubleClick) {
                    restoreFromTray();
                }
            });
}

void MainWindow::hideToTray(bool showHint)
{
    if (!supportsTrayMode()) {
        return;
    }

    hide();

    if (showHint) {
        maybeShowTrayHint();
    }
}

void MainWindow::maybeShowTrayHint()
{
    if (trayHintShown || !supportsTrayMode() || !trayIcon->isVisible()) {
        return;
    }

    trayIcon->showMessage(QStringLiteral("Wynd is still running"),
                          QStringLiteral("Reminders will keep working from the system tray. Use the tray icon to reopen or quit Wynd."),
                          QSystemTrayIcon::Information,
                          10000);

    trayHintShown = true;
    saveWindowPreferences();
}

bool MainWindow::shouldPersistInTray() const
{
    return keepRunningInTray && supportsTrayMode();
}

bool MainWindow::systemPrefersDarkTheme() const
{
    const QStyleHints *styleHints = qApp->styleHints();
    if (styleHints == nullptr) {
        return true;
    }

    switch (styleHints->colorScheme()) {
    case Qt::ColorScheme::Light:
        return false;
    case Qt::ColorScheme::Dark:
        return true;
    default:
        return true;
    }
}

void MainWindow::toggleTheme()
{
    if (isThemeTransitionRunning || isPageTransitionRunning) {
        return;
    }

    isThemeTransitionRunning = true;
    isDarkTheme = !isDarkTheme;
    saveThemePreference();
    themeButton->setEnabled(false);

    QParallelAnimationGroup *transitionGroup = new QParallelAnimationGroup(this);

    QSequentialAnimationGroup *buttonSequence = new QSequentialAnimationGroup(transitionGroup);
    QPropertyAnimation *buttonFadeOut =
        new QPropertyAnimation(themeButtonOpacityEffect, "opacity", buttonSequence);
    buttonFadeOut->setDuration(180);
    buttonFadeOut->setStartValue(1.0);
    buttonFadeOut->setEndValue(0.0);
    buttonFadeOut->setEasingCurve(QEasingCurve::InOutCubic);
    buttonSequence->addAnimation(buttonFadeOut);
    buttonSequence->addAnimation(new QPauseAnimation(45, buttonSequence));
    QPropertyAnimation *buttonFadeIn =
        new QPropertyAnimation(themeButtonOpacityEffect, "opacity", buttonSequence);
    buttonFadeIn->setDuration(360);
    buttonFadeIn->setStartValue(0.0);
    buttonFadeIn->setEndValue(1.0);
    buttonFadeIn->setEasingCurve(QEasingCurve::OutCubic);
    buttonSequence->addAnimation(buttonFadeIn);

    QSequentialAnimationGroup *contentSequence = new QSequentialAnimationGroup(transitionGroup);
    QPropertyAnimation *contentFadeOut =
        new QPropertyAnimation(contentOpacityEffect, "opacity", contentSequence);
    contentFadeOut->setDuration(260);
    contentFadeOut->setStartValue(1.0);
    contentFadeOut->setEndValue(0.08);
    contentFadeOut->setEasingCurve(QEasingCurve::InOutCubic);
    contentSequence->addAnimation(contentFadeOut);
    contentSequence->addAnimation(new QPauseAnimation(60, contentSequence));
    QPropertyAnimation *contentFadeIn =
        new QPropertyAnimation(contentOpacityEffect, "opacity", contentSequence);
    contentFadeIn->setDuration(430);
    contentFadeIn->setStartValue(0.08);
    contentFadeIn->setEndValue(1.0);
    contentFadeIn->setEasingCurve(QEasingCurve::OutCubic);
    contentSequence->addAnimation(contentFadeIn);

    connect(contentFadeOut, &QPropertyAnimation::finished, this, [this]() {
        applyTheme();
    });

    connect(transitionGroup, &QParallelAnimationGroup::finished, this, [this, transitionGroup]() {
        isThemeTransitionRunning = false;
        themeButton->setEnabled(true);
        transitionGroup->deleteLater();
    });

    transitionGroup->start();
}

void MainWindow::openSettings()
{
    if (isThemeTransitionRunning || isPageTransitionRunning) {
        return;
    }

    settingsPage->loadSettings(favoriteCities,
                               alertManager->rules(),
                               alertManager->alertsEnabled(),
                               keepRunningInTray,
                               startOnLogin);
    switchToPage(settingsPage);
}

void MainWindow::switchToPage(QWidget *targetPage)
{
    if (targetPage == nullptr
        || pageStack->currentWidget() == targetPage
        || isThemeTransitionRunning
        || isPageTransitionRunning) {
        return;
    }

    isPageTransitionRunning = true;

    animateOpacitySwap(
        this,
        contentOpacityEffect,
        220,
        30,
        360,
        0.02,
        QEasingCurve(QEasingCurve::InOutCubic),
        QEasingCurve(QEasingCurve::OutCubic),
        [this, targetPage]() {
            pageStack->setCurrentWidget(targetPage);
            if (targetPage == dashboardPage) {
                updateFavoriteCitiesStripGeometry();
            }
        },
        [this]() {
            isPageTransitionRunning = false;

            if (themeButtonOpacityEffect != nullptr) {
                themeButtonOpacityEffect->setOpacity(1.0);
            }
            if (themeButton != nullptr) {
                themeButton->update();
            }
        }
    );
}

void MainWindow::refreshActiveCity()
{
    if (activeCityQuery.trimmed().isEmpty()) {
        locationService->detectLocation();
        return;
    }

    requestWeatherForCity(activeCityQuery);
}

void MainWindow::requestWeatherForCity(const QString &city)
{
    const QString trimmedCity = normalizedCityQuery(city);
    if (trimmedCity.isEmpty()) {
        return;
    }

    activeCityQuery = trimmedCity;
    refreshFavoriteCityPillPresentation();
    api->fetchWeather(trimmedCity);
    api->fetchForecast(trimmedCity);
    subtitleLabel->setText(currentDateLabelText());
}

void MainWindow::rebuildFavoriteCityPills()
{
    if (favoriteCitiesLayout == nullptr
        || favoriteCitiesEmptyLabel == nullptr
        || favoriteCitiesPillArea == nullptr
        || favoriteCitiesScrollArea == nullptr) {
        return;
    }

    QLayoutItem *child = nullptr;
    while ((child = favoriteCitiesLayout->takeAt(0)) != nullptr) {
        if (child->widget() != nullptr) {
            delete child->widget();
        }
        delete child;
    }

    favoriteCityButtons.clear();

    for (const QString &city : favoriteCities) {
        FavoriteCityPillButton *pillButton = new FavoriteCityPillButton(city);
        pillButton->setProperty("favoriteCity", city);
        pillButton->setDarkThemeEnabled(isDarkTheme);
        pillButton->setMinimumHeight(38);
        pillButton->setMinimumWidth(qMax(96, pillButton->fontMetrics().horizontalAdvance(city) + 36));
        pillButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        connect(pillButton, &QPushButton::clicked, this, [this, city]() {
            cityInput->setText(city);
            requestWeatherForCity(city);
            statusBar()->showMessage(QStringLiteral("Loading weather for %1...").arg(city), 2500);
        });

        favoriteCitiesLayout->addWidget(pillButton);
        favoriteCityButtons.append(pillButton);
    }

    updateFavoriteCitiesStripGeometry();
    refreshFavoriteCityPillPresentation();
}

void MainWindow::refreshFavoriteCityPillPresentation()
{
    const QString activeCityKey = favoriteCityComparisonKey(activeCityQuery);

    for (QPushButton *button : favoriteCityButtons) {
        if (button == nullptr) {
            continue;
        }

        const QString favoriteCity = button->property("favoriteCity").toString();
        button->setChecked(!activeCityKey.isEmpty()
                           && favoriteCityComparisonKey(favoriteCity) == activeCityKey);

        if (FavoriteCityPillButton *pillButton =
                dynamic_cast<FavoriteCityPillButton *>(button)) {
            pillButton->setDarkThemeEnabled(isDarkTheme);
        }

        button->update();
    }
}

void MainWindow::updateFavoriteCitiesStripGeometry()
{
    if (favoriteCitiesScrollArea == nullptr
        || favoriteCitiesPillArea == nullptr
        || favoriteCitiesEmptyLabel == nullptr
        || favoriteCitiesLayout == nullptr) {
        return;
    }

    const bool hasFavorites = !favoriteCities.isEmpty();
    favoriteCitiesScrollArea->setVisible(hasFavorites);
    favoriteCitiesEmptyLabel->setVisible(!hasFavorites);

    if (!hasFavorites) {
        favoriteCitiesPillArea->setFixedSize(0, 0);
        favoriteCitiesPillArea->updateGeometry();
        if (favoriteCitiesScrollArea->horizontalScrollBar() != nullptr) {
            favoriteCitiesScrollArea->horizontalScrollBar()->setValue(0);
        }
        return;
    }

    favoriteCitiesPillArea->adjustSize();
    const QSize contentSize = favoriteCitiesLayout->sizeHint();
    favoriteCitiesPillArea->setMinimumSize(contentSize);
    favoriteCitiesPillArea->updateGeometry();

    if (favoriteCitiesScrollArea->horizontalScrollBar() != nullptr) {
        favoriteCitiesScrollArea->horizontalScrollBar()->setValue(0);
    }

    if (favoriteCitiesPanel != nullptr) {
        favoriteCitiesPanel->updateGeometry();
    }
}

void MainWindow::searchWeather()
{
    const QString city = cityInput->text().trimmed();
    if (city.isEmpty()) {
        handleWeatherError("Enter a city");
        return;
    }

    requestWeatherForCity(city);
    currentCityLabel->setText(titleCaseWords(city));
}

void MainWindow::autoCityDetected(QString city, QString country)
{
    city = city.trimmed();
    if (city.isEmpty()) {
        return;
    }

    requestWeatherForCity(city);

    const QString formattedCity = titleCaseWords(city);
    const QString locationLabel = country.trimmed().isEmpty()
        ? formattedCity
        : formattedCity + ", " + country.trimmed();
    currentCityLabel->setText(locationLabel);
}

void MainWindow::displayCurrentWeather(
    QString locationLabel,
    QString condition,
    double temp,
    int humidity,
    double wind,
    QString icon,
    double precip
)
{
    if (!locationLabel.trimmed().isEmpty()) {
        currentCityLabel->setText(locationLabel.trimmed());
    }

    tempLabel->setText(formatTemperatureText(temp));
    conditionLabel->setText(titleCaseWords(condition));

    humidityLabel->setText(QString::number(humidity) + " %");
    windLabel->setText(QString::number(wind) + " km/h");
    precipLabel->setText(QString::number(precip) + " mm");

    const QString iconUrl =
        "https://openweathermap.org/img/wn/" +
        icon +
        "@2x.png";

    QPixmap pix;
    pix.loadFromData(api->downloadIcon(iconUrl));
    if (pix.isNull()) {
        backgroundLabel->setText("--");
        backgroundLabel->setPixmap(QPixmap());
    } else {
        backgroundLabel->setText(QString());
        backgroundLabel->setPixmap(
            pix.scaled(140, 140, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        );
    }
}

void MainWindow::displayHourly(QList<QVariantMap> hourly)
{
    QLayoutItem *child;
    while ((child = hourlyLayout->takeAt(0)) != nullptr) {
        if (child->widget() != nullptr) {
            delete child->widget();
        }
        delete child;
    }

    for (const auto &item : hourly) {
        QWidget *rowWidget = new QWidget;
        rowWidget->setAttribute(Qt::WA_StyledBackground, true);
        rowWidget->setProperty("themeRole", "hourlyRow");
        rowWidget->setMinimumHeight(40);

        QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(10, 0, 10, 0);

        QLabel *timeLab = new QLabel(item["time"].toString());
        timeLab->setProperty("themeRole", "hourlyTime");

        QLabel *iconLab = new QLabel;
        iconLab->setProperty("themeRole", "hourlyIcon");
        iconLab->setAlignment(Qt::AlignCenter);
        iconLab->setFixedSize(48, 48);

        const QString iconCode = item["icon"].toString();
        if (iconCode.trimmed().isEmpty()) {
            iconLab->setText("--");
        } else {
            const QString iconUrl =
                "https://openweathermap.org/img/wn/" +
                iconCode +
                "@2x.png";

            QNetworkAccessManager *manager = new QNetworkAccessManager(iconLab);
            QNetworkRequest request((QUrl(iconUrl)));

            QObject::connect(manager, &QNetworkAccessManager::finished, iconLab, [iconLab](QNetworkReply *reply) {
                if (reply->error() == QNetworkReply::NoError) {
                    QPixmap pix;
                    pix.loadFromData(reply->readAll());
                    iconLab->setPixmap(pix.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                } else {
                    iconLab->setText("--");
                }
                reply->deleteLater();
            });

            manager->get(request);
        }

        QLabel *tmpLab = new QLabel(item["temp"].toString());
        tmpLab->setProperty("themeRole", "hourlyTemp");
        tmpLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        rowLayout->addWidget(timeLab);
        rowLayout->addStretch();
        rowLayout->addWidget(iconLab);
        rowLayout->addWidget(tmpLab);
        hourlyLayout->addWidget(rowWidget);
    }

    hourlyLayout->setSpacing(20);
    hourlyLayout->addStretch();
}

void MainWindow::displayForecast(QList<QVariantMap> forecast)
{
    QLayoutItem *child;
    while ((child = forecastLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    for (const auto &item : forecast) {
        ForecastCard *card = new ForecastCard(
            item["day"].toString(),
            item["temp"].toString(),
            item["humidity"].toString(),
            item["icon"].toString()
        );
        forecastLayout->addWidget(card);
    }
}

void MainWindow::handleWeatherError(QString errorMessage)
{
    currentCityLabel->setText(errorMessage);
    subtitleLabel->setText(currentDateLabelText());

    tempLabel->setText("-- C");
    conditionLabel->setText("--");
    humidityLabel->setText("-- %");
    windLabel->setText("-- km/h");
    precipLabel->setText("-- mm");

    QList<QVariantMap> forecast;
    for (int i = 0; i < 5; ++i) {
        QVariantMap map;
        map["day"] = QDateTime::currentDateTime().addDays(i).toString("ddd");
        map["temp"] = "-- C";
        map["humidity"] = "--%";
        map["icon"] = "";
        forecast.append(map);
    }
    displayForecast(forecast);

    QList<QVariantMap> hourly;
    for (int i = 0; i < 5; ++i) {
        QVariantMap map;
        map["time"] = "--:--";
        map["temp"] = "-- C";
        map["icon"] = "";
        hourly.append(map);
    }
    displayHourly(hourly);

    backgroundLabel->setPixmap(QPixmap());
    backgroundLabel->setText("--");
}

void MainWindow::restoreFromTray()
{
    const bool wasMaximized = isMaximized();
    show();

    if (wasMaximized) {
        showMaximized();
    } else {
        showNormal();
    }

    raise();
    activateWindow();
}

void MainWindow::quitFromTray()
{
    quitRequestedFromTray = true;

    if (trayIcon != nullptr) {
        trayIcon->hide();
    }

    close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (event != nullptr && shouldPersistInTray() && !quitRequestedFromTray) {
        event->ignore();
        hideToTray(true);
        return;
    }

    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    if (event == nullptr
        || event->type() != QEvent::WindowStateChange
        || !isMinimized()
        || !shouldPersistInTray()) {
        return;
    }

    QTimer::singleShot(0, this, [this]() {
        if (isMinimized() && shouldPersistInTray()) {
            hideToTray(true);
        }
    });
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event == nullptr || event->button() != Qt::LeftButton || windowTitleBar == nullptr) {
        QMainWindow::mousePressEvent(event);
        return;
    }

    const QPoint windowPos = event->position().toPoint();
    const QPoint titleLocalPos = windowTitleBar->mapFrom(this, windowPos);
    if (!windowTitleBar->rect().contains(titleLocalPos)) {
        QMainWindow::mousePressEvent(event);
        return;
    }

    QWidget *hitWidget = windowTitleBar->childAt(titleLocalPos);
    if (qobject_cast<QPushButton *>(hitWidget) != nullptr) {
        QMainWindow::mousePressEvent(event);
        return;
    }

    isWindowDragActive = true;
    windowDragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    event->accept();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event == nullptr || !isWindowDragActive) {
        QMainWindow::mouseMoveEvent(event);
        return;
    }

    if (!isMaximized()) {
        move(event->globalPosition().toPoint() - windowDragOffset);
    }

    event->accept();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event != nullptr && event->button() == Qt::LeftButton) {
        isWindowDragActive = false;
    }

    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event == nullptr || event->button() != Qt::LeftButton || windowTitleBar == nullptr) {
        QMainWindow::mouseDoubleClickEvent(event);
        return;
    }

    const QPoint windowPos = event->position().toPoint();
    const QPoint titleLocalPos = windowTitleBar->mapFrom(this, windowPos);
    if (!windowTitleBar->rect().contains(titleLocalPos)) {
        QMainWindow::mouseDoubleClickEvent(event);
        return;
    }

    QWidget *hitWidget = windowTitleBar->childAt(titleLocalPos);
    if (qobject_cast<QPushButton *>(hitWidget) != nullptr) {
        QMainWindow::mouseDoubleClickEvent(event);
        return;
    }

    isMaximized() ? showNormal() : showMaximized();
    event->accept();
}
