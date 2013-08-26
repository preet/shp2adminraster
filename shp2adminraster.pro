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


# shapelib
PATH_SHAPELIB = /home/preet/Dev/scratch/gis/shapefiles/shapelib
INCLUDEPATH += $${PATH_SHAPELIB}
HEADERS += \
    $${PATH_SHAPELIB}/shapefil.h

SOURCES += \
    $${PATH_SHAPELIB}/shpopen.c \
    $${PATH_SHAPELIB}/shptree.c \
    $${PATH_SHAPELIB}/dbfopen.c \
    $${PATH_SHAPELIB}/safileio.c

# main
SOURCES += shp2adminraster.cpp
