#pragma once
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
#include <ctime>
#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "LatLon.h"

using namespace std;

extern vector<double> streetSegmentLengths;
extern vector<StreetSegmentInfo> streetSegments;
clock_t startTime;

struct DeliveryInfo {
    //Specifies a delivery order.
    //
    //To satisfy the order the item-to-be-delivered must have been picked-up 
    //from the pickUp intersection before visiting the dropOff intersection.

    DeliveryInfo(unsigned pick_up, unsigned drop_off)
    : pickUp(pick_up), dropOff(drop_off) {
    }

    DeliveryInfo() {
    };


    //The intersection id where the item-to-be-delivered is picked-up.
    unsigned pickUp;

    //The intersection id where the item-to-be-delivered is dropped-off.
    unsigned dropOff;

    bool operator==(const DeliveryInfo& d) const {
        return (this->pickUp == d.pickUp && this->dropOff == d.dropOff);
    }
};

double compute_nodes_travel_time(const std::vector<unsigned> intersections, const unsigned turn_penalty) {
    double travelTime = 0;
    vector<unsigned> tempPath;
    for (unsigned i = 0; i < intersections.size() - 1; i++) {
        tempPath = find_path_between_intersections(intersections[i], intersections[i + 1], turn_penalty);
        travelTime += compute_path_travel_time(tempPath, turn_penalty);
    }
    return travelTime;
}

std::vector<unsigned> two_opt_swap(const std::vector<unsigned> path, int i, int k) {
    vector<unsigned> newPath;

    //1. take route[1] to route[i-1] and add them in order to new_route
    vector<unsigned> pathPart1(path.begin(), path.begin() + i - 1);
    newPath.insert(newPath.end(), pathPart1.begin(), pathPart1.end());

    //2. take route[i] to route[k] and add them in reverse order to new_route
    vector<unsigned> pathPart2(path.begin() + i - 1, path.begin() + k);
    reverse(pathPart2.begin(), pathPart2.end());
    newPath.insert(newPath.end(), pathPart2.begin(), pathPart2.end());

    //3. take route[k+1] to end and add them in order to new_route
    vector<unsigned> pathPart3(path.begin() + k, path.end());
    newPath.insert(newPath.end(), pathPart3.begin(), pathPart3.end());

    return newPath;
}

bool false_start(const std::vector<unsigned>& path,
        const std::vector<unsigned>& depots,
        unsigned& intersect_start) {
    StreetSegmentInfo seg1 = getStreetSegmentInfo(path[0]);

    auto to = std::find(depots.begin(), depots.end(), seg1.to);
    auto from = std::find(depots.begin(), depots.end(), seg1.from);

    if (to != depots.end() && from == depots.end()) {
        intersect_start = *to;
    } else if (to == depots.end() && from != depots.end()) {
        intersect_start = *from;
    } else if (to != depots.end() && from != depots.end()) {
        if (path.size() == 1) {
            intersect_start = *from;
        } else {
            StreetSegmentInfo seg2 = getStreetSegmentInfo(path[1]);
            if (seg1.to == seg2.to || seg1.to == seg2.from) {
                intersect_start = seg1.from;
            } else {
                assert(seg1.from == seg2.to || seg1.from == seg2.from);
                intersect_start = seg1.to;
            }
        }

    } else {
        return false;
    }
    return true;
}

bool is_depot(const std::vector<unsigned>& depots,
        const unsigned intersect_id) {

    auto end = std::find(depots.begin(), depots.end(), intersect_id);

    return (end != depots.end());
}

void pick_up(const std::vector<DeliveryInfo>&,
        const std::multimap<unsigned, int>& to_pick_up,
        const unsigned curr_intersect,
        std::vector<bool>& picked_up) {

    auto pair = to_pick_up.equal_range(curr_intersect);

    for (auto i = pair.first; i != pair.second; ++i) {
        int id = i->second;

        picked_up[id] = true;
    }
}

void drop_off(const std::vector<DeliveryInfo>&,
        const std::multimap<unsigned, int>& to_drop_off,
        const std::vector<bool>& picked_up,
        const unsigned curr_intersect,
        std::vector<bool>& dropped_off) {
    auto pair = to_drop_off.equal_range(curr_intersect);

    //Mark each delivery dropped-off
    for (auto i = pair.first; i != pair.second; ++i) {
        int id = i->second;

        if (picked_up[id]) {
            dropped_off[id] = true;

        }
    }
}

bool delivered_all_packages(const std::vector<DeliveryInfo>& deliveries,
        const std::vector<bool>& picked_up,
        const std::vector<bool>& dropped_off) {

    int undelivered = 0;
    for (int i = 0; i < deliveries.size(); ++i) {
        if (!dropped_off[i]) {
            ++undelivered;
        } else {
            assert(picked_up[i]);
        }
    }

    if (undelivered > 0) {
        return false;
    }

    return true;
}

bool courier_path_is_legal(const std::vector<DeliveryInfo>& deliveries,
        const std::vector<unsigned>& depots,
        const std::vector<unsigned>& path) {


    if (path.empty()) {
        return false;

    } else {
        assert(!path.empty());

        unsigned curr_intersect;
        if (!false_start(path, depots, curr_intersect)) {
            return false;
        }

        assert(is_depot(depots, curr_intersect));

        std::vector<bool> picked_up(deliveries.size(), false);
        std::vector<bool> dropped_off(deliveries.size(), false);

        std::multimap<unsigned, int> to_pick_up; //Intersection_ID -> deliveries index
        std::multimap<unsigned, int> to_drop_off; //Intersection_ID -> deliveries index

        for (int id = 0; id < deliveries.size(); ++id) {
            unsigned pick_up_intersection = deliveries[id].pickUp;
            unsigned drop_off_intersection = deliveries[id].dropOff;

            to_pick_up.insert(std::make_pair(pick_up_intersection, id));
            to_drop_off.insert(std::make_pair(drop_off_intersection, id));
        }

        for (int id = 0; id < path.size(); ++id) {

            unsigned next_intersection;

            pick_up(deliveries, to_pick_up, curr_intersect, picked_up);

            drop_off(deliveries, to_drop_off, picked_up, curr_intersect, dropped_off);

            curr_intersect = next_intersection;
        }

        if (!is_depot(depots, curr_intersect)) {
            return false;
        }

        if (!delivered_all_packages(deliveries, picked_up, dropped_off)) {
            return false;
        }

    }

    //Everything validated
    return true;

}

// This routine takes in a vector of N deliveries (pickUp, dropOff
// intersection pairs), another vector of M intersections that
// are legal start and end points for the path and a turn penalty
// (see m3.h for details on turn penalties). 
//
// The first vector 'deliveries' gives the delivery information: a set of 
// pickUp/dropOff pairs of intersection ids which specify the 
// deliveries to be made. A delivery can only be dropped-off after 
// the associated item has been picked-up. 
// 
// The second vector 'depots' gives the intersection
// ids of courier company depots containing trucks; you start at any
// one of these depots and end at any one of the depots.
//
// This routine returns a vector of street segment ids that form a
// path, where the first street segment id is connected to a depot
// intersection, and the last street segment id also connects to a
// depot intersection.  The path must traverse all the delivery
// intersections in an order that allows all deliveries to be made --
// i.e. a package won't be dropped off if you haven't picked it up
// yet.
//
// You can assume that N is always at least one, and M is always at
// least one (i.e. both input vectors are non-empty).
//
// It is legal for the same intersection to appear multiple times in
// the pickUp or dropOff list (e.g. you might have two deliveries with
// a pickUp intersection id of #50). The same intersection can also
// appear as both a pickUp location and a dropOff location.
//        
// If you have two pickUps to make at an intersection, 
// traversing the intersection once is sufficient
// to pick up both packages, and similarly one traversal of an 
// intersection is sufficient to drop off all the (already picked up) 
// packages that need to be dropped off at that intersection.
//
// Depots will never appear as pickUp or dropOff locations for deliveries.
//  
// If no path connecting all the delivery locations
// and a start and end depot exists, this routine should return an
// empty (size == 0) vector.

std::vector<unsigned> traveling_courier(const std::vector<DeliveryInfo>& deliveries,
        const std::vector<unsigned>& depots,
        const float turn_penalty) {

    //cout << depots[0] << endl;

    startTime = clock();

    vector<vector<unsigned>> allPaths;
    vector<vector<unsigned>> allNodePaths;

    for (unsigned i = 0; i < depots.size(); i++) {
        //cout << "depot: " << depots[i] << endl;

        vector <DeliveryInfo> pickups;
        vector <unsigned> dropoffs;

        //load pickup  vector
        for (unsigned i = 0; i < deliveries.size(); i++) {
            pickups.push_back(deliveries[i]);
        }

        unsigned startDepot = -1;
        unsigned endDepot;
        DeliveryInfo firstPickUp;
        unsigned tempDepot;
        double smallest = std::numeric_limits<double>::max();
        double tempDist;
        LatLon point1, point2;

        //find which depot has geographically closest pickup to start at

        tempDepot = depots[i];
        point1 = getIntersectionPosition(tempDepot);
        unsigned next;
        if (deliveries.size() < 5)
            for (unsigned q = 0; q < pickups.size(); q++) {
                next = pickups[q].pickUp;
                point2 = getIntersectionPosition(next);
                tempDist = find_distance_between_two_points(point1, point2);
                if (find_path_between_intersections(tempDepot, next, turn_penalty).size() != 0) {
                    if (tempDist <= smallest) {
                        startDepot = tempDepot;
                        firstPickUp = deliveries[q];
                        smallest = tempDist;
                    }
                }
            } else
            for (unsigned q = 0; q < pickups.size(); q++) {
                next = pickups[q].pickUp;
                point2 = getIntersectionPosition(next);
                tempDist = find_distance_between_two_points(point1, point2);
                //if (find_path_between_intersections(tempDepot, next, turn_penalty).size() != 0){
                if (tempDist <= smallest) {
                    startDepot = tempDepot;
                    firstPickUp = deliveries[q];
                    smallest = tempDist;
                }
                //}
            }

        if (startDepot == -1) {
            continue;
        }

        vector <unsigned> intersectionsVisited;

        intersectionsVisited.push_back(startDepot); //start depot is first location
        intersectionsVisited.push_back(firstPickUp.pickUp); //go to first pickup

        dropoffs.push_back(firstPickUp.dropOff); //Add dropoff of first package to list of dropoffs


        //remove first pickup from list of remaining pickups
        auto it = std::find(pickups.begin(), pickups.end(), firstPickUp);
        if (it != pickups.end()) {
            pickups.erase(it);
        }

        //cout << "b1" << endl;
        DeliveryInfo potentialPickup;
        unsigned potentialDropOff;

        double closestPickup;
        double closestDropOff;
        double placeHolderDist;
        bool isPickup;

        //loop while there are still pickups or dropoffs
        while (pickups.size() != 0 || dropoffs.size() != 0) {
            closestPickup = std::numeric_limits<double>::max();
            closestDropOff = std::numeric_limits<double>::max();
            point1 = getIntersectionPosition(intersectionsVisited.back());


            //find geographically closest pickup
            for (unsigned i = 0; i < pickups.size(); i++) {
                point2 = getIntersectionPosition(pickups[i].pickUp);
                placeHolderDist = find_distance_between_two_points(point1, point2);
                if (placeHolderDist <= closestPickup) {
                    potentialPickup = pickups[i];
                    closestPickup = placeHolderDist;
                }
            }

            //find geographically closest dropoff
            for (unsigned i = 0; i < dropoffs.size(); i++) {
                point2 = getIntersectionPosition(dropoffs[i]);
                placeHolderDist = find_distance_between_two_points(point1, point2);
                if (placeHolderDist <= closestDropOff) {
                    potentialDropOff = dropoffs[i];
                    closestDropOff = placeHolderDist;
                }
            }

            //compare to see if should go to pickup or dropoff
            // cout << "pickup Dist" << closestPickup<<endl;
            //cout << "dropoff Dist" << closestDropOff<<endl;
            //if pickup is closer
            if (closestPickup <= closestDropOff) {
                //cout << "pickup" << potentialPickup.pickUp << endl;
                intersectionsVisited.push_back(potentialPickup.pickUp); //go to closest pickup

                dropoffs.push_back(potentialPickup.dropOff); //Add dropoff of package to list of dropoffs


                //remove pickup from list of remaining pickups
                auto it = std::find(pickups.begin(), pickups.end(), potentialPickup);
                if (it != pickups.end()) {
                    pickups.erase(it);
                }

                //if location is also a dropoff
                //auto qt = std::find(dumpManz.begin(), dumpManz.end(), potentialPickup.pickUp);
                //    if (qt != dumpManz.end()) {
                //         dumpManz.erase(qt);
                //    }


            }//if dropoff is closer
            else {
                //cout << "dropOff" << potentialDropOff << endl;
                intersectionsVisited.push_back(potentialDropOff); //go to closest dropoff
                auto iter = std::find(dropoffs.begin(), dropoffs.end(), potentialDropOff);
                if (iter != dropoffs.end()) {
                    dropoffs.erase(iter);
                }
            }
        }

        //find closest depot to end journey
        smallest = std::numeric_limits<double>::max();
        unsigned intersectEnd = intersectionsVisited.back();
        point1 = getIntersectionPosition(intersectionsVisited.back());

        if (depots.size() < 5)
            for (unsigned i = 0; i < depots.size(); i++) {
                tempDepot = depots[i];
                point2 = getIntersectionPosition(tempDepot);
                tempDist = find_distance_between_two_points(point1, point2);
                if (find_path_between_intersections(intersectEnd, tempDepot, turn_penalty).size() != 0) {
                    if (tempDist < smallest) {
                        endDepot = tempDepot;
                        smallest = tempDist;
                    }
                }
            } else
            for (unsigned i = 0; i < depots.size(); i++) {
                tempDepot = depots[i];
                point2 = getIntersectionPosition(tempDepot);
                tempDist = find_distance_between_two_points(point1, point2);
                //if (find_path_between_intersections(intersectEnd, tempDepot, turn_penalty).size() != 0) {
                    if (tempDist < smallest) {
                        endDepot = tempDepot;
                        smallest = tempDist;
                    }
                //}
            }

        // cout << "end depot" << endDepot << endl;
        intersectionsVisited.push_back(endDepot);

        //  for (unsigned q = 0; q < intersectionsVisited.size(); q++) {
        //   cout << intersectionsVisited[q] << endl;
        // }

        vector <unsigned> path;
        vector <unsigned> tempPath;
        //reverse(intersectionsVisited.begin(), intersectionsVisited.end());
        /*while (intersectionsVisited.size() >=0) {
           // cout << intersectionsVisited.back() << endl;
            tempPath = find_path_between_intersections(intersectionsVisited.back(), intersectionsVisited[intersectionsVisited.size() - 2], turn_penalty);

            //return empty if fails
            if (tempPath.size() == 0)
                return tempPath;

            path.insert(path.end(), tempPath.begin(), tempPath.end());
            intersectionsVisited.pop_back();
        }*/

        for (unsigned w = 0; w < intersectionsVisited.size() - 1; w++) {
            //cout << "intersect 1: " << intersectionsVisited[w] << endl;
            //cout << "intersect 2: " << intersectionsVisited[w + 1] << endl;
            tempPath = find_path_between_intersections(intersectionsVisited[w], intersectionsVisited[w + 1], turn_penalty);
            //return empty if fails
            // if (tempPath.size() == 0)
            //      return tempPath;
            if (tempPath.size() != 0) {
                //std::cout << "t 1 " << getStreetSegmentInfo(tempPath.front()).to << std::endl;
                //std::cout << "f 1 " << getStreetSegmentInfo(tempPath.front()).from << std::endl;
                //std::cout << "t 2 " << getStreetSegmentInfo(tempPath.back()).to << std::endl;
                //std::cout << "f 2 " << getStreetSegmentInfo(tempPath.back()).from << std::endl;

                path.insert(path.end(), tempPath.begin(), tempPath.end());
            }
        }
        allPaths.push_back(path);
        allNodePaths.push_back(intersectionsVisited);
    }

    vector<unsigned> bestPath;
    unsigned bestOpt;
    double smallestFull = std::numeric_limits<double>::max();
    double temporary;

    for (int z = 0; z < allPaths.size(); z++) {
        //cout << "to: " << std::cout << getStreetSegmentInfo(allPaths[z].front()).to << std::endl;
        //cout << "from: " << std::cout << getStreetSegmentInfo(allPaths[z].front()).from << std::endl;
        temporary = compute_path_travel_time(allPaths[z], turn_penalty);
        //cout << "tt: " << temporary << endl;
        if (temporary <= smallestFull) {
            //cout << "i: " << z << endl;
            bestPath = allPaths[z];
            smallestFull = temporary;
            bestOpt = z;
        }
    }
/*
    vector<unsigned> finalPath;
    vector<unsigned> tempPath;
    vector<unsigned> bestNodesPath;
    //cout << "prev: " << compute_nodes_travel_time(allNodePaths[bestOpt],turn_penalty) << endl;
    //bestNodesPath = two_opt_swap_function(allNodePaths[bestOpt], turn_penalty);

    vector<unsigned> newNodes = allNodePaths[bestOpt];
    vector<unsigned> curNodes = allNodePaths[bestOpt];

    //cout << "end " << endl;

    if (deliveries.size() < 10)
        return bestPath;

    //2 opt
    int travelDifference = 1;
    double minTravelTime;
    double curTravelTime;

    //pop front and back
    //cout << newNodes.size() << endl;
    unsigned front = newNodes.front();
    newNodes.erase(newNodes.begin());
    unsigned back = newNodes.back();
    newNodes.pop_back();
    //cout << newNodes[0] << endl;
    //cout << newNodes.size() << endl;



    clock_t endTime;
    clock_t clockTicksTaken;
    double timeInSeconds;

    //repeat until no improvement
start:
    while (travelDifference > 0) {
        minTravelTime = compute_nodes_travel_time(newNodes, turn_penalty);
        for (unsigned i = 1; i < newNodes.size() - 1; i++) {
            for (unsigned k = i + 1; k < newNodes.size(); k++) {
                endTime = clock();
                clockTicksTaken = endTime - startTime;
                timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
                if (timeInSeconds > 20) {
                    goto end;
                }
                curNodes = two_opt_swap(newNodes, i, k);
                curTravelTime = compute_nodes_travel_time(curNodes, turn_penalty);
                if (curTravelTime < minTravelTime) {
                    //cout << curDist << endl;
                    travelDifference = minTravelTime - curTravelTime;
                    minTravelTime = curTravelTime;
                    newNodes = curNodes;
                    goto start;
                }
            }
        }
    }

end:
    //store front and back
    newNodes.insert(newNodes.begin(), front);
    newNodes.push_back(back);

    bestNodesPath = newNodes;

    //cout << "2opt: " << compute_nodes_travel_time(bestNodesPath,turn_penalty) << endl;
    for (int i = 0; i < bestNodesPath.size() - 1; i++) {
        //cout << bestNodesPath[i] << endl;
        tempPath = find_path_between_intersections(bestNodesPath[i], bestNodesPath[i + 1], turn_penalty);

        //return empty if fails
        // if (tempPath.size() == 0)
        //      return tempPath;

        finalPath.insert(finalPath.end(), tempPath.begin(), tempPath.end());
    }*/

    //if (courier_path_is_legal(deliveries, depots, finalPath) == true)
        //return finalPath;
    //else
        return bestPath;
}


