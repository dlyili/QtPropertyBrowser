TEMPLATE=lib
DEFINES += QTPROPERTYBROWSER_LIBRARY
CONFIG += staticlib qtpropertybrowser-buildlib 
mac:CONFIG += absolute_library_soname
win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release build_all 
include(../src/qtpropertybrowser.pri)
TARGET = $$QTPROPERTYBROWSER_LIBNAME
DESTDIR = $$QTPROPERTYBROWSER_LIBDIR
target.path = $$DESTDIR
INSTALLS += target
