# Wynd 🌬️

Wynd is a modern, high-performance C++ desktop weather dashboard application built using the **Qt 6** framework and compiled with **CMake**. 

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

## 🏗️ Object-Oriented C++ Implementation

Wynd was built to showcase core Object-Oriented Programming (OOP) and system design paradigms:

1.  **Inheritance:** Extended core Qt classes to build specialized components. (e.g., `MainWindow` inherits from `QMainWindow`, `FlowLayout` from `QLayout`, and widgets like `CurvedSurfaceWidget` from `QWidget`).
2.  **Encapsulation:** Hidden object member states (using `private`/`protected` modifiers) and exposed state transitions exclusively via public interfaces, signals, slots, and properties. Configuration keys are isolated in the `AppSettings` namespace.
3.  **Abstraction:** Network queries and JSON parsing logic are abstracted inside `WeatherAPI` and `LocationService`, decoupling the frontend GUI widgets from raw web payloads.
4.  **Static & Dynamic Polymorphism:**
    *   *Dynamic:* Overrode paint events (`paintEvent`) and layout operations (`setGeometry`, `sizeHint`) utilizing standard virtual table dispatch.
    *   *Static:* Utilized compile-time template instantiation for lists and associative structures (`QList`, `QMap`).
5.  **File Handling:** Implemented file reading through `QFile` to dynamically parse `.qss` theme stylesheets, and structured configuration reading/writing using `QSettings` to persist user choices and rules.

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
    git clone https://github.com/YOUR_USERNAME/Wynd.git
    cd Wynd
    ```
2.  Add your OpenWeather API key inside [src/weather/WeatherAPI.h](src/weather/WeatherAPI.h) by updating:
    ```cpp
    QString apiKey = "YOUR_API_KEY_HERE";
    ```
3.  Configure and compile the project using CMake:
    ```bash
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    ```
4.  Launch the application:
    *   On Windows: `Release\Wynd.exe`
    *   On Linux/macOS: `./Wynd`

---

## 👥 Group Project Division of Work

*   **Member 1 (Architect):** Developed the core application architecture (`Main.cpp`), `MainWindow` layout bootstrap, window drag events, minimize-to-tray interactions, and main event connections.
*   **Member 2 (API/Network Developer):** Built the async networking engine (`WeatherAPI`) and location service (`LocationService`), constructed HTTP request routing, handled JSON parsing, and managed network error states.
*   **Member 3 (System Logic/Persistence):** Authored the background alert rules evaluator (`WeatherAlertManager`), designed alert timeline matching, and configured settings serialization/deserialization to disk using `QSettings`.
*   **Member 4 (UI Designer):** Designed the `SettingsDialog` form controls, implemented input validations, managed the dynamic style-sheet parser (`QFile`), and created the smooth property animations.
*   **Member 5 (Layout & Graphics):** Programmed the custom flow-wrap layout engine (`FlowLayout`) and managed low-level vector rendering (`QPainter` overrides) for custom panel shapes and anti-aliased geometry.
