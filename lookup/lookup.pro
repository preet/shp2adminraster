QT       += core

CONFIG   += console
TEMPLATE = app

# avoid linking in dl since we dont use it
DEFINES += SQLITE_OMIT_LOAD_EXTENSION


# kompex
PATH_KOMPEX = /home/preet/Dev/env/sys/kompex
INCLUDEPATH += $${PATH_KOMPEX}/include
HEADERS += \
    $${PATH_KOMPEX}/include/sqlite3.h \
    $${PATH_KOMPEX}/include/KompexSQLiteStreamRedirection.h \
    $${PATH_KOMPEX}/include/KompexSQLiteStatement.h \
    $${PATH_KOMPEX}/include/KompexSQLitePrerequisites.h \
    $${PATH_KOMPEX}/include/KompexSQLiteException.h \
    $${PATH_KOMPEX}/include/KompexSQLiteDatabase.h \
    $${PATH_KOMPEX}/include/KompexSQLiteBlob.h

LIBS += -L$${PATH_KOMPEX}/lib -lkompex

# main
SOURCES += lookup.cpp
