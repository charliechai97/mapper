#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <vector>
#include <string>
#include <math.h> 
#include <unordered_map>
#include <queue>   
#include <limits>

#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "LatLon.h"

using namespace std;

extern vector<double> streetSegmentLengths;
extern vector<StreetSegmentInfo> streetSegments;

struct Node {
    unsigned id;
    vector<StreetSegmentIndex> edge; // connected edges
    vector<double> cost;
    int prevId;
    int prevEdge;
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
        travelTime = -1;
        visited = false;
    }

    bool operator>(const Node& n) const {
        return travelTime > n.travelTime;
    }
};

struct SortbyCost {

    bool operator()(const Node& lhs, const Node& rhs) const {
        return lhs.travelTime > rhs.travelTime;
    }
};

extern vector<Node> allNodes;

extern vector<unsigned> intersection_to_search;
extern vector<unsigned> POI_to_search;
extern unordered_multimap<string, unsigned> poi_by_name;
extern vector <unsigned> shortestPath;

// Returns the time required to travel along the path specified, in seconds. 
// The path is given as a vector of street segment ids, and this function 
// can assume the vector either forms a legal path or has size == 0.
// The travel time is the sum of the length/speed-limit of each street 
// segment, plus the given turn_penalty (in seconds) per turn implied by the path. 
// A turn occurs when two consecutive street segments have different street IDs.

double compute_path_travel_time(const std::vector<unsigned>& path,
        const double turn_penalty) {
    double travel_time = 0;
    if (path.size() != 0) {
        for (unsigned i = 0; i < path.size(); i++) {
            double distance = streetSegmentLengths[path[i]];
            double speed_limit = streetSegments[path[i]].speedLimit;
            travel_time += (distance / speed_limit)*3.6;
            if (i != 0 && streetSegments[path[i]].streetID != streetSegments[path[i - 1]].streetID) {
                travel_time += turn_penalty;
            }
        }
    }
    else{
        return std::numeric_limits<double>::max();
    }
    return travel_time;
}



// Returns a path (route) between the start intersection and the end 
// intersection, if one exists. This routine should return the shortest path
// between the given intersections when the time penalty to turn (change
// street IDs) is given by turn_penalty (in seconds).
// If no path exists, this routine returns an empty (size == 0) vector. 
// If more than one path exists, the path with the shortest travel time is 
// returned. The path is returned as a vector of street segment ids; traversing 
// these street segments, in the returned order, would take one from the start 
// to the end intersection.

std::vector<unsigned> traceBack(const unsigned intersect_id_start, const unsigned intersect_id_end) {
    vector<unsigned> path;

    Node currNode = allNodes[intersect_id_end];
    while (currNode.id != intersect_id_start) {
        path.push_back(currNode.prevEdge);
        currNode = allNodes[currNode.prevId];
    }

    reverse(path.begin(), path.end());
    return path;
}

std::vector<unsigned> find_path_between_intersections(const unsigned intersect_id_start,
        const unsigned intersect_id_end,
        const double turn_penalty) {

    //clear all nodes
    for (unsigned i = 0; i < getNumberOfIntersections(); i++) {
        allNodes[i].prevId = -1;
        allNodes[i].prevEdge = -1;
        allNodes[i].travelTime = -1;
        allNodes[i].visited = false;
    }

    vector<unsigned> path;

    //start search
    //initialize current travel time
    std::priority_queue<Node, vector<Node>, SortbyCost> openSet;
    openSet.push(allNodes[intersect_id_start]);

    while (openSet.size() != 0) {
        //store back and pop
        Node currNode = openSet.top();
        openSet.pop();
        allNodes[currNode.id].visited = true;

        //break and trace back current node id is equal to the end
        if (currNode.id == intersect_id_end) {
            path = traceBack(intersect_id_start, intersect_id_end);
            break;
        }

        //visit all neighbour edges at current node
        vector<unsigned> neighbourEdge = currNode.edge;
        for (unsigned i = 0; i < neighbourEdge.size(); ++i) {
            StreetSegmentInfo edgeInfo = streetSegments[neighbourEdge[i]];
            unsigned neighbourId;
            //get neighbouring id
            if (edgeInfo.to == currNode.id) {
                neighbourId = edgeInfo.from;
            } else {
                neighbourId = edgeInfo.to;
            }

            //do stuff if node has not been visited
            if (allNodes[neighbourId].visited == false) {
                //calculate tentative travel time
                double tTravelTime = currNode.cost[i] + currNode.travelTime;

                //add turn_penalty if different street ids
                if (currNode.id != intersect_id_start &&
                        streetSegments[neighbourEdge[i]].streetID != streetSegments[currNode.prevEdge].streetID) {
                    tTravelTime += turn_penalty;
                }

                //update travel time if shortest path
                if (tTravelTime < allNodes[neighbourId].travelTime ||
                        allNodes[neighbourId].travelTime == -1) {
                    allNodes[neighbourId].travelTime = tTravelTime;
                    allNodes[neighbourId].prevEdge = neighbourEdge[i];
                    allNodes[neighbourId].prevId = currNode.id;
                    //add to open set
                    openSet.push(allNodes[neighbourId]);
                }
            }
        }
    }
    return path;
}

// Returns the shortest travel time path (vector of street segments) from 
// the start intersection to a point of interest with the specified name.
// The path will begin at the specified intersection, and end on the 
// intersection that is closest (in Euclidean distance) to the point of 
// interest.
// If no such path exists, returns an empty (size == 0) vector.

std::vector<unsigned> find_path_to_point_of_interest(const unsigned intersect_id_start,
        const std::string point_of_interest_name,
        const double turn_penalty) {

    LatLon someting;
    unsigned closestIntersection;

    POI_to_search.clear();
    intersection_to_search.clear();
    auto its = poi_by_name.equal_range(point_of_interest_name);
    for (auto it = its.first; it != its.second; ++it) {
        POI_to_search.push_back(it->second);
    }
    for (unsigned w = 0; w < POI_to_search.size(); w++) {
        someting = getPointOfInterestPosition(POI_to_search[w]);
        closestIntersection = find_closest_intersection(someting);
        intersection_to_search.push_back(closestIntersection);
    }

        //clear all nodes
    for (unsigned i = 0; i < getNumberOfIntersections(); i++) {
        allNodes[i].prevId = -1;
        allNodes[i].prevEdge = -1;
        allNodes[i].travelTime = -1;
        allNodes[i].visited = false;
    }

    vector<unsigned> path;

    //start search
    //initialize current travel time
    std::priority_queue<Node, vector<Node>, SortbyCost> openSet;
    openSet.push(allNodes[intersect_id_start]);

    while (openSet.size() != 0) {
        //store back and pop
        Node currNode = openSet.top();
        openSet.pop();
        allNodes[currNode.id].visited = true;

        //break and trace back current node id is equal to any of the end ids
        auto found = find(intersection_to_search.begin(), intersection_to_search.end(), currNode.id);
        if (found != intersection_to_search.end()) {
            path = traceBack(intersect_id_start, allNodes[*found].id);
            break;
        }

        //visit all neighbour edges at current node
        vector<unsigned> neighbourEdge = currNode.edge;
        for (unsigned i = 0; i < neighbourEdge.size(); ++i) {
            StreetSegmentInfo edgeInfo = streetSegments[neighbourEdge[i]];
            unsigned neighbourId;
            //get neighbouring id
            if (edgeInfo.to == currNode.id) {
                neighbourId = edgeInfo.from;
            } else {
                neighbourId = edgeInfo.to;
            }

            //do stuff if node has not been visited
            if (allNodes[neighbourId].visited == false) {
                //calculate tentative travel time
                double tTravelTime = currNode.cost[i] + currNode.travelTime;

                //add turn_penalty if different street ids
                if (currNode.id != intersect_id_start &&
                        streetSegments[neighbourEdge[i]].streetID != streetSegments[currNode.prevEdge].streetID) {
                    tTravelTime += turn_penalty;
                }

                //update travel time if shortest path
                if (tTravelTime < allNodes[neighbourId].travelTime ||
                        allNodes[neighbourId].travelTime == -1) {
                    allNodes[neighbourId].travelTime = tTravelTime;
                    allNodes[neighbourId].prevEdge = neighbourEdge[i];
                    allNodes[neighbourId].prevId = currNode.id;
                    //add to open set
                    openSet.push(allNodes[neighbourId]);
                }
            }
        }
    }

    return path;
}
