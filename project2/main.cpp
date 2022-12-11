#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/intersections.h>
#include <CGAL/Polygon_2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>

#include "sharedLib.h"
#include "incrementalAlgorithm.h"
#include "convexHullAlgorithm.h"

typedef Polygon_2::Segment_2 Segment_2;

class PathEdge
{
public:
    Polygon_2 path;
    Segment_2 edge;
    int deltaArea;
};

void findPaths(Polygon_2 A, std::vector<Polygon_2> &paths, int k, Segment_2 edge);
void printPaths(std::vector<Polygon_2> paths);
bool isEdgePartOfPath(Polygon_2 path, Segment_2 edge);

int main(int argc, char **argv)
{

    if (argc < 11)
    {
        std::cout << "Not enough arguments" << std::endl;
        return ERROR;
    }
    std::string inputFile = argv[2];
    std::string outputFile = argv[4];
    std::string algorithm = argv[6];
    int edgeSelection = atoi(argv[8]);
    if (edgeSelection != 1 && edgeSelection != 2 && edgeSelection != 3)
    {
        std::cout << "Invalid edge selection algorithm. Please choose 1, 2 or 3." << std::endl;
        return ERROR;
    }
    std::string initialization = argv[10];
    if (initialization != "1a" && initialization != "1b" && initialization != "2a" && initialization != "2b")
    {
        std::cout << "Invalid initialization algorithm. Please choose 1a, 1b, 2a or 2b." << std::endl;
        return ERROR;
    }
    int onionInitialization = atoi(argv[12]);

    srand(time(0));

    std::ifstream input(inputFile);
    if (!input)
    {
        std::cout << "Could not open file " << inputFile << std::endl;
        return ERROR;
    }

    // Ignore first two lines
    std::string line;
    std::getline(input, line);
    std::getline(input, line);

    Polygon_2 polygon;

    // Loop to read inputFile line by line
    while (std::getline(input, line))
    {
        std::vector<std::string> tokens = split(line, '\t');
        polygon.push_back(Point_2(std::stod(tokens[1]), std::stod(tokens[2])));
    }

    // Check the edge selection input if the incremental algorithm is chosen
    if (algorithm == "incremental")
    {
        if (initialization == "1a")
        {
            // Sort points by x coordinate largest to smallest
            std::sort(polygon.begin(), polygon.end(), [](Point_2 p1, Point_2 p2)
                      { return p1.x() > p2.x(); });
        }
        else if (initialization == "1b")
        {
            // Sort points by x coordinate smallest to largest
            std::sort(polygon.begin(), polygon.end(), [](Point_2 p1, Point_2 p2)
                      { return p1.x() < p2.x(); });
        }
        else if (initialization == "2a")
        {
            // Sort points by y coordinate largest to smallest
            std::sort(polygon.begin(), polygon.end(), [](Point_2 p1, Point_2 p2)
                      { return p1.y() > p2.y(); });
        }
        else if (initialization == "2b")
        {
            // Sort points by y coordinate smallest to largest
            std::sort(polygon.begin(), polygon.end(), [](Point_2 p1, Point_2 p2)
                      { return p1.y() < p2.y(); });
        }
    }

    bool isMaximization; 
    // if(argv[10] == "-max")
    //     isMaximization = true;
    // else if(argv[10] == "-min")
    //     isMaximization = false;
    // else
    // {
    //     std::cout << "Invalid argument for maximization or minimization. Please choose -max or -min." << std::endl;
    //     return ERROR;
    // }
    isMaximization = false;


    // Length k ≤ 10 and threshold are user-defined parameters.
    int k = 10;
    int threshold = -10;
    int deltaArea = -100;

    // Initialize: Obtain (greedy) solution S; T ← ∅
    Polygon_2 A, saveA;
    int constructionTime = 0;

    // Create the polygon A according to the algorithm chosen by the user
    A = convexHullAlgorithm(polygon, edgeSelection, constructionTime);
    std::cout << "------------------A--------------------" << std::endl;
    printPolygon(A);
    std::cout << "--------------------------------------" << std::endl;

    saveA = A;

    if (!A.is_simple())
    {
        std::cout << "Polygon A is not simple" << std::endl;
        return ERROR;
    }
    else
        std::cout << "Polygon A is  simple" << std::endl;

    // Polygon_2 B;

    // B.push_back(Point_2(0, 0));
    // B.push_back(Point_2(0, 100));
    // B.push_back(Point_2(100, 100));
    // B.push_back(Point_2(100, 0));

    // Segment_2 edge(Point_2(0, 0), Point_2(0, 100));

    // std::vector<Polygon_2> paths;
    // findPaths(B, paths, k, edge);
    // printPaths(paths);

    // edge = Segment_2(Point_2(100, 100), Point_2(100, 0));
    // paths.clear();
    // findPaths(B, paths, k, edge);
    // printPaths(paths);

    // while ∆A ≥ threshold do
    while (1)
    {
        if(isMaximization)
        {
            if(deltaArea < threshold)
            {

                std::cout << "Best maximization solution found" << std::endl;
                break;
            }
        }
        else
        {
            if(deltaArea > threshold)
            {
                std::cout << "Best minimization solution found" << std::endl;
                break;
            }
        }
        
        int area = std::abs(A.area());

        std::vector<PathEdge> T;
        // for every edge e ∈ S do
        for (auto e = A.edges_begin(); e != A.edges_end(); e++)
        {
            std::vector<Polygon_2> paths;
            // for every path V of length ≤ k do
            findPaths(A, paths, k, *e);
            for (auto path : paths)
            {
                // B is our testing polygon
                Polygon_2 B = A;
                // if V moving to e increases area and retains simplicity then

               
                // Remove path from B
                for (auto p = path.vertices_begin(); p != path.vertices_end(); p++)
                    B.erase(std::find(B.begin(), B.end(), *p));

                // Add path between e.begin() and e.end()
                int indexSource = findPolygonPoint(B, e->source());
                int indexTarget = findPolygonPoint(B, e->target());
                if (indexSource < indexTarget)
                {
                    for (auto p = path.vertices_begin(); p != path.vertices_end(); p++)
                        B.insert(B.vertices_begin() + indexTarget, *p);
                }
                else
                {
                    for (auto p = path.vertices_begin(); p != path.vertices_end(); p++)
                        B.insert(B.vertices_begin() + indexSource, *p);
                }

                // If it retains simplicity then we add it to T
                if (B.is_simple())
                {
                    Polygon_2 addedPolygon;

                    int indexPreviousVertex = findPolygonPoint(A, path[0]);
                    if (indexPreviousVertex == 0)
                        indexPreviousVertex = A.size() - 1;
                    else
                        indexPreviousVertex--;

                    int indexNextVertex = findPolygonPoint(A, path[path.size() - 1]);

                    if (indexNextVertex == A.size() - 1)
                        indexNextVertex = 0;
                    else
                        indexNextVertex++;

                    // is indexPreviousVertex a point of the path
                    bool isInPath = false;
                    for (auto point : path)
                    {
                        if (point == A[indexPreviousVertex])
                        {
                            isInPath = true;
                            std::cout << "Previous vertex is in path" << std::endl;
                            break;
                        }

                        if(point == A[indexNextVertex])
                        {
                            isInPath = true;
                            std::cout << "Next vertex is in path" << std::endl;
                            break;
                        }
                    }

                    // Add previous vertex, path and next vertex to the addedPolygon
                    addedPolygon.push_back(A[indexPreviousVertex]);
                    for (auto point : path)
                        addedPolygon.push_back(point);
                    addedPolygon.push_back(A[indexNextVertex]);

                    // Calculate the addedPolygon area
                    int addedArea = std::abs(addedPolygon.area());

                    Polygon_2 removedPolygon;
                    // Add e.source(), the path and e.target() to removedArea
                    removedPolygon.push_back(e->source());
                    for (auto p = path.vertices_begin(); p != path.vertices_end(); p++)
                        removedPolygon.push_back(*p);

                    removedPolygon.push_back(e->target());
                    // Check if the removedPolygon is simple
                    if(!addedPolygon.is_simple())
                        std::cout << "Removed polygon is not simple" << std::endl;

                    // Calculate the removedPolygon area
                    int removedPolygonArea = std::abs(removedPolygon.area());

                    // Calculate the difference between the added and removed polygons
                    int delta = addedArea - removedPolygonArea;
                    // B area
                    int Barea = std::abs(B.area());


                    T.push_back(PathEdge{path, *e, Barea - area});
                } // end if
            }     // end for
        } // end for


        // Sort T by delta in descending order
        std::sort(T.begin(), T.end(), [](PathEdge p1, PathEdge p2)
                  { return p1.deltaArea > p2.deltaArea; });


        // Apply all changes in T to S
        if (!T.empty())
        {
            // Choose the path with the highest delta
            PathEdge chosenPEdge = T[0];
            
            // Choose the path with the lowest delta if we are minimizing
            if(!isMaximization)
            {
                chosenPEdge = T[T.size() - 1];
            }
            
            // Remove path from A
            for (auto p = chosenPEdge.path.vertices_begin(); p != chosenPEdge.path.vertices_end(); p++)
                A.erase(std::find(A.begin(), A.end(), *p));

            // Add path between e.begin() and e.end()
            int indexSource = findPolygonPoint(A, chosenPEdge.edge.source());
            int indexTarget = findPolygonPoint(A, chosenPEdge.edge.target());

            if (indexSource < indexTarget)
            {
                for (auto p = chosenPEdge.path.vertices_begin(); p != chosenPEdge.path.vertices_end(); p++)
                    A.insert(A.vertices_begin() + indexTarget, *p);
            }
            else
            {
                for (auto p = chosenPEdge.path.vertices_begin(); p != chosenPEdge.path.vertices_end(); p++)
                    A.insert(A.vertices_begin() + indexSource, *p);
            }

            // Keep best solution S′; ∆A ← Area(S′) − Area(S)
            // Update area - Area already calculated when we added the path to T
            deltaArea = chosenPEdge.deltaArea;
        }
        // If no path is found then we stop
        else
            break;
    }// end while

   
    // is A simple?
    std::cout << "AFTER:" << std::abs(A.area()) << ",  BEFORE:" << std::abs(saveA.area()) << std::endl;
    if (!A.is_simple())
        std::cout << "Polygon A is not simple" << std::endl;
    else
        std::cout << "Polygon A is  simple" << std::endl;
    return 0;
}

void findPaths(Polygon_2 A, std::vector<Polygon_2> &paths, int k, Segment_2 edge)
{
    // for every vertex of A
    for (auto v = A.vertices_begin(); v != A.vertices_end(); v++)
    {
        auto saveV = v;
        for (int i = 1; i <= k; i++)
        {
            Polygon_2 path;
            for (int j = 1; j <= i; j++)
            {
                path.push_back(*v);
                v++;
                if (v == A.vertices_end())
                    v = A.vertices_begin();
            }

            v = saveV;

            //  if the path includes the edge, ignore it
            if (isEdgePartOfPath(path, edge))
                continue;

            paths.push_back(path);
        }
    }
}

bool isEdgePartOfPath(Polygon_2 path, Segment_2 edge)
{
    for (auto p = path.vertices_begin(); p != path.vertices_end(); p++)
    {
        if (*p == edge.source() || *p == edge.target())
            return true;
    }
    return false;
}

void printPaths(std::vector<Polygon_2> paths)
{
    int i = 1;
    for (auto path : paths)
    {

        std::cout << "Path " << i << ":" << std::endl;
        for (auto point : path)
        {
            std::cout << point << std::endl;
        }
        i++;
    }
}