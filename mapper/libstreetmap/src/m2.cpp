#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <vector>
#include <string>
#include <math.h> 
#include <unordered_map>
#include <queue>
#include <sstream>
#include <ctime>
#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "graphics.h"
#include "LatLon.h"
#include <stdlib.h>

using namespace std;


vector<unsigned> shortestPath;
//defining new structures

struct intersection_data {
    LatLon position;
    std::string name;
};

struct feature_data {
    vector<LatLon> position;
    FeatureType type;
    std::string name;
    double size;

    feature_data(vector<LatLon> position_, FeatureType type_, std::string name_, double size_) {
        position = position_;
        type = type_;
        name = name_;
        size = size_;
    }

    bool operator>(const feature_data& f) const {
        return size > f.size;
    }
};

struct SortbySize {

    bool operator()(const feature_data& lhs, const feature_data& rhs) const {
        return lhs.size > rhs.size;
    }
};

struct POI_data {
    LatLon position;
    std::string type;
    std::string name;
};


//global variables
extern std::vector<std::vector<StreetSegmentIndex>> streetStreetSegments;
extern unordered_map<string, vector<IntersectionIndex>> intersections;
extern vector<double> streetSegmentLengths;

std::vector<intersection_data> intersection; //data structure for intersections
std::vector<feature_data> allFeatures; //vector for features
std::vector<POI_data> allPOI; //data structure for points of interest
std::vector<std::vector<LatLon>> segCurvePoints; //data structure for curve points
std::unordered_map<OSMID, StreetSegmentIndex> allStreetSegments; //street segment index mapped by OSMID
std::unordered_map<std::string, std::vector<OSMID>> roadOSMID; //OSMID mapped by road type
double average_latitude;
double dlat;
double dlon;
bool has_clicked = false;
bool find_distance = false;
bool helpClicked = false;
LatLon clicked_on;
std::vector<IntersectionIndex> highlight_ids;
int counter;
unsigned id;
unsigned id2;
unsigned idPOI;
int zoom_level;
string line;
bool show_path = false;
vector<unsigned> POI_to_search;
vector <unsigned> intersection_to_search;
extern unordered_multimap<string, unsigned> poi_by_name;
bool show_legend = false;

//coordinate system conversions
//

double longitude_to_cartesian(double longitude, double averageLatitude) {
    double latAvgInRad = averageLatitude * DEG_TO_RAD;
    double x = longitude * cos(latAvgInRad);
    return x;
}

double latitude_to_cartesian(double latitude) {
    double y = latitude;
    return y;
}

double cartesian_to_longitude(double x, double averageLatitude) {
    double latAvgInRad = averageLatitude * DEG_TO_RAD;
    double lon = x / cos(latAvgInRad);
    return lon;
}

double cartesian_to_latitude(double y) {
    double lat = y;
    return lat;
}

void area_of_screen() {
    t_bound_box worldCoords = get_visible_world();
    t_point bottomLeft = worldCoords.bottom_left();
    t_point topRight = worldCoords.top_right();
    double worldArea = (topRight.x - bottomLeft.x)*(topRight.y - bottomLeft.y);
    if (worldArea > 0.05) {
        zoom_level = 1;
    } else {
        zoom_level = 2;
    }
}

double angle_from_lat_lon(LatLon latlon1, LatLon latlon2) {
    double lon1 = latlon1.lon();
    double lon2 = latlon2.lon();
    double lat1 = latlon1.lat();
    double lat2 = latlon2.lat();

    double dLon = (lon2 - lon1);

    double y = sin(dLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);

    double angle = atan2(y, x) * 180 / PI;

    return angle;
}

void print_path(vector<unsigned> path, unsigned start_intersect_id) {
    StreetIndex prevStreetID = -1;
    StreetSegmentIndex prevSegID = -1;
    IntersectionIndex prevIntersectID = start_intersect_id;
    std::string streetName;
    StreetSegmentInfo seg_info;
    StreetSegmentIndex streetSegID;
    IntersectionIndex intersectID;
    LatLon p1; // next intersection
    LatLon p2; // current intersection
    LatLon p3; // last intersection
    cout << "Directions: " << endl;
    for (unsigned i = 0; i < path.size(); i++) {
        seg_info = getStreetSegmentInfo(path[i]);
        streetSegID = path[i];
        streetName = getStreetName(seg_info.streetID);

        if (seg_info.to == prevIntersectID) {
            intersectID = seg_info.from;
        } else {
            intersectID = seg_info.to;
        }

        if (streetName == "<unknown>") {
            streetName = "unknown";
        }
        if (prevStreetID == -1) {
            std::cout << "Get on " << streetName << ". Travel for " << streetSegmentLengths[streetSegID] << " meters." << std::endl;
        } else if (prevStreetID != seg_info.streetID) {
            //getting 3 intersection points to calculate angles
            p1 = getIntersectionPosition(prevIntersectID);
            p2 = getIntersectionPosition(intersectID);
            if (prevIntersectID == getStreetSegmentInfo(prevSegID).to) {
                p3 = getIntersectionPosition(getStreetSegmentInfo(prevSegID).from);
            } else {
                p3 = getIntersectionPosition(getStreetSegmentInfo(prevSegID).to);
            }

            double angle1 = angle_from_lat_lon(p2, p1);
            double angle2 = angle_from_lat_lon(p2, p3);

            if ((angle1 - angle2) < 3) {
                std::cout << "Turn Left on " << streetName << ". Travel for " << streetSegmentLengths[streetSegID] << " meters." << endl;
            } else if ((angle1 - angle2) > 3) {
                std::cout << "Turn Right on " << streetName << ". Travel for " << streetSegmentLengths[streetSegID] << " meters." << endl;
            } else {
                std::cout << "Continue on " << streetName << ". Travel for " << streetSegmentLengths[streetSegID] << " meters." << endl;
            }
        }
        prevStreetID = seg_info.streetID;
        prevIntersectID = intersectID;
        prevSegID = path[i];
    }
    cout << "Arrived at destination" << endl;
}

void draw_street_segment(unsigned ID) {
    auto fromLocation = getStreetSegmentInfo(ID).from;
    auto toLocation = getStreetSegmentInfo(ID).to;

    //no curve points
    unsigned curvePointCount = getStreetSegmentInfo(ID).curvePointCount;
    if (curvePointCount == 0) {
        auto const x1 = longitude_to_cartesian(intersection[fromLocation].position.lon(), average_latitude);
        auto const y1 = latitude_to_cartesian(intersection[fromLocation].position.lat());
        auto const x2 = longitude_to_cartesian(intersection[toLocation].position.lon(), average_latitude);
        auto const y2 = latitude_to_cartesian(intersection[toLocation].position.lat());
        drawline(x1, y1, x2, y2);
    }//if curve points exist go through all curve points and draw
    else {
        LatLon point1 = intersection[fromLocation].position;
        LatLon point2;
        for (unsigned j = 0; j < segCurvePoints[ID].size(); j++) {
            point2 = segCurvePoints[ID][j];

            auto const x1 = longitude_to_cartesian(point1.lon(), average_latitude);
            auto const y1 = latitude_to_cartesian(point1.lat());
            auto const x2 = longitude_to_cartesian(point2.lon(), average_latitude);
            auto const y2 = latitude_to_cartesian(point2.lat());
            drawline(x1, y1, x2, y2);
            point1 = point2;
        }
        point2 = intersection[toLocation].position;
        auto const x1 = longitude_to_cartesian(point1.lon(), average_latitude);
        auto const y1 = latitude_to_cartesian(point1.lat());
        auto const x2 = longitude_to_cartesian(point2.lon(), average_latitude);
        auto const y2 = latitude_to_cartesian(point2.lat());
        drawline(x1, y1, x2, y2);
    }
}

void draw_screen() {
    clearscreen();

    area_of_screen();


    //drawing intersections
    for (unsigned i = 0; i < intersection.size(); ++i) {
        auto const x = longitude_to_cartesian(intersection[i].position.lon(), average_latitude);
        auto const y = latitude_to_cartesian(intersection[i].position.lat());
        float const width = 0.000050;
        float const height = 0.000050;
        setcolor(RED);
        fillrect(x - width / 2, y - height / 2, x + width / 2, y + height / 2);
    }

    //drawing features
    for (unsigned i = 0; i < allFeatures.size(); i++) {
        unsigned numPoint = allFeatures[i].position.size();
        //closed feature
        if (allFeatures[i].position[0].lat() == allFeatures[i].position[numPoint - 1].lat() &&
                allFeatures[i].position[0].lon() == allFeatures[i].position[numPoint - 1].lon()) {
            //creating t_point array for fillPoly
            t_point vertices[numPoint];
            for (unsigned j = 0; j < numPoint; j++) {
                auto const x = longitude_to_cartesian
                        (allFeatures[i].position[j].lon(), average_latitude);
                auto const y = latitude_to_cartesian
                        (allFeatures[i].position[j].lat());
                vertices[j].x = x;
                vertices[j].y = y;
            }
            if (allFeatures[i].type == 4 && zoom_level == 2) // river
                setcolor(16);
            else if (allFeatures[i].type == 1) // park
                setcolor(GREEN);
            else if (allFeatures[i].type == 2) // beach
                setcolor(19); //khaki
            else if (allFeatures[i].type == 3) // lake
                setcolor(16); //light blue
            else if (allFeatures[i].type == 5) // river
                setcolor(3);
            else if (allFeatures[i].type == 7 && zoom_level == 2){ // building
                setcolor(2); //grey 75
                continue;
            }
            else if (allFeatures[i].type == 8) // greenspace
                setcolor(13); //dark green
            else if (allFeatures[i].type == 9) // golfcourse
                setcolor(GREEN);
            fillpoly(vertices, numPoint);
        }//open feature
        else {
            setlinewidth(1);
            setcolor(16);
            for (unsigned j = 1; j < numPoint; j++) {
                auto const x1 = longitude_to_cartesian
                        (allFeatures[i].position[j - 1].lon(), average_latitude);
                auto const y1 = latitude_to_cartesian
                        (allFeatures[i].position[j - 1].lat());
                auto const x2 = longitude_to_cartesian
                        (allFeatures[i].position[j].lon(), average_latitude);
                auto const y2 = latitude_to_cartesian
                        (allFeatures[i].position[j].lat());
                drawline(x1, y1, x2, y2);
            }
        }
    }

    //drawing all streets
    for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) {
        if (getStreetSegmentInfo(i).speedLimit >= 100) {
            setcolor(ORANGE);
            setlinewidth(5);
            draw_street_segment(i);
        }
        if (zoom_level == 2) {
            setcolor(WHITE);
            setlinewidth(1);
            draw_street_segment(i);
        }
    }


    //drawing major streets by type using OSMID
    if (zoom_level == 2) {
        vector<OSMID> primary = roadOSMID["primary"];
        for (unsigned i = 0; i < primary.size(); i++) {
            unsigned ID = allStreetSegments[primary[i]];
            setcolor(WHITE);
            setlinewidth(4);
            draw_street_segment(ID);
        }
        vector<OSMID> primary_link = roadOSMID["primary_link"];
        for (unsigned i = 0; i < primary_link.size(); i++) {
            unsigned ID = allStreetSegments[primary_link[i]];
            setcolor(WHITE);
            setlinewidth(4);
            draw_street_segment(ID);
        }

        vector<OSMID> secondary = roadOSMID["secondary"];
        for (unsigned i = 0; i < secondary.size(); i++) {
            unsigned ID = allStreetSegments[secondary[i]];
            setcolor(WHITE);
            setlinewidth(3);
            draw_street_segment(ID);
        }
        vector<OSMID> secondary_link = roadOSMID["secondary_link"];
        for (unsigned i = 0; i < secondary_link.size(); i++) {
            unsigned ID = allStreetSegments[secondary_link[i]];
            setcolor(WHITE);
            setlinewidth(3);
            draw_street_segment(ID);
        }

        vector<OSMID> tertiary = roadOSMID["tertiary"];
        for (unsigned i = 0; i < tertiary.size(); i++) {
            unsigned ID = allStreetSegments[tertiary[i]];
            setcolor(WHITE);
            setlinewidth(3);
            draw_street_segment(ID);
        }
        vector<OSMID> tertiary_link = roadOSMID["tertiary_link"];
        for (unsigned i = 0; i < tertiary_link.size(); i++) {
            unsigned ID = allStreetSegments[tertiary_link[i]];
            setcolor(WHITE);
            setlinewidth(3);
            draw_street_segment(ID);
        }
    }
    vector<OSMID> motorway_link = roadOSMID["motorway_link"];
    for (unsigned i = 0; i < motorway_link.size(); i++) {
        unsigned ID = allStreetSegments[motorway_link[i]];
        setcolor(ORANGE);
        setlinewidth(4);
        draw_street_segment(ID);
    }
    vector<OSMID> trunk_link = roadOSMID["trunk_link"];
    for (unsigned i = 0; i < trunk_link.size(); i++) {
        unsigned ID = allStreetSegments[trunk_link[i]];
        setcolor(ORANGE);
        setlinewidth(4);
        draw_street_segment(ID);
    }
    vector<OSMID> motorway = roadOSMID["motorway"];
    for (unsigned i = 0; i < motorway.size(); i++) {
        unsigned ID = allStreetSegments[motorway[i]];
        setcolor(ORANGE);
        setlinewidth(5);
        draw_street_segment(ID);
    }
    vector<OSMID> trunk = roadOSMID["trunk"];
    for (unsigned i = 0; i < trunk.size(); i++) {
        unsigned ID = allStreetSegments[trunk[i]];
        setcolor(ORANGE);
        setlinewidth(4);
        draw_street_segment(ID);
    }

    if (show_path == true) {
        for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) {
            if (find(shortestPath.begin(), shortestPath.end(), i) != shortestPath.end()) {
                setcolor(RED);
                setlinewidth(4);
                draw_street_segment(i);
            }
        }
    }

    //

    double some_var_x;
    double some_var_y;

    //Write street names
    for (unsigned i = 0; i < getNumberOfStreets(); i++) {
        for (unsigned j = 0; j < streetStreetSegments[i].size(); j++) {
            auto segmentInfo = getStreetSegmentInfo(streetStreetSegments[i][j]);
            auto fromLatLon = intersection[segmentInfo.from].position;
            auto toLatLon = intersection[segmentInfo.to].position;
            auto const x1 = longitude_to_cartesian(fromLatLon.lon(), average_latitude);
            auto const y1 = latitude_to_cartesian(fromLatLon.lat());
            auto const x2 = longitude_to_cartesian(toLatLon.lon(), average_latitude);
            auto const y2 = latitude_to_cartesian(toLatLon.lat());

            unsigned curvePointCount = getStreetSegmentInfo(streetStreetSegments[i][j]).curvePointCount;
            //if no curve points draw in middle of segment
            if (curvePointCount == 0) {
                some_var_x = (x1 + x2) / 2;
                some_var_y = (y1 + y2) / 2;
            }//if curve points, draw at the halfway curve point of segment
            else {
                auto const theCP = getStreetSegmentCurvePoint(streetStreetSegments[i][j], curvePointCount / 2);
                some_var_x = longitude_to_cartesian(theCP.lon(), average_latitude);
                some_var_y = latitude_to_cartesian(theCP.lat());
            }

            //only draw name every 4th segment of the street
            if (j % 4 == 0) {

                if (getStreetName(i) != "<unknown>") {
                    //rotate street name to fit street
                    double dx = (x2 - x1);
                    double dy = (y2 - y1);
                    int rotateDeg = atan(dy / dx) *180 / PI;
                    settextrotation(rotateDeg);
                    setcolor(23);
                    {
                        setcolor(BLACK);
                        drawtext(some_var_x, some_var_y, getStreetName(i), 0.001, 0.001);
                        settextrotation(0);
                    }
                }

                //Display one way Streets
                string text;

                text = ">";
                //rotate arrow to fit street and direction
                double dx = (x2 - x1);
                double dy = (y2 - y1);
                int rotateDeg = atan2(dy, dx) *180 / PI;
                settextrotation(rotateDeg);

                setcolor(PURPLE);
                if (segmentInfo.oneWay == true)
                    drawtext(some_var_x, some_var_y, text, 0.001, 0.001);
                settextrotation(0);

            }
        }
    }

    //drawing points of interest
    for (unsigned i = 0; i < allPOI.size(); i++) {

        auto const x = longitude_to_cartesian(allPOI[i].position.lon(), average_latitude);
        auto const y = latitude_to_cartesian(allPOI[i].position.lat());
        float const radius1 = 0.00005;
        float const radius2 = 0.000025;
        setcolor(PINK);
        fillarc(x, y, radius1, 0, 1000);
        setcolor(3);
        fillarc(x, y, radius2, 0, 1000);
        setcolor(BLUE);
        auto ytext = y + 0.00010;
        drawtext(x, ytext, allPOI[i].name, 0.0009, 0.0009);
    }

    //drawing highlighted intersections
    for (unsigned i = 0; i < highlight_ids.size(); ++i) {
        LatLon highlightPos = getIntersectionPosition(highlight_ids[i]);
        auto const x = longitude_to_cartesian(highlightPos.lon(), average_latitude);
        auto const y = latitude_to_cartesian(highlightPos.lat());
        float const width = 0.00050;
        float const height = 0.00050;
        setcolor(YELLOW);
        fillrect(x - width / 2, y - height / 2, x + width / 2, y + height / 2);
    }

    //debug test case
    double radius1;
    double radius2;

    //if click on map
    if (has_clicked == true) {
        set_coordinate_system(GL_SCREEN);
        setcolor(BLACK);
        fillrect(5, 630, 400, 930);
        setcolor(WHITE);
        drawtext(202, 700, "Closest Point of Interest: ");
        drawtext(202, 750, allPOI[idPOI].name);
        drawtext(202, 800, "Closest Intersection: ");
        drawtext(202, 850, intersection[id].name);
        string display = to_string(dlat) + ", " + to_string(dlon);
        drawtext(202, 650, display);
        set_coordinate_system(GL_WORLD);
        //    show_path = false;
        //highlighting
        LatLon highlightPos = intersection[id].position;
        auto x = longitude_to_cartesian(highlightPos.lon(), average_latitude);
        auto y = latitude_to_cartesian(highlightPos.lat());
        float const width = 0.00050;
        float const height = 0.00050;
        //highlight closest intersection
        setcolor(YELLOW);
        fillrect(x - width / 2, y - height / 2, x + width / 2, y + height / 2);
        //display it's name
        setcolor(BLACK);
        drawtext(x, y, intersection[id].name, 0.001, 0.001);

        x = longitude_to_cartesian(allPOI[idPOI].position.lon(), average_latitude);
        y = latitude_to_cartesian(allPOI[idPOI].position.lat());
        radius1 = 0.00025;
        setcolor(ORANGE);
        fillarc(x, y, radius1, 0, 1000);
        //Write its name
        setcolor(BLACK);
        drawtext(x, y, allPOI[idPOI].name, 0.0009, 0.0009);
    }

    //highlighting 2 intersections for finding distance
    if (find_distance == true) {
        for (unsigned i = 0; i < highlight_ids.size(); ++i) {
            LatLon highlightPos = getIntersectionPosition(highlight_ids[i]);
            auto const x = longitude_to_cartesian(highlightPos.lon(), average_latitude);
            auto const y = latitude_to_cartesian(highlightPos.lat());
            float const width = 0.00050;
            float const height = 0.00050;
            setcolor(YELLOW);
            fillrect(x - width / 2, y - height / 2, x + width / 2, y + height / 2);
        }
    }

    //draw shortest path
    if (show_path == true) {


    }

    //create legend
    if (show_legend == true) {
        set_coordinate_system(GL_SCREEN);
        setcolor(240, 240, 240, 255);
        fillrect(0, 0, 250, 250);
        setcolor(RED);
        drawtext(125, 10, "LEGEND");
        //
        setcolor(PURPLE);
        drawtext(20, 40, ">");
        setcolor(BLACK);
        drawtext(150, 40, " ONE WAY");
        //
        setcolor(16);
        drawline(15, 70, 30, 70);
        setcolor(BLACK);
        drawtext(150, 70, "RIVER");
        //
        setcolor(RED);
        fillrect(16, 96, 24, 104);
        setcolor(BLACK);
        drawtext(150, 100, "INTERSECTION");
        //
        setcolor(ORANGE);
        drawline(15, 130, 30, 130);
        setcolor(BLACK);
        drawtext(150, 130, "HIGHWAY");
        //
        setcolor(PINK);
        fillarc(20, 160, radius1, 0, 1000);
        setcolor(3);
        fillarc(20, 160, radius2, 0, 1000);
        setcolor(BLACK);
        drawtext(150, 160, "POINT OF INTEREST");
        //
        setcolor(GREEN);
        fillrect(16, 186, 24, 194);
        setcolor(BLACK);
        drawtext(150, 190, "PARK");
        //
        setcolor(13);
        fillrect(16, 216, 24, 224);
        setcolor(BLACK);
        drawtext(150, 220, "GREENSPACE");
    }

    set_coordinate_system(GL_WORLD);

    copy_off_screen_buffer_to_screen();

}

//act on clicking

void act_on_button_press(float x, float y, t_event_buttonPressed event) {

    auto const lon = cartesian_to_longitude(x, average_latitude);
    auto const lat = cartesian_to_latitude(y);
    dlon = lon;
    dlat = lat;
    LatLon position(lat, lon);

    if (find_distance == false) {
        //highlight closest intersection and POI to mouseclick
        id = find_closest_intersection(position);
        idPOI = find_closest_point_of_interest(position);
        clicked_on = position;
        has_clicked = !has_clicked;
        draw_screen();
    } else {
        //find distance between 2 intersections
        //using mouseclick to choose intersections
        if (counter == 0) {
            id = find_closest_intersection(position);
            highlight_ids.push_back(id);
            draw_screen();
            counter++;
        } else if (counter == 1) {
            id2 = find_closest_intersection(position);
            highlight_ids.push_back(id2);
            draw_screen();

            //counter++;
            //else {


            cout << "Enter Turn Penalty" << endl;
            std::getline(std::cin, line);
            stringstream linestream(line);
            double turn_penalty;
            linestream >> turn_penalty;
            while (turn_penalty < 0) {
                cout << "Invalid. Please enter a positive number or 0" << endl;
                std::getline(std::cin, line);
                stringstream linestream(line);
                double turn_penalty;
                linestream >> turn_penalty;
            }
            shortestPath = find_path_between_intersections(id, id2, turn_penalty);
            print_path(shortestPath, id);
            highlight_ids.clear();
            find_distance = false;
            has_clicked = false;
            counter = 0;
            draw_screen();
        }
    }
}

//new button to highlight closest intersection from 2 streets
//typed in terminal

void act_on_find_button_func(void (*drawscreen_ptr) (void)) {
    if (helpClicked == true) {
        cout << "Find: Enter two streets to see where they intersect" << endl;
    } else {
        std::string street1, street2;
        std::cout << "Enter street name 1: ";
        getline(std::cin, street1);
        std::cout << "Enter street name 2: ";
        getline(std::cin, street2);
        std::cout << "Intersection information: " << std::endl;
        highlight_ids = find_intersection_ids_from_street_names(street1, street2);
        if (highlight_ids.size() == 0)
            std::cout << "No information available" << std::endl;
        else {
            for (unsigned i = 0; i < highlight_ids.size(); i++) {
                unsigned ID = highlight_ids[i];
                std::cout << "Intersection ID: " << ID << std::endl;
                std::cout << "Streets on intersection " <<
                        intersection[ID].name << ":" << std::endl;
                std::vector<string> names;
                names = find_intersection_street_names(ID);
                for (unsigned j = 0; j < names.size(); j++) {
                    std::cout << names[j] << std::endl;
                }
            }
            std::cout << "Done" << std::endl;
        }
    }
}

//new button to find distance between 2 intersection, chosen by mouseclicks

void act_on_itoi_button_func(void (*drawscreen_ptr) (void)) {
    if (helpClicked == true) {
        cout << "Int -> Int: Enter the name of two intersections to find the fastest path from the first intersection to the second intersection" << endl;
    } else {

        string method;
        cout << "Type 'Enter' to enter the names of two intersections or 'Click' to select them by clicking" << endl;
        getline(std::cin, method);
        while (method != "Click" && method != "click" && method != "Enter" && method != "enter") {
            cout << "Invalid Command" << endl;
            cout << "Type 'Enter' to enter the names of two intersections or 'Click' to select them by clicking" << endl;
            getline(std::cin, method);
        }

        if (method == "enter" || method == "Enter") {

            bool correct = false;
            string intersectionStart;
            string intersectionEnd;

            std::string street1Start, street2Start, street1End, street2End;
            std::cout << "Enter start street name 1: " << endl;
            getline(std::cin, street1Start);
            std::cout << "Enter start street name 2: " << endl;
            getline(std::cin, street2Start);


            auto startInter = find_intersection_ids_from_street_names(street1Start, street2Start);
            while (startInter.size() == 0) {
                std::cout << "No information available" << std::endl;
                std::cout << "Enter start street name 1: " << endl;

                getline(std::cin, street1Start);
                std::cout << "Enter start street name 2: " << endl;

                getline(std::cin, street2Start);
                startInter = find_intersection_ids_from_street_names(street1Start, street2Start);
            }

            std::cout << "Enter end street name 1: " << endl;

            getline(std::cin, street1End);
            std::cout << "Enter end street name 2: " << endl;

            getline(std::cin, street2End);
            auto endInter = find_intersection_ids_from_street_names(street1End, street2End);
            while (endInter.size() == 0) {
                std::cout << "No information available" << std::endl;
                std::cout << "Enter end street name 1: " << endl;

                getline(std::cin, street1End);
                std::cout << "Enter end street name 2: " << endl;

                getline(std::cin, street2End);
                endInter = find_intersection_ids_from_street_names(street1End, street2End);
            }
            id = startInter[0];
            id2 = endInter[0];

            string turn_penalty_;
            cout << "Enter turn penalty" << endl;
            std::getline(std::cin, line);
            stringstream linestream(line);
            double turn_penalty;
            linestream >> turn_penalty;
            while (turn_penalty < 0) {
                cout << "Invalid. Please enter a positive number or 0" << endl;
                std::getline(std::cin, line);
                stringstream linestream(line);
                double turn_penalty;
                linestream >> turn_penalty;
            }
            clock_t startTime = clock();
            shortestPath = find_path_between_intersections(id, id2, turn_penalty);
            clock_t endTime = clock();
            clock_t clockTicksTaken = endTime - startTime;
            double timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
            cout << (double) CLOCKS_PER_SEC << endl;
            print_path(shortestPath, startInter[0]);

        } else {
            cout << "Click on 2 intersections" << endl;
            find_distance = true;
        }

        show_path = true;
        //do some calculations here     
    }
}

void act_on_itopoi_button_func(void (*drawscreen_ptr) (void)) {
    if (helpClicked == true) {
        cout << "Int -> POI: Enter the name of an intersection and the name of a point of interest to find the fastest path from the intersection to the point of interest" << endl;

    } else {


        bool correct = false;
        string intersectionStart;


        std::string street1Start, street2Start;
        std::cout << "Enter start street name 1: " << endl;
        getline(std::cin, street1Start);
        std::cout << "Enter start street name 2: " << endl;
        getline(std::cin, street2Start);


        auto startInter = find_intersection_ids_from_street_names(street1Start, street2Start);
        while (startInter.size() == 0) {
            std::cout << "No information available" << std::endl;
            std::cout << "Enter start street name 1: " << endl;

            getline(std::cin, street1Start);
            std::cout << "Enter start street name 2: " << endl;

            getline(std::cin, street2Start);
            startInter = find_intersection_ids_from_street_names(street1Start, street2Start);
        }

        id = startInter[0];

        cout << "Enter Point of Interest Name" << endl;
        string daPOI;
        getline(std::cin, daPOI);
        int q = poi_by_name.count(daPOI);
        while (q == 0) {
            cout << "Not Found, please enter a new point of interest" << endl;
            getline(std::cin, daPOI);
            q = poi_by_name.count(daPOI);
        }


        cout << "Enter turn_penalty" << endl;
        std::getline(std::cin, line);
        stringstream linestream(line);
        double turn_penalty;
        linestream >> turn_penalty;
        while (turn_penalty < 0) {
            cout << "Invalid. Please enter a positive number or 0" << endl;
            std::getline(std::cin, line);
            stringstream linestream(line);
            double turn_penalty;
            linestream >> turn_penalty;
        }
        shortestPath = find_path_to_point_of_interest(id, daPOI, turn_penalty);
        print_path(shortestPath, startInter[0]);
    }
    show_path = true;
}

void act_on_help_button_func(void (*drawscreen_ptr) (void)) {
    if (helpClicked == false) {
        helpClicked = true;
        cout << "" << endl;
        cout << "Interface information: " << endl;
        cout << "Click anywhere on the map to highlight and receive information about the nearest intersection and point of interest" << endl;
        cout << "" << endl;
        cout << "Click on one of the following buttons for more information: " << endl;
        cout << "Legend" << endl;
        cout << "Find" << endl;
        cout << "Int -> Int" << endl;
        cout << "Int -> POI" << endl;
        cout << "Clear Path" << endl;
        cout << "" << endl;
        cout << "Click the help button again to return to map" << endl;
        cout << "" << endl;
    } else {
        helpClicked = false;
        cout << "Ending help session" << endl;
        cout << "" << endl;
    }
}

void act_on_legend_button_func(void (*drawscreen_ptr) (void)) {
    if (helpClicked == false){
        show_legend = !show_legend;
        draw_screen();
    }
    else{
        cout << "Legend: Toggles the on-screen legend" << endl;
    }
    
}

void act_on_clear_button_func(void (*drawscreen_ptr) (void)) {
    if (helpClicked == true) {
        cout << "Clear Path: Erase the currently drawn path" << endl;

    } else {
        show_path = false;
        cout << "Path cleared" << endl;
        shortestPath.clear();
        draw_screen();
    }
}

double get_area_of_polygon(vector<LatLon> position) {
    double area = 0;
    unsigned j = position.size() - 1;
    double x1, x2, y1, y2;
    for (unsigned i = 0; i < position.size(); i++) {
        x1 = longitude_to_cartesian(position[i].lon(), average_latitude);
        x2 = longitude_to_cartesian(position[j].lon(), average_latitude);
        y1 = latitude_to_cartesian(position[i].lat());
        y2 = latitude_to_cartesian(position[j].lat());
        area += (x1 + x2) * (y2 - y1);
        j = i;
    }
    return area * 0.5;
}

void draw_map() {

    //loads features into a priority
    //sort by size of feature
    unsigned numFeatures = getNumberOfFeatures();
    for (unsigned i = 0; i < numFeatures; i++) {
        string name = getFeatureName(i);
        FeatureType type = getFeatureType(i);
        unsigned numPoint = getFeaturePointCount(i);
        vector<LatLon> position;
        for (unsigned j = 0; j < numPoint; j++) {
            position.push_back(getFeaturePoint(i, j));
        }
        double size = get_area_of_polygon(position);
        feature_data tempFeature(position, type, name, size);
        allFeatures.push_back(tempFeature);
    }
    sort(allFeatures.begin(), allFeatures.end(), SortbySize());


    //loads points of interest into data structure
    allPOI.resize(getNumberOfPointsOfInterest());
    for (unsigned i = 0; i < allPOI.size(); i++) {
        allPOI[i].name = getPointOfInterestName(i);
        allPOI[i].type = getPointOfInterestType(i);
        allPOI[i].position = getPointOfInterestPosition(i);
    }


    //loads intersections into data structure
    double maximum_latitude = getIntersectionPosition(0).lat();
    double minimum_latitude = maximum_latitude;
    double maximum_longitude = getIntersectionPosition(0).lon();
    double minimum_longitude = maximum_longitude;

    intersection.resize(getNumberOfIntersections());
    for (unsigned i = 0; i < intersection.size(); ++i) {
        intersection[i].position = getIntersectionPosition(i);
        intersection[i].name = getIntersectionName(i);

        maximum_latitude = std::max(maximum_latitude, intersection[i].position.lat());
        minimum_latitude = std::min(minimum_latitude, intersection[i].position.lat());
        maximum_longitude = std::max(maximum_longitude, intersection[i].position.lon());
        minimum_longitude = std::min(minimum_longitude, intersection[i].position.lon());
    }
    average_latitude = 0.5 * (maximum_latitude + minimum_latitude);



    //loads curve points into data structure
    for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) {
        std::vector<LatLon> curvePointLatLon;
        unsigned numPoint(getStreetSegmentInfo(i).curvePointCount);
        for (unsigned j = 0; j < numPoint; j++) {
            curvePointLatLon.push_back(getStreetSegmentCurvePoint(i, j));
        }
        segCurvePoints.push_back(curvePointLatLon);
    }



    //loads street IDs into hash table mapped by way OSM IDs
    for (unsigned i = 0; i < getNumberOfStreetSegments(); i++) {
        OSMID streetOSMID = getStreetSegmentInfo(i).wayOSMID;
        allStreetSegments.insert(pair<OSMID, StreetSegmentIndex> (streetOSMID, i));
    }



    //loads OSMIDs into a hash table mapped by road type
    for (unsigned i = 0; i < getNumberOfWays(); i++) {
        const OSMWay* wayTag = getWayByIndex(i);
        unsigned tagCount = getTagCount(wayTag);
        for (unsigned j = 0; j < tagCount; j++) {
            pair<std::string, std::string> tagPair = getTagPair(wayTag, j);
            if (tagPair.first == "highway") {
                int found = roadOSMID.count(tagPair.second);
                if (found) {
                    roadOSMID.at(tagPair.second).push_back(wayTag->id());
                } else {
                    std::vector<OSMID> temp;
                    temp.push_back(wayTag->id());
                    roadOSMID.insert(pair<string, std::vector < OSMID >> (tagPair.second, temp));
                }
            }
        }
    }


    //initialize graphics
    init_graphics("Map", 3);
    set_visible_world(longitude_to_cartesian(minimum_longitude, average_latitude),
            latitude_to_cartesian(minimum_latitude),
            longitude_to_cartesian(maximum_longitude, average_latitude),
            latitude_to_cartesian(maximum_latitude));




    set_drawing_buffer(OFF_SCREEN);
    //button for finding intersection
    create_button("Window", "Clear Path", act_on_clear_button_func);
    create_button("Window", "Int -> POI", act_on_itopoi_button_func);
    create_button("Window", "Int -> Int", act_on_itoi_button_func);
    create_button("Window", "Find", act_on_find_button_func);
    create_button("Window", "Legend", act_on_legend_button_func);
    create_button("Window", "Help", act_on_help_button_func);

    event_loop(act_on_button_press, nullptr, nullptr, draw_screen);

    //clear previous data structures
    intersection.clear();
    allPOI.clear();
    segCurvePoints.clear();
    allStreetSegments.clear();
    roadOSMID.clear();
    highlight_ids.clear();
    allFeatures.clear();
    close_graphics();
}

