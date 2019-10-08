#include "m1.h"
#include "m3.h"
#include "StreetsDatabaseAPI.h"
#include <boost/algorithm/string.hpp>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <math.h> 
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/rtree.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using namespace std;

typedef bg::model::point<double, 2, bg::cs::cartesian> point;
typedef std::pair<point, unsigned> value;

//street indices mapped by street name
unordered_map<string, vector<StreetIndex>> streets;

//intersection indices mapped by intersection name
unordered_map<string, vector<IntersectionIndex>> intersections;

//vector of street segment info by street segment index
vector<StreetSegmentInfo> streetSegments;

//vector of street segment indices mapped by street index 
vector<vector<StreetSegmentIndex>> streetStreetSegments;

//vector of street segments by intersection index
vector<vector<StreetSegmentIndex>> intersectionStreetSegments;

//vector of intersections by street index
vector<vector<IntersectionIndex>> streetIntersections;

//vector of street segment lengths
vector<double> streetSegmentLengths;

//multi_map of POI
unordered_multimap<string, unsigned> poi_by_name;

bgi::rtree< value, bgi::quadratic<16> > poiRTree;
bgi::rtree< value, bgi::quadratic<16> > intersectionRTree;





//checks whether or not a map is open
bool isOpen;


struct Node {
    unsigned id;
    vector<StreetSegmentIndex> edge; // connected edges
    vector<double> cost;
    int prevId;
    int prevEdge;
    int prevStreetID;
    bool visited;
    double travelTime;

    Node() {
    }

    Node(unsigned id_, vector<StreetSegmentIndex> edge_, vector<double> cost_) {
        id = id_;
        edge = edge_;
        cost = cost_;
        prevId = -1;
        prevEdge = -1;
        prevStreetID = -1;
        travelTime = -1;
        visited = false;
    }

    bool operator>(const Node& n) const {
        return travelTime > n.travelTime;
    }
};

//graph of all nodes on the map
vector<Node> allNodes;

//function to initialize graph

void initialize_graph() {
    unsigned numIntersection = getNumberOfIntersections();
    for (unsigned i = 0; i < numIntersection; i++) {
        unsigned streetSegCount = getIntersectionStreetSegmentCount(i);
        vector<unsigned> intersectionStreetSeg;
        vector<double> segCost;

        //storing street segments attached to intersection
        for (unsigned j = 0; j < streetSegCount; j++) {
            unsigned streetSeg = getIntersectionStreetSegment(i, j);
            StreetSegmentInfo segInfo = getStreetSegmentInfo(streetSeg);

            //skip segment if not valid one way
            if (segInfo.oneWay == true && i == segInfo.to) {
                continue;
            } else {
                //store street seg into vector
                intersectionStreetSeg.push_back(streetSeg);
                //store cost into vector
                vector<unsigned> temp;
                temp.push_back(streetSeg);
                double cost = compute_path_travel_time(temp, 0);
                segCost.push_back(cost);
            }

        }
        Node n(i, intersectionStreetSeg, segCost);
        allNodes.push_back(n);
    }
}

bool load_map(std::string map_path) {

    isOpen = loadStreetsDatabaseBIN(map_path); //check if this passes. return false and do not close if it fails
    if (isOpen == false)
        return false;

    //Load your map related data structures here

    unsigned numStreetSeg = getNumberOfStreetSegments();
    unsigned numIntersection = getNumberOfIntersections();
    unsigned numPointsOfInterest = getNumberOfPointsOfInterest();
    unsigned numStreet = getNumberOfStreets();

    //create an r-tree and store Points of Interest in it
    for (unsigned i = 0; i < numPointsOfInterest; ++i) {
        LatLon poiPos = getPointOfInterestPosition(i);
        point somePoi(poiPos.lat(), poiPos.lon());
        // insert new value
        poiRTree.insert(std::make_pair(somePoi, i));
    }


    //create an r-tree and store intersections in it
    for (unsigned i = 0; i < numIntersection; ++i) {
        LatLon intersectionPos = getIntersectionPosition(i);
        point someIntersection(intersectionPos.lat(), intersectionPos.lon());
        // insert new value
        intersectionRTree.insert(std::make_pair(someIntersection, i));
    }


    //load streets into data structure
    for (unsigned i = 0; i < numStreet; i++) {
        string currName = getStreetName(i);
        int found = streets.count(currName);
        if (found) {
            streets.at(currName).push_back(i);
        } else {
            vector<unsigned> temp;
            temp.push_back(i);
            streets.insert(pair<string, vector < StreetIndex >> (currName, temp));
        }
    }


    //load intersections into an unordered map
    for (unsigned i = 0; i < numIntersection; i++) {
        string currIntersectionName = getIntersectionName(i);
        int found = intersections.count(currIntersectionName);
        if (found) {
            intersections.at(currIntersectionName).push_back(i);
        } else {
            vector<unsigned> temp;
            temp.push_back(i);
            intersections.insert(pair<string, vector < IntersectionIndex >> (currIntersectionName, temp));
        }
    }


    //load street segment info into vector
    for (unsigned i = 0; i < numStreetSeg; i++) {
        StreetSegmentInfo streetSegInfo = getStreetSegmentInfo(i);
        streetSegments.push_back(streetSegInfo);
    }


    //load street segments indices into vector
    for (unsigned i = 0; i < numStreetSeg; i++) {
        StreetIndex streetInd = streetSegments[i].streetID;
        if (streetInd < streetStreetSegments.size()) {
            streetStreetSegments[streetInd].push_back(i);
        } else {
            vector<unsigned> temp;
            temp.push_back(i);
            streetStreetSegments.push_back(temp);
        }
    }


    //load intersection street segments into vector
    for (unsigned i = 0; i < numIntersection; i++) {
        unsigned streetSegCount = getIntersectionStreetSegmentCount(i);
        vector<unsigned> intersectionStreetSeg;
        for (unsigned j = 0; j < streetSegCount; j++) {
            unsigned streetSeg = getIntersectionStreetSegment(i, j);
            intersectionStreetSeg.push_back(streetSeg);
        }
        intersectionStreetSegments.push_back(intersectionStreetSeg);
    }


    //loads vector of intersections mapped by street index
    for (unsigned i = 0; i < numStreet; i++) {
        vector<unsigned> tempIntersections = find_all_street_intersections(i);
        streetIntersections.push_back(tempIntersections);
    }


    //loads vector of street segment lengths
    for (unsigned i = 0; i < numStreetSeg; i++) {
        double distance = 0;
        StreetSegmentInfo streetSeg = streetSegments[i];
        //if no curve points
        if (streetSeg.curvePointCount == 0) {
            LatLon point1 = getIntersectionPosition(streetSeg.from);
            LatLon point2 = getIntersectionPosition(streetSeg.to);
            distance = find_distance_between_two_points(point1, point2);
        } else {
            //if curve points exist
            LatLon point1 = getIntersectionPosition(streetSeg.from);
            LatLon point2;
            for (unsigned j = 0; j < streetSeg.curvePointCount; j++) {
                point2 = getStreetSegmentCurvePoint(i, j);
                distance += find_distance_between_two_points(point1, point2);
                point1 = point2;
            }
            distance += find_distance_between_two_points(point2, getIntersectionPosition(streetSeg.to));
        }
        streetSegmentLengths.push_back(distance);
    }

    //loads pois into multimap
    for (unsigned i = 0; i < numPointsOfInterest; i++) {
        string currPOIName = getPointOfInterestName(i);
        poi_by_name.insert(pair<string, unsigned> (currPOIName, i));
    }

    initialize_graph();


    return true;
}


//Returns street id(s) for the given street name
//If no street with this name exists, returns a 0-length vector.

std::vector<unsigned> find_street_ids_from_name(std::string street_name) {
    int found = streets.count(street_name);
    if (found) {
        return streets.at(street_name);
    } else {
        vector <unsigned> empty;
        return empty;
    }
}


//Returns the street segments for the given intersection 

std::vector<unsigned> find_intersection_street_segments(unsigned intersection_id) {
    return intersectionStreetSegments[intersection_id];
}


//Returns the street names at the given intersection (includes duplicate street names in returned vector)

std::vector<std::string> find_intersection_street_names(unsigned intersection_id) {
    vector<unsigned> intersectionStreetSeg;
    vector<StreetSegmentInfo> intersectionStreetSegInfo;
    vector<string> intersectionStreetNames;

    intersectionStreetSeg = find_intersection_street_segments(intersection_id); //find segments at intersection
    for (unsigned i = 0; i < intersectionStreetSeg.size(); i++) {
        StreetSegmentInfo ID = streetSegments[intersectionStreetSeg[i]];
        intersectionStreetSegInfo.push_back(ID);
    }
    for (unsigned i = 0; i < intersectionStreetSegInfo.size(); i++) {
        intersectionStreetNames.push_back(getStreetName(intersectionStreetSegInfo[i].streetID)); //push back the name of the street each segment belongs to
    }
    return intersectionStreetNames;
}


//Returns true if you can get from intersection1 to intersection2 using a single street segment (hint: check for 1-way streets too)
//corner case: an intersection is considered to be connected to itsel

bool are_directly_connected(unsigned intersection_id1, unsigned intersection_id2) {
    //connected to itself
    if (intersection_id1 == intersection_id2)
        return true;

    //find all street segments at intersection 1
    std::vector<unsigned> segmentsAtIntersection1 = find_intersection_street_segments(intersection_id1);
    int size = segmentsAtIntersection1.size();

    std::vector<unsigned> adjacentToIntersection1;
    for (int i = 0; i < size; i++)

        //check if current intersection is the from or to, then push the other intersection of the street segment into a vector after checking one way
        //this finds all adjacent intersections
        if (streetSegments[segmentsAtIntersection1[i]].from == intersection_id1) {

            adjacentToIntersection1.push_back(streetSegments[segmentsAtIntersection1[i]].to);

        } else if (streetSegments[segmentsAtIntersection1[i]].to == intersection_id1) {
            if (streetSegments[segmentsAtIntersection1[i]].oneWay == false) {//if not one way, then push into vector
                adjacentToIntersection1.push_back(streetSegments[segmentsAtIntersection1[i]].from);

            }
        }

    //search through all adjacent intersections. If intersection 2 is in the vector, then are_directly_connected = true
    if (std::find(adjacentToIntersection1.begin(), adjacentToIntersection1.end(), intersection_id2) != adjacentToIntersection1.end()) {
        // Found the item
        return true;
    } else
        return false;
}

/*Returns all intersections reachable by traveling down one street segment 
from given intersection (hint: you can't travel the wrong way on a 1-way street)
the returned vector should NOT contain duplicate intersections*/
std::vector<unsigned> find_adjacent_intersections(unsigned intersection_id) {
    std::vector<unsigned> segmentsAtIntersection = find_intersection_street_segments(intersection_id);
    int size = segmentsAtIntersection.size();

    std::vector<unsigned> adjacentIntersections;
    for (int i = 0; i < size; i++)

        //check if current intersection is the from or to, then push the other intersection of the street segment into a vector after checking one way

        if (streetSegments[segmentsAtIntersection[i]].from == intersection_id) {
            //check for duplicates
            if (std::find(adjacentIntersections.begin(), adjacentIntersections.end(), streetSegments[segmentsAtIntersection[i]].to) != adjacentIntersections.end()) {
                // Found the item
            } else {
                adjacentIntersections.push_back(streetSegments[segmentsAtIntersection[i]].to);
            }
        } else if (streetSegments[segmentsAtIntersection[i]].to == intersection_id) {
            if (streetSegments[segmentsAtIntersection[i]].oneWay == false) {//if not one way, then push into vector
                //check for duplicates
                if (std::find(adjacentIntersections.begin(), adjacentIntersections.end(), streetSegments[segmentsAtIntersection[i]].from) != adjacentIntersections.end()) {
                    // Found the item
                } else {
                    adjacentIntersections.push_back(streetSegments[segmentsAtIntersection[i]].from);
                }
            }
        }
    return adjacentIntersections;
}


//Returns all street segments for the given street

std::vector<unsigned> find_street_street_segments(unsigned street_id) {
    return streetStreetSegments[street_id];
}


//Returns all intersections along the a given street

std::vector<unsigned> find_all_street_intersections(unsigned street_id) {
    vector<unsigned> street_intersections;

    //finds all street segments in street
    vector<unsigned> street_segments = streetStreetSegments[street_id];

    for (unsigned i = 0; i < street_segments.size(); i++) {
        //finds intersections connected to a street segment
        unsigned IDto = streetSegments[street_segments[i]].to;
        unsigned IDfrom = streetSegments[street_segments[i]].from;

        //checks for duplicates
        if (find(street_intersections.begin(), street_intersections.end(), IDfrom) == street_intersections.end()) {
            street_intersections.push_back(IDfrom);
        }
        if (find(street_intersections.begin(), street_intersections.end(), IDto) == street_intersections.end()) {
            street_intersections.push_back(IDto);
        }
    }

    return street_intersections;
}

//Return all intersection ids for two intersecting streets
//This function will typically return one intersection id.
//However street names are not guarenteed to be unique, so more than 1 intersection id may exist

std::vector<unsigned> find_intersection_ids_from_street_names(std::string street_name1, std::string street_name2) {
    vector<unsigned> street1 = find_street_ids_from_name(street_name1);
    vector<unsigned> street2 = find_street_ids_from_name(street_name2);
    vector<unsigned> intersectionIDs;
    //find all intersection on street 1
    for (unsigned i = 0; i < street1.size(); i++) {
        vector<unsigned> intersections1 = streetIntersections[street1[i]];
        //find all intersection on street 2
        for (unsigned j = 0; j < street2.size(); j++) {
            vector<unsigned> intersections2 = streetIntersections[street2[j]];
            for (unsigned k = 0; k < intersections1.size(); k++) {
                for (unsigned l = 0; l < intersections2.size(); l++) {
                    if (intersections1[k] == intersections2[l]) {
                        //check which intersections appear in both streets
                        intersectionIDs.push_back(intersections1[k]);
                    }
                }
            }
        }
    }
    return intersectionIDs;
}


//Returns the distance between two coordinates in meters

double find_distance_between_two_points(LatLon point1, LatLon point2) {
    double latAvgInRad = ((point1.lat() * DEG_TO_RAD) + (point2.lat() * DEG_TO_RAD)) / 2;

    std::pair<double, double> point1_xy;
    std::pair<double, double> point2_xy;
    point1_xy.second = point1.lat() * DEG_TO_RAD;
    point2_xy.second = point2.lat() * DEG_TO_RAD;
    point1_xy.first = point1.lon() * DEG_TO_RAD * cos(latAvgInRad);
    point2_xy.first = point2.lon() * DEG_TO_RAD * cos(latAvgInRad);

    double pythag = sqrt(pow((point2_xy.second - point1_xy.second), 2) + pow((point2_xy.first - point1_xy.first), 2));
    double distance = pythag * EARTH_RADIUS_IN_METERS;
    return distance;
}

//Returns the length of the given street segment in meters

double find_street_segment_length(unsigned street_segment_id) {
    return streetSegmentLengths[street_segment_id];
}

//Returns the length of the specified street in meters

double find_street_length(unsigned street_id) {
    double distance = 0;
    //street length is sum of all segment lengths
    for (unsigned i = 0; i < streetStreetSegments[street_id].size(); i++) {
        unsigned streetSegmentID = streetStreetSegments[street_id][i];
        distance += find_street_segment_length(streetSegmentID);
    }
    return distance;
}

//Returns the travel time to drive a street segment in seconds (time = distance/speed_limit)

double find_street_segment_travel_time(unsigned street_segment_id) {
    double distance = streetSegmentLengths[street_segment_id];
    double speed_limit = streetSegments[street_segment_id].speedLimit;
    return (distance / speed_limit)*3.6;
}

//Returns the nearest point of interest to the given position

unsigned find_closest_point_of_interest(LatLon my_position) {
    point myPos(my_position.lat(), my_position.lon());
    unsigned closestPOI = 0;
    //set comparison distance to max
    double minDistance = numeric_limits<double>::max();

    std::vector<value> result_n;
    unsigned counterNum = 50; //use 50 by default
    unsigned numPOIS = getNumberOfPointsOfInterest(); //if given map has less than 50 points of interest, use total number
    if (numPOIS < counterNum)
        counterNum = numPOIS;
    //find the counterNum nearest points
    poiRTree.query(bgi::nearest(myPos, counterNum), std::back_inserter(result_n));

    //of the closest points, determine the exact closest one
    for (unsigned i = 0; i < counterNum; i++) {
        LatLon aPoint = getPointOfInterestPosition(result_n[i].second);
        double currDistance = find_distance_between_two_points(my_position, aPoint);

        if (currDistance < minDistance) {
            closestPOI = result_n[i].second;
            minDistance = currDistance;

        }

    }


    return closestPOI;
}


//Returns the the nearest intersection to the given position

unsigned find_closest_intersection(LatLon my_position) {

    point myPos(my_position.lat(), my_position.lon());
    unsigned closestIntersection = 0;
    //set comparison distance to max
    double minDistance = numeric_limits<double>::max();

    std::vector<value> result_n;

    unsigned counterNum = 100; //use 100 by default
    unsigned numIntersections = getNumberOfIntersections(); //if given map has less than 100 intersections, use total number
    if (numIntersections < counterNum)
        counterNum = numIntersections;
    //find closest points
    intersectionRTree.query(bgi::nearest(myPos, counterNum), std::back_inserter(result_n));

    //of closest intersections, find the exact closest
    for (unsigned i = 0; i < counterNum; i++) {
        LatLon aPoint = getIntersectionPosition(result_n[i].second);
        double currDistance = find_distance_between_two_points(my_position, aPoint);

        if (currDistance < minDistance) {
            closestIntersection = result_n[i].second;
            minDistance = currDistance;

        }

    }


    return closestIntersection;
}


void close_map() {
    //Clean-up your map related data structures here

    //only close map if already open
    if (isOpen == true) {
        closeStreetDatabase();
    }

    //clear data structures
    streets.clear();
    intersections.clear();
    streetSegments.clear();
    streetStreetSegments.clear();
    intersectionStreetSegments.clear();
    streetIntersections.clear();
    streetSegmentLengths.clear();
    poiRTree.clear();
    intersectionRTree.clear();
}
