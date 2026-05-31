cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0084 NEW)

project(miniscope VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_QCUSTOMPLOT "Use QCustomPlot backend instead of Qt Charts" OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts SerialPort PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts SerialPort PrintSupport)

include(FetchContent)
FetchContent_Declare(
    QtAwesome
    GIT_REPOSITORY https://github.com/gamecreature/QtAwesome.git
    GIT_TAG        main
)
FetchContent_GetProperties(QtAwesome)
if(NOT qtawesome_POPULATED)
    FetchContent_Populate(QtAwesome)
endif()

# Sources shared by both backends
set(SHARED_SOURCES
    main.cpp
    qLabeledUnitedSpinBox.h qLabeledUnitedSpinBox.cpp
    serialReader.h serialReader.cpp
)

# Backend-specific sources
if(USE_QCUSTOMPLOT)
    set(BACKEND_SOURCES
        mainwindow_qcp.h mainwindow_qcp.cpp
        channel_qcp.h   channel_qcp.cpp
        qcustomplot.h   qcustomplot.cpp
    )
else()
    set(BACKEND_SOURCES
        mainwindow.h mainwindow.cpp
        channel.h    channel.cpp
        qUnitedAxis.h qUnitedAxis.cpp
    )
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(miniscope
        MANUAL_FINALIZATION
        ${SHARED_SOURCES}
        ${BACKEND_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET miniscope APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(miniscope SHARED
            ${SHARED_SOURCES}
            ${BACKEND_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(miniscope
            ${SHARED_SOURCES}
            ${BACKEND_SOURCES}
        )
    endif()
endif()

if(USE_QCUSTOMPLOT)
    target_compile_definitions(miniscope PRIVATE USE_QCUSTOMPLOT)
    target_link_libraries(miniscope PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::SerialPort
        Qt${QT_VERSION_MAJOR}::PrintSupport
    )
else()
    target_link_libraries(miniscope PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Charts
        Qt${QT_VERSION_MAJOR}::SerialPort
    )
endif()

target_sources(miniscope PRIVATE
    ${qtawesome_SOURCE_DIR}/QtAwesome/QtAwesome.cpp
    ${qtawesome_SOURCE_DIR}/QtAwesome/QtAwesomeAnim.cpp
    ${qtawesome_SOURCE_DIR}/QtAwesome/QtAwesomeFree.qrc
)
target_include_directories(miniscope PRIVATE ${qtawesome_SOURCE_DIR}/QtAwesome)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.miniscope)
endif()
set_target_properties(miniscope PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS miniscope
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(miniscope)
endif()
