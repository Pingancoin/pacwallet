cmake_minimum_required(VERSION 3.21)

project(PingancoinWalletQt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

qt_add_executable(pacwallet-qt
    src/main.cpp
    src/ApiClient.cpp
    src/MainWindow.cpp
    src/ServiceController.cpp
    src/pages/MultisigPage.cpp
    src/pages/OverviewPage.cpp
    src/pages/ReceivePage.cpp
    src/pages/SendPage.cpp
    src/pages/SettingsPage.cpp
    src/pages/TransactionsPage.cpp
    src/pages/WelcomePage.cpp
)

target_include_directories(pacwallet-qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(pacwallet-qt PRIVATE
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    set_target_properties(pacwallet-qt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
