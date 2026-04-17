cmake_minimum_required(VERSION 3.16)
project(DiBooks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Поиск Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(DiBooks
    main_with_facade.cpp
)

target_link_libraries(DiBooks PRIVATE Qt6::Core Qt6::Widgets)

# Указываем пути к Qt для Homebrew
if(APPLE)
    target_include_directories(DiBooks PRIVATE
        /opt/homebrew/opt/qt/include
        /opt/homebrew/opt/qt/include/QtCore
        /opt/homebrew/opt/qt/include/QtWidgets
    )
endif()
