enum IntersectionType
{
    XSEC_COINCIDENT,
    XSEC_PARALLEL,
    XSEC_TRUE,
    XSEC_FALSE
};

IntersectionType calcLinesIntersect(double a_x1, double a_y1,
                                    double a_x2, double a_y2,
                                    double b_x1, double b_y1,
                                    double b_x2, double b_y2,
                                    double &i_x1, double &i_y1)
{
    double ua_numr = (b_x2-b_x1)*(a_y1-b_y1)-(b_y2-b_y1)*(a_x1-b_x1);
    double ub_numr = (a_x2-a_x1)*(a_y1-b_y1)-(a_y2-a_y1)*(a_x1-b_x1);
    double denr = (b_y2-b_y1)*(a_x2-a_x1)-(b_x2-b_x1)*(a_y2-a_y1);

    if(denr == 0.0)
    {
        // lines are coincident
        if(ua_numr == 0.0 && ub_numr == 0.0)
        {   return XSEC_COINCIDENT;   }

        // lines are parallel
        else
        {   return XSEC_PARALLEL;   }
    }

    double ua = ua_numr/denr;
    double ub = ub_numr/denr;

    if(ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0)
    {
        i_x1 = a_x1+ua*(a_x2-a_x1);
        i_y1 = a_y1+ua*(a_y2-a_y1);
        return XSEC_TRUE;
    }

    return XSEC_FALSE;
}

bool getPointInPoly(QList<Vec2d> const &listVertices,
                    Vec2d &pip)
{
    // ref: http://mathoverflow.net/questions/
    //      56655/get-a-point-inside-a-polygon

    // find bounding box
    double minx = listVertices[0].x;
    double maxx = listVertices[0].x;
    double miny = listVertices[0].y;
    double maxy = listVertices[0].y;
    for(int i=1; i < listVertices.size(); i++)   {
        minx = std::min(minx,listVertices[i].x);
        maxx = std::max(maxx,listVertices[i].x);
        miny = std::min(miny,listVertices[i].y);
        maxy = std::max(maxy,listVertices[i].y);
    }

    double diag2 = sqrt((maxx-minx)*(maxx-minx) +
                       (maxy-miny)*(maxy-miny)) * 2;

    // let point P0 be a point outside
    // the bounding box
    Vec2d p0(minx-diag2,miny-diag2);


    QList<Vec2d> listVx = listVertices;
    listVx.push_back(listVx[0]); // wrap around

    for(int e=1; e < listVx.size(); e++)
    {
        // let point M be the midpoint of a
        // given edge of the polygon
        Vec2d m((listVx[e-1].x + listVx[e].x)/2.0,
                (listVx[e-1].y + listVx[e].y)/2.0);

        // let P0M represent the direction vector
        // from P0 to M
        Vec2d p0m(m.x-p0.x,m.y-p0.y);

        // let P0P1 represent a line through
        // the polygon with direction vector
        // P0M
        Vec2d p1(m.x+p0m.x,m.y+p0m.y);

        // test P0P1 against all the edges of
        // the polygon
        QMap<double,Vec2d> xsecPointsByDist;
        for(int i=1; i < listVx.size(); i++)   {
            Vec2d xsecPoint;
            IntersectionType xsecType =
                    calcLinesIntersect(listVx[i].x,listVx[i].y,
                                       listVx[i-1].x,listVx[i-1].y,
                                       p0.x,p0.y,
                                       p1.x,p1.y,
                                       xsecPoint.x,xsecPoint.y);

            if(xsecType == XSEC_TRUE)   {
                double dx = p0.x-xsecPoint.x;
                double dy = p0.y-xsecPoint.y;
                double dist = sqrt((dx*dx)+(dy*dy));
                xsecPointsByDist.insert(dist,xsecPoint);
            }
        }

        if(xsecPointsByDist.size() < 2)   {
            // we should never get here unless there
            // were floating point issues or the poly
            // edge was parallel to p0, so try another
            // edge by continuing
            continue;
        }
        else   {
            // the midpoint of the first two intersection
            // points should be a point inside the poly
            QMap<double,Vec2d>::iterator it = xsecPointsByDist.begin();
            Vec2d xs1 = it.value();
            ++it;
            Vec2d xs2 = it.value();
            pip.x = (xs1.x + xs2.x)/2.0;
            pip.y = (xs2.y + xs2.y)/2.0;
            return true;
        }
    }
    return false;
}
