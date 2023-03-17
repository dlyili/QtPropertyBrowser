infile(config.pri, SOLUTIONS_LIBRARY, yes): CONFIG += qtpropertybrowser-uselib
QT += core gui widgets
TEMPLATE += fakelib
QTPROPERTYBROWSER_LIBNAME = $$qtLibraryTarget(QtPropertyBrowser)
TEMPLATE -= fakelib
CONFIG += c++17

CONFIG(debug,debug|release){
    DESTDIR = $$PWD/x64/Debug/
} else {
    DESTDIR = $$PWD/x64/Release/
}
QTPROPERTYBROWSER_LIBDIR = $$DESTDIR
unix:qtpropertybrowser-uselib:!qtpropertybrowser-buildlib:QMAKE_RPATHDIR += $$QTPROPERTYBROWSER_LIBDIR
