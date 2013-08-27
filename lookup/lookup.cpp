/*
   Copyright 2013 Preet Desai

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include <exception>

// qt
#include <QCoreApplication>
#include <QStringList>
#include <QDebug>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QColor>

// kompex
#include "KompexSQLitePrerequisites.h"
#include "KompexSQLiteDatabase.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "KompexSQLiteBlob.h"

void badInput()
{
    qDebug() << "ERROR: Wrong number of arguments: ";
    qDebug() << "* Pass the adminraster.sqlite file created by";
    qDebug() << "  shp2adminraster as the first argument followed";
    qDebug() << "  by the longitude and latitude.";
    qDebug() << "Ex:";
    qDebug() << "./lookup /path/to/adminraster.sqlite -79.3 43.5";
}

void getTilePixel(double lon,
                  double lat,
                  size_t &tile_idx,
                  size_t &pixel_x,
                  size_t &pixel_y)
{
    size_t adjTile = 0;
    double adjLon = lon + 180.0;
    double adjLat = (lat-90.0)*-1.0;
    if(lon > 0.0)   {
        adjTile = 324;
        adjLon = lon;
    }
    size_t rowIdx = adjLat/10;
    size_t colIdx = adjLon/10;

    tile_idx = (rowIdx*18 + colIdx) + adjTile;
    pixel_x = adjLon*100 - colIdx*1000;
    pixel_y = adjLat*100 - rowIdx*1000;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);

    // check input args
    QStringList inputArgs = app.arguments();
    if(inputArgs.size() < 4)   {
        badInput();
        return -1;
    }

    bool opOk = false;

    double lon = inputArgs[2].toDouble(&opOk);
    if(!opOk || (lon < -180.0) || (lon > 180.0))   {
        qDebug() << "ERROR: Invalid longitude";
        return -1;
    }

    double lat = inputArgs[3].toDouble(&opOk);
    if(!opOk || (lat < -90.0) || (lat > 90.0))   {
        qDebug() << "ERROR: Invalid longitude";
        return -1;
    }

    Kompex::SQLiteDatabase  * pDatabase;
    Kompex::SQLiteStatement * pStmt;
    try   {
        pDatabase = new Kompex::SQLiteDatabase(
                    inputArgs[1].toStdString(),
                    SQLITE_OPEN_READONLY,0);

        pStmt = new Kompex::SQLiteStatement(pDatabase);
    }
    catch(Kompex::SQLiteException &exception)   {
        qDebug() << "ERROR: Could not open database:";
        qDebug() << QString::fromStdString(exception.GetString());
        return -1;
    }

    // get tile and pixel based on input coordinates
    size_t tileIdx,pixel_x,pixel_y;
    getTilePixel(lon,lat,tileIdx,pixel_x,pixel_y);

    qDebug() << "Input Coords: (" << lon << "," << lat << ")";
    qDebug() << "Tile:" << tileIdx;
    qDebug() << "Pixel: (" << pixel_x << "," << pixel_y << ")";

    // get tile image data from the database
    Kompex::SQLiteBlob * pBlob;
    pBlob = new Kompex::SQLiteBlob(pDatabase,"main","tiles","png",
                                   tileIdx,Kompex::BLOB_READONLY);

    int blobSize = pBlob->GetBlobSize();
    char * readBuffer = new char[blobSize];
    pBlob->ReadBlob(readBuffer,blobSize);

    QImage tileImage = QImage::fromData((uchar*)readBuffer,blobSize);
    delete readBuffer;
    delete pBlob;

    // sample pixel
    QColor pixelColor(tileImage.pixel(pixel_x,pixel_y));
    QString colorName = pixelColor.name();      // #RRGGBB
    colorName.remove(0,1);                      // remove '#'
    QString dbIdx = QString::number(colorName.toInt(&opOk,16),10);

    qDebug() << "Pixel Color: " << colorName;
    qDebug() << "Pixel Value: " << dbIdx;


    // lookup database entry
    QString sqlQuery = "SELECT * FROM admin1 WHERE id="+dbIdx+";";
    pStmt->Sql(sqlQuery.toStdString());

    QString admin1_name = "N/A";
    bool admin1_disp = false;

    QString admin0_name = "N/A";
    QString sov_name = "N/A";

    if(pStmt->FetchRow())   {
        // get admin1 data
        admin1_name = QString::fromStdString(pStmt->GetColumnString("name"));
        admin1_disp = pStmt->GetColumnBool("disputed");
        int admin0_idx = pStmt->GetColumnInt("admin0");
        int sov_idx = pStmt->GetColumnInt("sov");
        pStmt->FreeQuery();

        // get admin0 data
        if(admin0_idx >= 0)   {
            sqlQuery = QString("SELECT * FROM admin0 WHERE id="+
                               QString::number(admin0_idx,10)+";");
            pStmt->Sql(sqlQuery.toStdString());

            if(pStmt->FetchRow())   {
                admin0_name = QString::fromStdString(pStmt->GetColumnString("name"));
            }
            pStmt->FreeQuery();
        }

        // get sov data
        if(sov_idx >= 0)   {
            sqlQuery = QString("SELECT * FROM sov WHERE id="+
                               QString::number(sov_idx,10)+";");
            pStmt->Sql(sqlQuery.toStdString());

            if(pStmt->FetchRow())   {
                sov_name = QString::fromStdString(pStmt->GetColumnString("name"));
            }
            pStmt->FreeQuery();
        }

        qDebug() << "Admin1: " << admin1_name;
        qDebug() << "Admin0: " << admin0_name;
        qDebug() << "Sov: " << sov_name;
        qDebug() << "Disputed: " << admin1_disp;
    }
    else   {
        pStmt->FreeQuery();
        qDebug() << "INFO: Nothing found at input coordinates";
    }

    return 0;
}


























