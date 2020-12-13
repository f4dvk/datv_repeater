TEMPLATE = app
CONFIG += console
CONFIG -= qt
CONFIG -= app_bundle
DEFINES += MAX_VERBOSE_LEVEL=3
QMAKE_CFLAGS += -std=gnu99
QMAKE_CFLAGS += -g # For profiling

isEmpty(PREFIX) {
 PREFIX = /usr/local/
}
TARGET = multimon
target.path = $$PREFIX/bin
INSTALLS += target

HEADERS += \
    multimon.h \
    filter.h

SOURCES += \
    unixinput.c \
    demod_dtmf.c \
    costabi.c \
    costabf.c

macx{
DEFINES += DUMMY_AUDIO
DEFINES += NO_X11
DEFINES += CHARSET_UTF8
#DEFINES += ARCH_X86_64
#LIBS += -L/usr/X11R6/lib -R/usr/X11R6/lib # If you care you can also compile this on OSX. Though
                                                 # since Apple will remove Xorg from Mountain Lion I feel
                                                 # like we should get rid of this dependency.
}


unix:freebsd-g++:!symbian:!macx{
#DEFINES += ARCH_I386
DEFINES += PULSE_AUDIO
DEFINES += CHARSET_UTF8
LIBS += -L/usr/local/lib -lpulse-simple -lpulse
}

unix:linux-g++-32:!symbian:!macx{
#DEFINES += ARCH_I386
DEFINES += PULSE_AUDIO
DEFINES += CHARSET_UTF8
LIBS += -lpulse-simple -lpulse
}

unix:linux-g++-64:!symbian:!macx{
#DEFINES += ARCH_X86_64
DEFINES += PULSE_AUDIO
DEFINES += CHARSET_UTF8
LIBS += -lpulse-simple -lpulse
}

unix:linux-g++:!symbian:!macx{
DEFINES += PULSE_AUDIO
DEFINES += CHARSET_UTF8
LIBS += -lpulse-simple -lpulse
}
