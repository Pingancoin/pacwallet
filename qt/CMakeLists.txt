cmake_minimum_required(VERSION 3.21)

project(PingancoinWalletQt VERSION 0.1.0 LANGUAGES CXX)

set(PACWALLET_QT_VERSION "${PROJECT_VERSION}" CACHE STRING "Pingancoin wallet display version")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

set(PACWALLET_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(PACWALLET_MACOS_ICON "${PACWALLET_RESOURCE_DIR}/pingancoin.icns")
set(PACWALLET_WINDOWS_ICON "${PACWALLET_RESOURCE_DIR}/pingancoin-icon.ico")

qt_add_executable(pacwallet-qt
    MACOSX_BUNDLE
    src/main.cpp
    src/ApiClient.cpp
    src/Localization.cpp
    src/MainWindow.cpp
    src/ServiceController.cpp
    src/pages/MultisigPage.cpp
    src/pages/OverviewPage.cpp
    src/pages/ReceivePage.cpp
    src/pages/SendPage.cpp
    src/pages/SettingsPage.cpp
    src/pages/TransactionsPage.cpp
    src/pages/WelcomePage.cpp
    resources/pacwallet.qrc
)

target_include_directories(pacwallet-qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(pacwallet-qt PRIVATE
    PACWALLET_QT_VERSION="${PACWALLET_QT_VERSION}"
)

target_link_libraries(pacwallet-qt PRIVATE
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    configure_file(
        "${PACWALLET_RESOURCE_DIR}/pacwallet.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/pacwallet.rc"
        @ONLY
    )
    target_sources(pacwallet-qt PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/pacwallet.rc")
    set_target_properties(pacwallet-qt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(APPLE)
    target_sources(pacwallet-qt PRIVATE "${PACWALLET_MACOS_ICON}")
    set_source_files_properties("${PACWALLET_MACOS_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    set_target_properties(pacwallet-qt PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Pingancoin Wallet"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.pingancoin.wallet"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "pingancoin.icns"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()
