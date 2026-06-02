#ifndef APPSETTINGS_H
#define APPSETTINGS_H

namespace AppSettings {

inline constexpr auto AppearanceGroup = "appearance";
inline constexpr auto ThemeKey = "theme";
inline constexpr auto DarkThemeValue = "dark";
inline constexpr auto LightThemeValue = "light";

inline constexpr auto SearchGroup = "search";
inline constexpr auto FavoritesKey = "favorites";
inline constexpr auto RecentSearchesKey = "recentSearches";

inline constexpr auto WeatherGroup = "weather";
inline constexpr auto ApiKeyKey = "apiKey";

inline constexpr auto WindowGroup = "window";
inline constexpr auto KeepRunningInTrayKey = "keepRunningInTray";
inline constexpr auto RunOnStartupKey = "runOnStartup";
inline constexpr auto HasShownTrayHintKey = "hasShownTrayHint";

inline constexpr auto AlertsGroup = "alerts";
inline constexpr auto AlertsEnabledKey = "enabled";
inline constexpr auto AlertRulesKey = "rules";
inline constexpr auto DeliveredReminderExpiriesKey = "deliveredReminderExpiries";

inline constexpr auto CacheGroup = "cache";
inline constexpr auto WeatherResponseGroup = "currentWeather";
inline constexpr auto ForecastResponseGroup = "forecast";

inline constexpr int MaxFavorites = 8;
inline constexpr int MaxRecentSearches = 10;

}

#endif
