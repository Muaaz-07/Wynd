//
// Created by Muhammad Muaaz on 3/11/2026.
//
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QPoint>
#include <QStringList>
#include "../weather/WeatherAPI.h"
#include "../location/locationService.h"

class QAction;
class QCloseEvent;
class QEvent;
class QLineEdit;
class QMenu;
class QPushButton;
class QLabel;
class QHBoxLayout;
class QScrollArea;
class QVBoxLayout;
class QGraphicsOpacityEffect;
class CurvedSurfaceWidget;
class QTimer;
class QSystemTrayIcon;
class WeatherAlertManager;
class QStackedWidget;
class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    // Main weather dashboard
    explicit MainWindow(bool startHiddenInTray = false, QWidget *parent = nullptr);
    bool supportsTrayMode() const;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    // Uses the detected city to fetch the initial weather payload.
    void autoCityDetected(QString city, QString country);
    // Runs a manual city search from the search field.
    void searchWeather();

    // Updates the primary weather summary shown in the hero card.
    void displayCurrentWeather(
        QString locationLabel,
        QString condition,
        double temp,
        int humidity,
        double wind,
        QString icon,
        double precip
    );

    // Rebuilds the daily forecast cards from the latest API response.
    void displayForecast(QList<QVariantMap> forecast);
    // Rebuilds the hourly list shown in the right-side panel.
    void displayHourly(QList<QVariantMap> hourly);
    // Resets the UI to a safe fallback state when weather calls fail.
    void handleWeatherError(QString errorMessage);

    // Swaps between the dark and light QSS themes.
    void toggleTheme();
    // Switches the main view from the dashboard to the settings page.
    void openSettings();
    // Refreshes the current city so reminders stay aligned with the latest forecast.
    void refreshActiveCity();

private:
    // Loads the active theme file and refreshes theme-dependent labels.
    void applyTheme();
    // Returns the current date text shown under the detected city.
    QString currentDateLabelText() const;
    // Updates the icon-only theme button without changing the active stylesheet.
    void updateThemeButtonIcon();
    // Restores the user's explicit theme or falls back to the system color scheme.
    void loadThemePreference();
    // Saves the current explicit theme choice.
    void saveThemePreference() const;
    // Loads the background behavior preferences.
    void loadWindowPreferences();
    // Saves tray and startup preferences.
    void saveWindowPreferences() const;
    // Loads favorite cities for the dashboard quick-switch section.
    void loadFavoriteCities();
    // Persists favorite cities chosen from Settings.
    void saveFavoriteCities() const;
    // Synchronizes the Windows login startup entry with app settings.
    void syncStartupRegistration() const;
    // Configures restore, refresh, and quit actions on the tray icon.
    void setupTrayIntegration();
    // Hides the main window while keeping reminder timers running.
    void hideToTray(bool showHint);
    // Shows a one-time tray hint for discoverability.
    void maybeShowTrayHint();
    // Returns true when close/minimize should keep the app alive in the tray.
    bool shouldPersistInTray() const;
    // Returns the current system preference when no explicit theme exists.
    bool systemPrefersDarkTheme() const;
    // Fetches both the current conditions and forecast for the requested city.
    void requestWeatherForCity(const QString &city);
    // Rebuilds the favorite-city pills shown on the dashboard.
    void rebuildFavoriteCityPills();
    // Updates selection and shadow styling for the favorite-city pills.
    void refreshFavoriteCityPillPresentation();
    // Sizes the favorite-city strip to match its button content.
    void updateFavoriteCitiesStripGeometry();
    // Smoothly swaps the stacked main content to the requested page.
    void switchToPage(QWidget *targetPage);
    // Restores the window from the tray icon.
    void restoreFromTray();
    // Exits the app from the tray menu.
    void quitFromTray();

    LocationService *locationService;
    WeatherAlertManager *alertManager;

    bool isDarkTheme = true;
    bool isThemeTransitionRunning = false;
    bool isPageTransitionRunning = false;
    bool keepRunningInTray = true;
    bool startOnLogin = false;
    bool trayHintShown = false;
    bool quitRequestedFromTray = false;

    QWidget *centralWidgetRoot;
    QWidget *dashboardPage;

    QWidget *blueCard;
    CurvedSurfaceWidget *leftContentSurface;
    QWidget *card1;
    QWidget *card2;
    QWidget *card3;
    QWidget *card4;
    QWidget *favoriteCitiesPanel;
    QWidget *favoriteCitiesPillArea;
    QScrollArea *favoriteCitiesScrollArea = nullptr;
    QWidget *rightCard;

    QLabel *backgroundLabel;
    QLabel *favoriteCitiesEmptyLabel;

    QLineEdit *cityInput;
    QPushButton *searchButton;

    QPushButton *settingsButton;
    QPushButton *themeButton;

    QWidget *windowTitleBar = nullptr;
    QLabel *windowTitleLabel = nullptr;
    QPushButton *minimizeButton = nullptr;
    QPushButton *maximizeButton = nullptr;
    QPushButton *closeButton = nullptr;

    bool isWindowDragActive = false;
    QPoint windowDragOffset;

    QLabel *titleLabel;
    QLabel *currentCityLabel;
    QLabel *subtitleLabel;

    QLabel *tempLabel;
    QLabel *conditionLabel;
    QLabel *humidityLabel;
    QLabel *windLabel;
    QLabel *precipLabel;

    QHBoxLayout *forecastLayout;
    QVBoxLayout *leftColumnLayout;
    QVBoxLayout *hourlyLayout;
    QHBoxLayout *favoriteCitiesLayout = nullptr;

    QGraphicsOpacityEffect *contentOpacityEffect;
    QGraphicsOpacityEffect *themeButtonOpacityEffect;

    WeatherAPI *api;
    QTimer *refreshTimer;
    QStackedWidget *pageStack;
    SettingsDialog *settingsPage;
    QSystemTrayIcon *trayIcon = nullptr;
    QMenu *trayMenu = nullptr;
    QAction *openTrayAction = nullptr;
    QAction *refreshTrayAction = nullptr;
    QAction *quitTrayAction = nullptr;
    QString activeCityQuery;
    QStringList favoriteCities;
    QList<QPushButton *> favoriteCityButtons;
};

#endif
