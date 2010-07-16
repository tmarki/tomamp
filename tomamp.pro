#-------------------------------------------------
#
# Project created by QtCreator 2010-07-15T14:44:54
#
#-------------------------------------------------

QT       += core gui phonon

TARGET = tomamp
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    +=

CONFIG += mobility
MOBILITY = 

symbian {
    TARGET.UID3 = 0xe3107f4b
    # TARGET.CAPABILITY += 
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}

RESOURCES += \
    ampres.qrc
