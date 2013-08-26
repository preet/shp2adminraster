shp2adminraster
===============
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

Dependencies
------------
 * Qt4+ with PNG Support
 * shapelib
 * kompex sqlite wrapper
 * optipng (if pngs are to be optimized)

inputs: admin0 shapefile v2.0.0 from Natural Earth Data, 
        admin1 shapefile v2.0.0 from Natural Earth Data
        
outputs: adminraster.sqlite
