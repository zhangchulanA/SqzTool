

HEADERS += \
    $$PWD/other/ChainBranch.h \
    $$PWD/other/FormValidator.h \
    $$PWD/other/SafeTimer.h \
    $$PWD/other/Singleton.h \
    $$PWD/other/SqzPluginInterface.h \
    $$PWD/other/SqzPluginManager.h \
    $$PWD/other/SqzThreadPool.h \
    $$PWD/other/TimeoutKeeper.h \
    $$PWD/translator/SqzTranslator.h \
    $$PWD/translator/SqzTranslatorMask.h \
    $$PWD/utils/FileUtils.h \
    $$PWD/utils/IniUtils.h \
    $$PWD/utils/JsonUtils.h \
    $$PWD/utils/StringUtils.h \
    $$PWD/utils/SystemUtils.h \
    $$PWD/utils/TimeUtils.h \
    $$PWD/utils/TimerUtils.h

SOURCES += \
    $$PWD/other/ChainBranch.cpp \
    $$PWD/other/FormValidator.cpp \
    $$PWD/other/SqzPluginInterface.cpp \
    $$PWD/other/SqzPluginManager.cpp \
    $$PWD/other/Singleton.cpp \
    $$PWD/other/SqzThreadPool.cpp \
    $$PWD/other/TimeoutKeeper.cpp \
    $$PWD/translator/SqzTranslator.cpp \
    $$PWD/translator/SqzTranslatorMask.cpp \
    $$PWD/utils/FileUtils.cpp \
    $$PWD/utils/IniUtils.cpp \
    $$PWD/utils/JsonUtils.cpp \
    $$PWD/utils/StringUtils.cpp \
    $$PWD/utils/SystemUtils.cpp \
    $$PWD/utils/TimeUtils.cpp \
    $$PWD/utils/TimerUtils.cpp

INCLUDEPATH +=  $$PWD/translator\
                $$PWD/utils \
                $$PWD/other
