#-------------------------------------------------
#
# Project created by QtCreator 2010-07-15T14:44:54
#
#-------------------------------------------------

QT       += core gui phonon

maemo5 { QT += maemo5 }

TARGET = tomamp
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    playlistmanager.cpp \
    optiondialog.cpp

HEADERS  += mainwindow.h \
    playlistmanager.h \
    optiondialog.h

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

OTHER_FILES += \
    bugs.txt \
    README

unix {
    # VARIABLES
    isEmpty(PREFIX):PREFIX = /usr #/local ?
    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share
    DEFINES += DATADIR=\"$$DATADIR\" \
        PKGDATADIR=\"$$PKGDATADIR\"

    contains(QT_CONFIG, hildon):{
          DEFINES += CHIMGDIR=\'\"$$DATADIR/$${TARGET}\"\'
    }
    # MAKE INSTALL
    INSTALLS += target \
        imagery \
        desktop \
        iconxpm \
        icon26 \
        icon40 \
        icon64
    target.path = $$BINDIR
    imagery.path = $$DATADIR/$${TARGET}/images
    imagery.files += ../src/images/*png
    desktop.path = $$DATADIR/applications/hildon
    desktop.files += $${TARGET}.desktop
    iconxpm.path = $$DATADIR/pixmap
    iconxpm.files += ../data/maemo/$${TARGET}.xpm
    icon26.path = $$DATADIR/icons/hicolor/26x26/apps
    icon26.files += ../data/26x26/Tomamp.png
    icon40.path = $$DATADIR/icons/hicolor/40x40/apps
    icon40.files += ../data/40x40/Tomamp.png
    icon64.path = $$DATADIR/icons/hicolor/64x64/apps
    icon64.files += ../data/64x64/Tomamp.png
}
