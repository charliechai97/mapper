#include <iostream>
#include <string>
#include <vector>
#include "graphics.h"
#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "m4.h"
#include "OSMDatabaseAPI.h"
#include "StreetsDatabaseAPI.h"

extern std::vector<double> streetSegmentLengths;

int main(int argc, char** argv) {

    std::string map_path;
    std::string osm_path;


    if (argc == 1) {
        //Use a default map
        map_path = "/cad2/ece297s/public/maps/toronto_canada.streets.bin";
    } else if (argc == 2) {
        //Get the map from the command line
        map_path = argv[1];
    } else {
        //Invalid arguments
        std::cerr << "Usage: " << argv[0] << " [map_file_path]\n";
        std::cerr << "  If no map_file_path is provided a default map is loaded.\n";
        return 1;
    }


    //enter a map path to load
    std::cout << "Enter map path" << std::endl;
    map_path = "/cad2/ece297s/public/maps/toronto_canada.streets.bin";

    //sets osm path to the correct path for entered map
    osm_path = map_path;
    int pos = osm_path.find("streets");
    if (pos != std::string::npos) {
        osm_path.replace(pos, 7, "osm"); // 7 = length("streets" )
    }

    //Load the map and related data structures
    bool load_success = load_map(map_path);
    if (!load_success) {
        std::cerr << "Failed to load map '" << map_path << "'\n";
        return 2;
    }

    //load osm data
    bool isOpenAPI = loadOSMDatabaseBIN(osm_path);

    std::cout << "Successfully loaded map '" << map_path << "'\n";



    //testing section
    draw_map();

    bool end = false;

    //Loads a new map
    while (end == false) {
        std::cout << "NEW MAP? ANSWER 'YES' IF SO" << std::endl;
        std::string answer;
        std::cin >> answer;
        if (answer == "YES" || answer == "yes") {
            std::cout << "Closing map\n";
            close_map();
            closeOSMDatabase();
            //enter path of new map
            std::cout << "Enter new map path" << std::endl;
            std::cin >> map_path;

            //sets osm path to the correct path for entered map
            osm_path = map_path;
            int pos = osm_path.find("streets");
            if (pos != std::string::npos) {
                osm_path.replace(pos, 7, "osm"); // 7 = length("streets" )
            }

            load_success = load_map(map_path);
            if (!load_success) {
                std::cerr << "Failed to load map '" << map_path << "'\n";
                return 2;
            }

            std::cout << "Successfully loaded map '" << map_path << "'\n";
            //You can now do something with the map
            isOpenAPI = loadOSMDatabaseBIN(osm_path);
            draw_map();
        } else {
            end = true;

        }
    }



    //Clean-up the map related data structures
    std::cout << "Closing map\n";

    close_map();
    closeOSMDatabase();

    return 0;
}
