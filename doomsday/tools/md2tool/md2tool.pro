# qmake project file for building md2tool
# (c) 2012-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
# License: GPL 2.0+

include(../../config.pri)

TEMPLATE = app
TARGET = md2tool

CONFIG -= app_bundle
win32: CONFIG += console
QT -= core gui

# TODO: need to test whether compiler supports this option
#!deng_macx4u_32bit : !deng_macx6_32bit_64bit {
#    *-g++* | *-gcc* | *-clang* {
#        QMAKE_CFLAGS_WARN_ON += -Wno-unused-result
#    }
#}

SOURCES += md2tool.c

HEADERS += md2tool.h anorms.h

# Installation.
!macx {
    INSTALLS += target
    target.path = $$DENG_BIN_DIR
}
