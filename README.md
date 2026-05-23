# Wynd

Wynd is a modern, C++ desktop weather dashboard application built using the **Qt 6** framework and compiled with **CMake**. 

Designed for both functionality and aesthetic appeal, the application provides real-time current conditions, hourly timelines, and 5-day forecasts, alongside an automated location detection service and a customizable rule-based weather alert notification system that runs in the background.

---

## 🌟 Key Features

*   **Auto Location Detection:** Resolves active coordinates using Nominatim OpenStreetMap IP geocoding services.
    
*   **Detailed Forecasts:** Real-time current conditions, hourly forecasts, and 5-day projections via OpenWeather API.
    
*   **Background Monitoring:** Runs silently in the system tray, keeping your custom alerts active even when the window is hidden.
    
*   **Custom Weather Alerts:** Setup custom rule-based reminders (e.g., "Alert me 3 hours before if the temperature falls below 5°C").
   
*   **Premium UI & Aesthetics:** Custom graphics painting, smooth opacity transitions, and dynamic Dark/Light theme toggling.

---

## 🛠️ Technology Stack

*   **Core:** C++17
*   **UI Framework:** Qt 6 (Widgets and Network modules)
*   **Build System:** CMake (3.16+)
*   **APIs:** OpenWeatherMap API & IP-API

---

## ⚙️ Compilation & Setup

### Prerequisites

Ensure you have the following installed:
*   A C++17 compatible compiler (MSVC, GCC, or Clang)
*   CMake 3.16 or higher
*   Qt 6.0+ SDK (including **Widgets** and **Network** modules)

### Building the Project

1.  Clone the repository:
    ```bash
    git clone https://github.com/Muaaz-07/Wynd.git
    cd Wynd
    ```

    
2.  Add your OpenWeather API key inside [src/weather/WeatherAPI.h](src/weather/WeatherAPI.h) by updating:
    ```cpp
    QString apiKey = "YOUR_API_KEY_HERE";

    ```

    Note: You can get your free OpenWeather API key by going to https://openweathermap.org/ and signing up.

    
    
4.  Configure and compile the project using CMake:
    ```
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    ```

    
5.  Launch the application:
    *   On Windows: `Release\Wynd.exe`
    *   On Linux/macOS: `./Wynd`


---


## 📜License

Wynd is Licensed under the [GNU General Public License v3.0](LICENSE). See the [LICENSE](LICENSE) file for the full GPLv3 terms.
