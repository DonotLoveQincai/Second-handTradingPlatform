# 联调用例：不启动界面，直接调 services + DAO，跑真实数据库。
# 和主工程完全独立（单独一个 .pro、单独一个 exe），不进 Second-handTradingPlatform 工程的编译列表。

QT       += core sql
QT       -= gui
CONFIG   += c++17 console
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = integration_test

# 复用主工程的源码目录
MAIN_DIR  = $$PWD/../Second-handTradingPlatform
INCLUDEPATH += $$MAIN_DIR

SOURCES += integration_test.cpp \
    $$files($$MAIN_DIR/core/*.cpp, true) \
    $$files($$MAIN_DIR/data/*.cpp, true) \
    $$files($$MAIN_DIR/services/*.cpp, true)

HEADERS += \
    $$files($$MAIN_DIR/core/*.h, true) \
    $$files($$MAIN_DIR/data/*.h, true) \
    $$files($$MAIN_DIR/services/*.h, true)

# 数据库脚本在 qrc 里（DbManager::init 从 :/sql/001_init.sql 读）
RESOURCES += $$MAIN_DIR/ui/resources/resources.qrc

# 中文输出不要乱码
msvc {
    QMAKE_CXXFLAGS += /utf-8
}
