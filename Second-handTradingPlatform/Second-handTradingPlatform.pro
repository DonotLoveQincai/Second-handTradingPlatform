QT       += core gui widgets sql
CONFIG   += c++17
TEMPLATE  = app
TARGET    = Second-handTradingPlatform

# 让 #include "core/entity/product.h" 这种写法能找到文件
INCLUDEPATH += $$PWD

# MSVC 下强制按 UTF-8 编译，否则中文字符串会乱码
msvc {
    QMAKE_CXXFLAGS += /utf-8
}

SOURCES += main.cpp

include(core/core.pri)
include(data/data.pri)
include(services/services.pri)
include(ui/ui.pri)

RESOURCES += ui/resources/resources.qrc

# 输出目录固定，避免 VS 和 Creator 各生成一份
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/debug
} else {
    DESTDIR = $$PWD/release
}