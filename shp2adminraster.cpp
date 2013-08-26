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
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QBuffer>

// shapelib
#include "shapefil.h"

// kompex
#include "KompexSQLitePrerequisites.h"
#include "KompexSQLiteDatabase.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "KompexSQLiteBlob.h"

bool g_optimize = false;

// 2d vector with color data
struct Vec2d
{
    Vec2d(double sx=0,double sy=0,QColor cy=QColor())
    {   x = sx;   y = sy;   c = cy;   }

    double x;
    double y;
    QColor c;
};

bool getPolysFromShapefile(QString const &fileShp,
                           QList<QList<Vec2d> > &listPolygons)
{
    SHPHandle hSHP = SHPOpen(fileShp.toLocal8Bit().data(),"rb");
    if(hSHP == NULL)   {
        qDebug() << "ERROR: Could not open shape file";
        return false;
    }

    if(hSHP->nShapeType != SHPT_POLYGON)   {
        qDebug() << "ERROR: Wrong shape file type:";
        qDebug() << "ERROR: Expected POLYGON";
        return false;
    }

    size_t nRecords = hSHP->nRecords;
    double xMax = hSHP->adBoundsMax[0];
    double yMax = hSHP->adBoundsMax[1];
    double xMin = hSHP->adBoundsMin[0];
    double yMin = hSHP->adBoundsMin[1];
    qDebug() << "INFO: Bounds: x: " << xMin << "<>" << xMax;
    qDebug() << "INFO: Bounds: y: " << yMin << "<>" << yMax;
    qDebug() << "INFO: Found " << nRecords << "POLYGONS";
    qDebug() << "INFO: Reading in data...";

    qDebug() << "INFO: Creating color list...";
    QStringList listColors;
    for(size_t i=0; i < nRecords; i++)   {
        QString strColor = QString::number(i,16);
        while(strColor.length() < 6)   {    // pad with zeros
            strColor.prepend("0");
        }
        strColor.prepend("#");
        listColors.push_back(strColor);
    }

    // create a list of polygons we can paint
    SHPObject * pSHPObj;

    for(size_t i=0; i < nRecords; i++)
    {   // for each object
        pSHPObj = SHPReadObject(hSHP,i);
        size_t nParts = pSHPObj->nParts;

        // build a list of start and end pts
        QList<int> listStartPts;
        QList<int> listEndB4Pts;
        for(size_t j=0; j < nParts-1; j++)   {
            listStartPts.push_back(pSHPObj->panPartStart[j]);
            listEndB4Pts.push_back(pSHPObj->panPartStart[j+1]);
        }
        listStartPts.push_back(pSHPObj->panPartStart[nParts-1]);
        listEndB4Pts.push_back(pSHPObj->nVertices);

        // build polys from start/end pts
        for(int j=0; j < listStartPts.size(); j++)
        {
            QList<Vec2d> listPts;
            size_t sIx = listStartPts[j];
            size_t eIx = listEndB4Pts[j];

            for(size_t k=sIx; k < eIx; k++)   {
                listPts.push_back(Vec2d(pSHPObj->padfX[k]+180,
                                       (pSHPObj->padfY[k]-90)*-1,
                                        QColor(listColors[i])));
            }
            listPolygons.push_back(listPts);
        }
        SHPDestroyObject(pSHPObj);
    }
    SHPClose(hSHP);

    return true;
}

bool rasterizePolygons(QString const &outputFolder,
                       QList<QList<Vec2d> > const &listPolygons)
{
    qDebug() << "INFO: Rasterizing polys to images in" << outputFolder;
    int kSzMult=100;

    QImage shImage1(180*kSzMult,180*kSzMult,
                   QImage::Format_RGB888);

    QImage shImage2(180*kSzMult,180*kSzMult,
                   QImage::Format_RGB888);

    shImage1.fill(Qt::white);
    shImage2.fill(Qt::white);

    // setup painter
    QPainter shPainter;
    QBrush shBrush(Qt::blue);

    // draw polys on image 1
    shPainter.begin(&shImage1);
    shPainter.setPen(Qt::NoPen);
    for(int i=0; i < listPolygons.size(); i++)
    {
        QPainterPath pPath;
        pPath.setFillRule(Qt::WindingFill);
        pPath.moveTo(listPolygons[i][0].x*kSzMult,
                     listPolygons[i][0].y*kSzMult);

        for(int j=0; j < listPolygons[i].size(); j++)   {
            pPath.lineTo(listPolygons[i][j].x*kSzMult,
                         listPolygons[i][j].y*kSzMult);
        }
        pPath.closeSubpath();

        shBrush.setColor(listPolygons[i][0].c);
        shPainter.setBrush(shBrush);
        shPainter.drawPath(pPath);
    }
    shPainter.end();

    // draw polys on image2
    shPainter.begin(&shImage2);
    shPainter.setPen(Qt::NoPen);
    for(int i=0; i < listPolygons.size(); i++)
    {
        QPainterPath pPath;
        pPath.setFillRule(Qt::WindingFill);
        pPath.moveTo(listPolygons[i][0].x*kSzMult - 180*kSzMult,
                     listPolygons[i][0].y*kSzMult);

        for(int j=0; j < listPolygons[i].size(); j++)   {
            pPath.lineTo(listPolygons[i][j].x*kSzMult - 180*kSzMult,
                         listPolygons[i][j].y*kSzMult);
        }
        pPath.closeSubpath();

        shBrush.setColor(listPolygons[i][0].c);
        shPainter.setBrush(shBrush);
        shPainter.drawPath(pPath);
    }
    shPainter.end();

    QString fname1 = outputFolder + "/imgW.png";
    QString fname2 = outputFolder + "/imgE.png";

    if(shImage1.save(fname1) && shImage2.save(fname2))   {
        qDebug() << "INFO: Saved images as" << fname1
                 << "and" << fname2;
        return true;
    }

    qDebug() << "ERROR: Could not save images";
    return false;
}

bool splitImageIntoTiles(QImage const &img,
                         QString const &pathTiles,
                         QStringList &listTileFiles)
{
    QString prefix = "tile_";
    QString postfix = ".png";

    if(pathTiles.at(pathTiles.size()-1) != '/')   {
        prefix.prepend("/");
    }

    size_t idx=0;
    for(size_t i=0; i < 18; i++)   {        // along y (lat)
        for(size_t j=0; j < 18; j++)   {    // along x (lon)
            int xMin = (j)*1000;
            int yMin = (i)*1000;
            QImage tile = img.copy(xMin,yMin,1000,1000);
            QString filename = pathTiles + prefix +
                    QString::number(idx,10) + postfix;

            if(!tile.save(filename))   {
                return false;
            }

            // optimize
            if(g_optimize)   {
                QString syscmd = "optipng -silent " + filename;
                if(system(syscmd.toLocal8Bit().data()) < 0)   {
                    qDebug() << "WARN: Failed to optimize" << filename;
                }
            }

            idx++;
            listTileFiles.push_back(filename);
            qDebug() << "INFO: Wrote" << idx << "of 324 tiles";
        }
    }
    return true;
}

bool writeTilesToDatabase(QStringList const &listTileFiles,
                          Kompex::SQLiteStatement * pStmt)
{

    for(int i=0; i < listTileFiles.size(); i++)   {

        QFile tileFile(listTileFiles[i]);
        if(!tileFile.open(QIODevice::ReadOnly))   {
            qDebug() << "ERROR: Could not open tile file"
                     << listTileFiles[i];
            return false;
        }

        QByteArray pngBlob = tileFile.readAll();
        try   {
            pStmt->Sql("INSERT INTO tiles(id,png) VALUES(?,?)");
            pStmt->BindInt(1,i);
            pStmt->BindBlob(2,pngBlob.data(),pngBlob.size());
            pStmt->ExecuteAndFree();
        }
        catch(Kompex::SQLiteException &exception)   {
            qDebug() << "ERROR: SQLite exception writing tile data:"
                     << QString::fromStdString(exception.GetString());
            return false;
        }
    }
    return true;
}

bool writeAdminRegionsToDatabase(QString const &a0_dbf,
                                 QString const &a1_dbf,
                                 Kompex::SQLiteStatement * pStmt)
{
    // populate the admin0 temp table
    {
        DBFHandle a0_hDBF = DBFOpen(a0_dbf.toLocal8Bit().data(),"rb");
        if(a0_hDBF == NULL)   {
            qDebug() << "ERROR: Could not open admin0 dbf file";
            return -1;
        }

        size_t a0_numRecords = DBFGetRecordCount(a0_hDBF);
        if(a0_numRecords == 0)   {
            qDebug() << "ERROR: admin0 dbf file has no records!";
            return -1;
        }

        size_t a0_idx_adm_name  = DBFGetFieldIndex(a0_hDBF,"name");
        size_t a0_idx_adm_a3    = DBFGetFieldIndex(a0_hDBF,"adm0_a3");
        size_t a0_idx_sov_name  = DBFGetFieldIndex(a0_hDBF,"sovereignt");
        size_t a0_idx_sov_a3    = DBFGetFieldIndex(a0_hDBF,"sov_a3");
        size_t a0_idx_type      = DBFGetFieldIndex(a0_hDBF,"type");
        size_t a0_idx_note      = DBFGetFieldIndex(a0_hDBF,"note_adm0");

        // create a temporary table we can use to lookup
        // the admin0 data we want for each admin1 entry
        pStmt->SqlStatement("CREATE TABLE IF NOT EXISTS temp("
                            "id INTEGER PRIMARY KEY NOT NULL UNIQUE,"
                            "adm_a3 TEXT NOT NULL UNIQUE,"
                            "sov_a3 TEXT NOT NULL,"
                            "adm_name TEXT,"
                            "sov_name TEXT,"
                            "type TEXT,"
                            "note TEXT);");

        pStmt->BeginTransaction();
        for(size_t i=0; i < a0_numRecords; i++)   {
            QString s0 = QString::number(i,10);
            QString s1(DBFReadStringAttribute(a0_hDBF, i, a0_idx_adm_a3));
            QString s2(DBFReadStringAttribute(a0_hDBF, i, a0_idx_sov_a3));
            QString s3(DBFReadStringAttribute(a0_hDBF, i, a0_idx_adm_name));
            QString s4(DBFReadStringAttribute(a0_hDBF, i, a0_idx_sov_name));
            QString s5(DBFReadStringAttribute(a0_hDBF, i, a0_idx_type));
            QString s6(DBFReadStringAttribute(a0_hDBF, i, a0_idx_note));

            QString stmt("INSERT INTO temp("
                         "id,"
                         "adm_a3,"
                         "sov_a3,"
                         "adm_name,"
                         "sov_name,"
                         "type,"
                         "note) VALUES(" +
                         s0 + "," +
                         "\"" + s1 + "\","
                         "\"" + s2 + "\","
                         "\"" + s3 + "\","
                         "\"" + s4 + "\","
                         "\"" + s5 + "\","
                         "\"" + s6 + "\");");

            pStmt->SqlStatement(stmt.toStdString());
        }
        pStmt->CommitTransaction();
        DBFClose(a0_hDBF);
    }

    // populate the admin1 table
    {
        DBFHandle a1_hDBF = DBFOpen(a1_dbf.toLocal8Bit().data(),"rb");
        if(a1_hDBF == NULL)   {
            qDebug() << "ERROR: Could not open admin1 dbf file";
            return -1;
        }

        size_t a1_numRecords = DBFGetRecordCount(a1_hDBF);
        if(a1_numRecords == 0)   {
            qDebug() << "ERROR: admin1 dbf file has no records!";
            return -1;
        }

        size_t a1_idx_adm_name  = DBFGetFieldIndex(a1_hDBF,"name");
        size_t a1_idx_adm_a3    = DBFGetFieldIndex(a1_hDBF,"sr_adm0_a3");
        size_t a1_idx_sov_a3    = DBFGetFieldIndex(a1_hDBF,"sr_sov_a3");
        size_t a1_idx_fclass    = DBFGetFieldIndex(a1_hDBF,"featurecla");
        size_t a1_idx_adminname = DBFGetFieldIndex(a1_hDBF,"admin");

        QStringList listSqlSaveSov;
        QStringList listSqlSaveAdmin0;
        QStringList listSqlSaveAdmin1;

        for(size_t i=0; i < a1_numRecords; i++)   {
            // get the name of this admin1 entry
            QString admin1_idx = QString::number(i,10);
            QString admin1_name(DBFReadStringAttribute(a1_hDBF, i, a1_idx_adm_name));

            // if the adm1 fclass is an aggregation, minor island or
            // remainder, we grab the name from another field which
            // doesn't contain a bunch of additional metadata
            QString fclass(DBFReadStringAttribute(a1_hDBF, i, a1_idx_fclass));
            if(fclass.contains("aggregation") ||
               fclass.contains("minor island") ||
               fclass.contains("remainder"))
            {
                admin1_name = QString(DBFReadStringAttribute(
                                   a1_hDBF, i, a1_idx_adminname));
            }

            // get the adm_a3,sov_a3 code for this admin1 entry
            QString adm_a3(DBFReadStringAttribute(a1_hDBF, i, a1_idx_adm_a3));
            QString sov_a3(DBFReadStringAttribute(a1_hDBF, i, a1_idx_sov_a3));

            // check if the adm_a3 code exists in the temp database
            QString stmt("SELECT * FROM temp WHERE adm_a3=\""+ adm_a3 + "\";");
            pStmt->Sql(stmt.toStdString());

            if(pStmt->FetchRow())   {
                // save admin0 info
                QString admin0_idx  = QString::number(pStmt->GetColumnInt("id"),10);
                QString admin0_type = QString::fromStdString(pStmt->GetColumnString("type"));
                QString admin0_note = QString::fromStdString(pStmt->GetColumnString("note"));
                pStmt->FreeQuery();

                // save admin1 info; we currently derive the
                // disputed field from admin0 type and note fields
                QString admin1_disputed = "0";
                if(admin0_type.contains("Disputed") ||
                   admin0_note.contains("Disputed"))   {
                    admin1_disputed = "1";
                }

                stmt = QString("INSERT INTO admin1(id,name,disputed,admin0,sov) VALUES(" +
                               admin1_idx + ",\"" +
                               admin1_name + "\"," +
                               admin1_disputed + "," +
                               admin0_idx + "," +
                               admin0_idx + ");");
                listSqlSaveAdmin1.push_back(stmt);
            }
            else   {
                pStmt->FreeQuery();

                // if there isn't a matching adm_a3 code in the temp
                // database try to get the sovereign state instead
                stmt = QString("SELECT * FROM temp WHERE sov_a3=\""+ sov_a3 + "\";");
                pStmt->Sql(stmt.toStdString());

                if(pStmt->FetchRow())   {
                    QString sov_idx     = QString::number(pStmt->GetColumnInt("id"));
                    pStmt->FreeQuery();

                    // since there's no true corresponding entry for
                    // the admin1 region through the adm_a3 code, we
                    // can't test for disputed regions
                    QString admin1_disputed = "0";

                    // to indicate that no admin0 region data exists
                    // for this entry, we use an index of value -1
                    QString admin0_idx = "-1";

                    stmt = QString("INSERT INTO admin1(id,name,disputed,admin0,sov) VALUES(" +
                                   admin1_idx + ",\"" +
                                   admin1_name + "\"," +
                                   admin1_disputed + "," +
                                   admin0_idx + "," +
                                   sov_idx + ");");
                    listSqlSaveAdmin1.push_back(stmt);
                }
                else   {
                    // fail without a matching adm_a3 or sov_a3
                    pStmt->FreeQuery();
                    return false;
                }
            }
        }

        // populate the admin0 and sov tables
        {
            QString stmt("SELECT * FROM temp;");
            pStmt->Sql(stmt.toStdString());

            while(pStmt->FetchRow())   {
                QString idx         = QString::number(pStmt->GetColumnInt("id"),10);
                QString admin0_name = QString::fromStdString(pStmt->GetColumnString("adm_name"));
                QString sov_name    = QString::fromStdString(pStmt->GetColumnString("sov_name"));

                stmt = QString("INSERT INTO sov(id,name) VALUES("+
                               idx + ",\"" +
                               sov_name + "\");");
                listSqlSaveSov.push_back(stmt);

                stmt = QString("INSERT INTO admin0(id,name) VALUES("+
                               idx + ",\"" +
                               admin0_name + "\");");
                listSqlSaveAdmin0.push_back(stmt);
            }
            pStmt->FreeQuery();
        }

        // write prepared statements
        pStmt->BeginTransaction();
        for(int i=0; i < listSqlSaveSov.size(); i++)   {
            pStmt->SqlStatement(listSqlSaveSov[i].toStdString());
        }
        pStmt->CommitTransaction();

        pStmt->BeginTransaction();
        for(int i=0; i < listSqlSaveAdmin0.size(); i++)   {
             pStmt->SqlStatement(listSqlSaveAdmin0[i].toStdString());
        }
        pStmt->CommitTransaction();

        pStmt->BeginTransaction();
        for(int i=0; i < listSqlSaveAdmin1.size(); i++)   {
            pStmt->SqlStatement(listSqlSaveAdmin1[i].toStdString());
        }
        pStmt->CommitTransaction();
    }

    // delete temp table
    pStmt->SqlStatement("DROP TABLE temp;");

    return true;
}

void badInput()
{
    qDebug() << "ERROR: Wrong number of arguments: ";
    qDebug() << "* Pass the directories containing the admin shapefiles. ";
    qDebug() << "* Each set of shapefiles should be in different directories. ";
    qDebug() << "* Pass in an -optimize flag after specifying the directories ";
    qDebug() << "  to optimize PNG files using OptiPNG (recommended!)";
    qDebug() << "ex:";
    qDebug() << "./shp2adminraster /admin0shapefiles /admin1shapefiles -optimize";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);
    QString pathApp = QCoreApplication::applicationDirPath();

    // check input args
    QStringList inputArgs = app.arguments();
    if(inputArgs.size() < 3)   {
        badInput();
        return -1;
    }
    if(inputArgs[1] == inputArgs[2])   {
        badInput();
        return -1;
    }
    if(inputArgs.size() == 4)   {
        if(inputArgs[3] == "-optimize")   {
            g_optimize = true;
        }
    }

    QDir appDir(pathApp);

    // filter shapefile types
    QStringList filterList;
    filterList << "*.shp" << "*.shx" << "*.dbf" << "*.prj";

    // get shapefiles for admin1
    QDir a1_dir = inputArgs[2];
    QString a1_fileShx,a1_fileShp,a1_fileDbf,a1_filePrj;
    QStringList a1_dirList = a1_dir.entryList(filterList,QDir::Files);

    for(int i=0; i < a1_dirList.size(); i++)   {
        if(a1_dirList[i].contains(".shp"))   {
            a1_fileShp = a1_dir.absoluteFilePath(a1_dirList[i]);
        }
        else if(a1_dirList[i].contains(".shx"))   {
            a1_fileShx = a1_dir.absoluteFilePath(a1_dirList[i]);
        }
        else if(a1_dirList[i].contains(".dbf"))   {
            a1_fileDbf = a1_dir.absoluteFilePath(a1_dirList[i]);
        }
        else if(a1_dirList[i].contains(".prj"))   {
            a1_filePrj = a1_dir.absoluteFilePath(a1_dirList[i]);
        }
    }

    // get dbf file for admin0
    QDir a0_dir = inputArgs[1];
    QString a0_fileDbf;
    QStringList a0_dirList = a0_dir.entryList(QDir::Files);
    for(int i=0; i < a0_dirList.size(); i++)   {
        if(a0_dirList[i].contains(".dbf"))   {
            a0_fileDbf = a0_dir.absoluteFilePath(a0_dirList[i]);
        }
    }

    // get polygons from admin1 shapefile
    QList<QList<Vec2d> > list_a1_polys;

    if(!getPolysFromShapefile(a1_fileShp,list_a1_polys))   {
        return -1;
    }

    // save admin1 polys as east/west images
    appDir.mkpath(pathApp+"/admin1");
    if(!rasterizePolygons(pathApp+"/admin1",list_a1_polys))   {
        return -1;
    }

    // cut images into tiles
    qDebug() << "INFO: Splitting into tiles...";
    QStringList listWestTileFiles;
    QImage * imgWest = new QImage(pathApp+"/admin1/imgW.png");
    appDir.mkpath(pathApp+"/admin1/west");
    if(!splitImageIntoTiles(*imgWest,"admin1/west",listWestTileFiles))   {
        qDebug() << "ERROR: Failed to split image into tiles [west]";
        return -1;
    }
    delete imgWest;

    QStringList listEastTileFiles;
    QImage * imgEast = new QImage(pathApp+"/admin1/imgE.png");
    appDir.mkpath(pathApp+"/admin1/east");
    if(!splitImageIntoTiles(*imgEast,"admin1/east",listEastTileFiles))   {
        qDebug() << "ERROR: Failed to split image into tiles [east]";
        return -1;
    }
    delete imgEast;

    // open new database and create tables
    qDebug() << "INFO: Creating database...";
    Kompex::SQLiteDatabase * pDatabase;
    Kompex::SQLiteStatement * pStmt;

    try   {
        pDatabase = new Kompex::SQLiteDatabase("adminraster.sqlite",
                                               SQLITE_OPEN_READWRITE |
                                               SQLITE_OPEN_CREATE,0);

        pStmt = new Kompex::SQLiteStatement(pDatabase);

        pStmt->SqlStatement("CREATE TABLE IF NOT EXISTS tiles("
                            "id INTEGER PRIMARY KEY NOT NULL UNIQUE,"
                            "png BLOB)");

        pStmt->SqlStatement("CREATE TABLE IF NOT EXISTS admin1("
                            "id INTEGER PRIMARY KEY NOT NULL UNIQUE,"
                            "name TEXT NOT NULL,"
                            "disputed INTEGER NOT NULL,"
                            "admin0 INTEGER,"
                            "sov INTEGER);");

        pStmt->SqlStatement("CREATE TABLE IF NOT EXISTS admin0("
                            "id INTEGER PRIMARY KEY NOT NULL UNIQUE,"
                            "name TEXT NOT NULL);");

        pStmt->SqlStatement("CREATE TABLE IF NOT EXISTS sov("
                            "id INTEGER PRIMARY KEY NOT NULL UNIQUE,"
                            "name TEXT NOT NULL);");
    }
    catch(Kompex::SQLiteException &exception)   {
        qDebug() << "ERROR: SQLite exception creating database:"
                 << QString::fromStdString(exception.GetString());
        return -1;
    }


    // write tile images into database
    qDebug() << "INFO: Writing tiles to database...";
    QStringList listAllTileFiles;
    listAllTileFiles.append(listWestTileFiles);
    listAllTileFiles.append(listEastTileFiles);
    writeTilesToDatabase(listAllTileFiles,pStmt);

    // remove image files
    // system("rm -rf admin1");

    // get records from admin0 and admin1 dbf
    qDebug() << "INFO: Writing admin regions to database...";
    writeAdminRegionsToDatabase(a0_fileDbf,a1_fileDbf,pStmt);


    // clean up database
    delete pStmt;
    delete pDatabase;

    // verify images written successfully [27]
//    qDebug() << "INFO: Test";
//    Kompex::SQLiteBlob * pBlob =
//            new Kompex::SQLiteBlob(pDatabase,"main","tiles","png",0);

//    int blobSize = pBlob->GetBlobSize();
//    char * readBuffer = new char[blobSize];
//    pBlob->ReadBlob(readBuffer,blobSize);
//    QImage temp = QImage::fromData((uchar*)readBuffer,blobSize);
//    if(!temp.save("test.png"))   {
//        qDebug() << "ERROR: failed to save";
//        return -1;
//    }

    //
    return 0;
}










