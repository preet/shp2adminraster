#shp2adminraster
This tool takes the Admin0 and Admin1 shape files available
from NaturalEarthData (http://www.naturalearthdata.com) and
converts the data into a SQLite database that can be used to
perform a rough lookup of administrative regions given a
latitude and longitude as input. For example, a lat/lon input 
of (43.7 N, 79.4 W) should output Ontario and Canada. 

An effort is made to note disputed territories based on the 
data provided by NaturalEarthData but no guarantees as to the 
accuracy of the data is provided. This application should not
be used in situations where a highly accurate lookup of admin 
regions is required, especially in disputed regions.

Input lookup is done by rasterizing the NaturalEarthData
vector files into color-coded PNGs and storing them as blobs
within an sqlite database. The rasterization provides a
resolution of 100px/degree longitude or latitude.

##Dependencies
 * Qt4+ with PNG and ICU support
 * shapelib
 * kompex sqlite wrapper
 * optipng (if pngs are to be optimized)

##Inputs
* admin0 shapefile v2.0.0 from Natural Earth Data, 
* admin1 shapefile v2.0.0 from Natural Earth Data
* (optional) admin1 name substitutions
        
##Outputs
* adminraster.sqlite

##Notes
###Shapefiles
* Use the shapefiles provided in this repo. This tool assumes the shapefile attribute names and rows match up correctly, but file names and content may change as NaturalEarthData is updated.
###Encoding
* NaturalEarthData shapefiles are currently encoded in WINDOWS-1252 (http://www.naturalearthdata.com/features/). It seems like some of the characters in the dataset are not supported with this encoding and they show up as '?' or other substitute characters when viewed. I don't know how to address this outside of manually substituting valid data. Hopefully in the future, NaturalEarthData shapefiles will ship with UTF-8 encoding.

* This tool converts all input text data to UTF-8 before its saved to the SQLite database.
 

###Translation
* There is a translation file (ne_10m_admin_1_states_provinces_shp_translations.dat) included that replaces some administrative region names with English (Latin character) translations. The translations will be used over the existing native
names if the translation file is placed in the admin1 directory.

###Optimization
* OptiPNG is used to optionally losslessly compress the generated image tiles. This substantially reduces the file size of the generated database.
