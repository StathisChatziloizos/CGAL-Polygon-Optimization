#ifndef __CONVEX_HULL_ALGORITHM_CPP__
#define __CONVEX_HULL_ALGORITHM_CPP__

#include "convexHullAlgorithm.h"

Polygon_2 findInternalPoints(Polygon_2 polygon, Polygon_2 A)
{
    Polygon_2 internalPoints;

    // Loop through all points of polygon
    for (auto polygonPoint = polygon.begin(); polygonPoint != polygon.end(); polygonPoint++)
    {
        bool isInside = false;
        // Loop through all points of A
        for (auto APoint = A.begin(); APoint != A.end(); APoint++)
        {
            // If point is both in polygon and A
            if (*polygonPoint == *APoint)
            {
                isInside = true;
                break;
            }
        }
        if (!isInside)
            internalPoints.push_back(*polygonPoint);
    }

    return internalPoints;
}

std::vector<EdgeDistance> findMinEdgeDistances(Polygon_2 A, Polygon_2 internalPoints)
{
    std::vector<std::pair<Polygon_2::Segment_2, Point_2>> minEdgeDistances;

    // Loop through every edge of A
    for (int i = 0; i < A.size(); i++)
    {
        Polygon_2::Segment_2 edge = A.edge(i);

        double minDistance = DBL_MAX;
        std::pair<Polygon_2::Segment_2, Point_2> minEdgeDist;

        // Find the distance from the edge to every point in internalPoints
        for (int j = 0; j < internalPoints.size(); j++)
        {
            Point_2 point = internalPoints[j];

            // Find the distance from the edge to the point
            double distance = CGAL::squared_distance(edge, point);

            if (distance < minDistance)
            {
                minDistance = distance;
                minEdgeDist = std::make_pair(edge, point);
            }
        }
        minEdgeDistances.push_back(minEdgeDist);
    }
    return minEdgeDistances;
}

EdgeDistance findMinTriangleArea(Polygon_2 A, std::vector<EdgeDistance> edgeDistances, std::vector<TriangleArea> &triangleAreas)
{
    double minArea = DBL_MAX;
    EdgeDistance minTriangleAreaDistance;

    // For every minEdgeDistance, find the area of the triangle formed by the edge and the point
    for (int i = 0; i < edgeDistances.size(); i++)
    {
        Polygon_2::Segment_2 edge = edgeDistances[i].first;
        Point_2 point = edgeDistances[i].second;
        // Create a triangle with the edge and the point
        Triangle_2 triangle(edge.source(), edge.target(), point);
        // Find the area of the triangle
        double area = std::abs(triangle.area());

        // Insert the edge and the area into the vector
        TriangleArea triangleArea = std::make_pair(edgeDistances[i], area);
        triangleAreas.push_back(triangleArea);

        if (area < minArea)
        {
            minArea = area;
            minTriangleAreaDistance = edgeDistances[i];
        }
    }

    return minTriangleAreaDistance;
}

Polygon_2 convexHullAlgorithm(Polygon_2 polygon, int edgeSelection, int &constructionTime)
{
    clock_t start = clock();
    clock_t end;

    Polygon_2 A, convexHull;

    // Calculate convex hull of polygon
    CGAL::convex_hull_2(polygon.begin(), polygon.end(), std::back_inserter(convexHull));
    A = convexHull;

    Polygon_2 internalPoints = findInternalPoints(polygon, A);

    int counter = 0;
    while (!internalPoints.is_empty())
    {
        std::vector<EdgeDistance> minEdgeDistances = findMinEdgeDistances(A, internalPoints);

        std::vector<TriangleArea> triangleAreas;
        EdgeDistance minEdgeAreaDistance = findMinTriangleArea(A, minEdgeDistances, triangleAreas);

        // Choose edge randomly
        if (edgeSelection == 1)
        {
            // Create a random engine
            auto rng = std::default_random_engine{};
            // Seed the reandom engine
            rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
            // Shuffle the triangleAreas vector
            std::shuffle(std::begin(triangleAreas), std::end(triangleAreas), rng);
        }
        // Add edge so that min area is added => max area is removed from A
        if (edgeSelection == 2)
        {
            // Sort triangleAreas by area largest to smallest
            std::sort(triangleAreas.begin(), triangleAreas.end(), [](TriangleArea t1, TriangleArea t2)
                      { return t1.second > t2.second; });
        }
        // Add edge so that max area is added => min area is removed from A
        else if (edgeSelection == 3)
        {
            // Sort triangleAreas by area smallest to largest
            std::sort(triangleAreas.begin(), triangleAreas.end(), [](TriangleArea t1, TriangleArea t2)
                      { return t1.second < t2.second; });
        }

        // For every triangle, see if it's edge on the polygon is visible
        // from the third point of the triangle
        for (int i = 0; i < triangleAreas.size(); i++)
        {
            // Make the triangle
            Triangle_2 triangle(triangleAreas[i].first.first.source(), triangleAreas[i].first.first.target(), triangleAreas[i].first.second);

            bool isIntersecting = false;

            // For every edge of the polygon check
            // check if it intersects the triangle
            for (int j = 0; j < A.size(); j++)
            {
                Polygon_2::Segment_2 edge = A.edge(j);
                const auto mutual = CGAL::intersection(triangle, edge);
                if (mutual)
                {
                    // If the intersection is a Segment
                    if (const Polygon_2::Segment_2 *mutualSegment = boost::get<Polygon_2::Segment_2>(&*mutual))
                    {
                        // Check if the mutualSegment is the same as the edge and ignore it
                        if (mutualSegment->source() == edge.source() && mutualSegment->target() == edge.target())
                            continue;
                        else
                        {
                            // Found an intersection Segment, therefore the edge in question,
                            // polygon[i] to polygon[(i + 1) % polygon.size()],  is not visible
                            isIntersecting = true;
                            break;
                        }
                    }
                    else
                        // If the intersection is a point then the polygon and the triangle
                        // share a vertex, therefore the edge in question can still be visible
                        const Point_2 *mutualPoint = boost::get<Point_2>(&*mutual);
                }
            }
            if (!isIntersecting)
            {
                Polygon_2::Segment_2 edge(triangleAreas[i].first.first.source(), triangleAreas[i].first.first.target());
                Point_2 point = triangleAreas[i].first.second;

                int index = findPolygonPoint(A, edge.target());
                A.insert(A.begin() + index, point);
                internalPoints.erase(std::find(internalPoints.begin(), internalPoints.end(), point));
                break;
            }
        }
    }

    // Construction time of incremental algorithm in milliseconds
    end = clock();
    constructionTime = (end - start) / (double)(CLOCKS_PER_SEC / 1000);

    return A;
}

#endif // __CONVEX_HULL_ALGORITHM_CPP__