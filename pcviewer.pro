HEADERS  = scene.h \
    viewer.h \
    mainwindow.h \
    camera.h
SOURCES  = scene.cpp \
    main.cpp \
    viewer.cpp \
    mainwindow.cpp \
    camera.cpp

QT += widgets

CONFIG += c++11

RESOURCES += \
    resources.qrc

unix:!mac {
 LIBS += -Wl,-rpath=\\\$$ORIGIN/../lib:\\\$$ORIGIN
}

target.path = /usr/share/pcviewer/bin
target.files = pcviewer qt.conf
INSTALLS += target
data.path = /usr/share/pcviewer/lib
data.files = /home/den/Qt5.5.1/5.5/gcc_64/lib/*
INSTALLS += data
