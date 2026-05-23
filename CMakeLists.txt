cmake_minimum_required(VERSION 3.16)
project(UdpChat LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql)

add_executable(udp_chat
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    chatdatabase.h
    chatdatabase.cpp
)

target_link_libraries(udp_chat PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)