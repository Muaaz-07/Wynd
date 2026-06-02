# Wynd

A modern desktop weather dashboard built with **C++17**, **Qt 6**, and **CMake**.

Wynd combines real-time weather monitoring with a polished desktop experience, offering current conditions, hourly forecasts, 5-day projections, automatic location detection, and customizable weather alerts that continue running in the background.

---

## 🌟 Features

- **📍 Automatic Location Detection**  
  Resolves your current coordinates using IP-API.

- **🌤 Detailed Forecasts**  
  View current conditions, hourly forecasts, and 5-day weather projections powered by OpenWeatherMap.

- **🔔 Custom Weather Alerts**  
  Create rule-based reminders such as:

  > Alert me 3 hours before the temperature falls below 5°C.

- **🖥 Background Monitoring**  
  Runs quietly in the system tray and keeps alerts active even when the main window is hidden.

- **🎨 Modern UI**  
  Features custom-painted graphics, smooth opacity transitions, and dynamic Dark/Light theme switching.

---

## 🛠 Technology Stack

| Component | Technology |
|------------|------------|
| Language | C++17 |
| UI Framework | Qt 6 (Widgets + Network) |
| Build System | CMake 3.16+ |
| Weather Data | OpenWeatherMap API |
| Geolocation | IP-API |

---

## ⚙️ Compilation & Setup

### Prerequisites

Before building Wynd, ensure the following are installed:

- C++17 compatible compiler (MSVC, GCC, or Clang)
- CMake 3.16 or newer
- Qt 6 SDK with:
  - Widgets module
  - Network module

---

### Step 1 — Clone the Repository

```bash
git clone https://github.com/Muaaz-07/Wynd.git
cd Wynd
```

---

### Step 2 — Configure Your OpenWeatherMap API Key

Open:

 > File: [src/weather/WeatherAPI.h](src/weather/WeatherAPI.h)
   
Replace:

```cpp
QString apiKey = "YOUR_API_KEY_HERE";
```

with your OpenWeatherMap API key.

> **Note**
>
> You can obtain a free API key by creating an account at https://openweathermap.org/

---

## 🔨 Build the Project

Choose one of the following methods.

### Option A — Build Using CMake (CLI)

Configure and compile from the command line:

```bash
mkdir build
cd build

cmake ..
cmake --build . --config Release
```

#### Run the Application

**Windows**

```bash
Release\Wynd.exe
```

**Linux / macOS**

```bash
./Wynd
```

---

### Option B — Build Using Qt Creator

1. Launch **Qt Creator**.
2. Select **File → Open File or Project**.
3. Open the project's `CMakeLists.txt`.
4. Select a compatible **Qt 6 Kit**.
5. Click **Configure Project**.
6. Verify your API key has been added to `WeatherAPI.h`.
7. Build the project:
   - **Build → Build Project**
   - or press **Ctrl + B**
8. Run the application:
   - **Run → Run**
   - or press **Ctrl + R**



---

## 📜 License

Wynd is licensed under the [GNU General Public License v3.0](LICENSE).

See the [LICENSE](LICENSE) file for the complete GPLv3 license terms.
