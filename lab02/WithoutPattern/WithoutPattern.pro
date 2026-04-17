QT += widgets
CONFIG += c++17

TARGET = dibooks_without

SOURCES += main_without_facade.cpp

# На macOS ARM с Homebrew Qt5, если нужно явно:
# macx {
#     INCLUDEPATH += /opt/homebrew/opt/qt@5/include
#     LIBS += -L/opt/homebrew/opt/qt@5/lib
# }
