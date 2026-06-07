cmake_minimum_required(VERSION 3.16)

project(Wynd LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTORCC ON)

# Widgets powers the desktop UI while Network handles the weather/location requests.
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()
set_source_files_properties(wynd.rc PROPERTIES SKIP_AUTOGEN ON)

# The executable bundles the UI, services, resources, and Windows icon metadata.
qt_add_executable(Wynd WIN32

        resources.qrc
    src/Main.cpp
    src/alerts/WeatherAlertManager.cpp
    src/alerts/WeatherAlertManager.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/SettingsDialog.cpp
    src/ui/SettingsDialog.h
    src/weather/WeatherAPI.cpp
    src/weather/WeatherAPI.h
    src/location/locationService.cpp
    src/location/locationService.h
    src/widgets/CurvedSurfaceWidget.cpp
    src/widgets/CurvedSurfaceWidget.h
    src/widgets/ForecastCard.cpp
    src/widgets/ForecastCard.h
    wynd.rc
)

target_link_libraries(Wynd PRIVATE
    Qt6::Widgets
    Qt6::Network
)

# Only run deployment in Release mode.
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")

    # Find the windeployqt executable
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_MKSPECS_DIR}/../bin")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET Wynd POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            
            --compiler-runtime
            
            --no-translations
            
            "$<TARGET_FILE:Wynd>"
            COMMENT "Running windeployqt to copy Qt dependencies..."
        )
    else()
        message(WARNING "windeployqt not found! Deployment step skipped.")
    endif()
endif()
