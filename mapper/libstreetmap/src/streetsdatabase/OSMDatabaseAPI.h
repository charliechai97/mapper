#ifndef OSMDATABASEAPI_H
#define OSMDATABASEAPI_H
#include "OSMEntity.h"

#include "OSMNode.h"
#include "OSMWay.h"
#include "OSMRelation.h"

/**********************************************************************************************************************************
 * LAYER-1 API INTRODUCTION
 *
 * ** This is a more complex API than layer 2 which is built on top of it; please start with layer 2. The information accessible here
 * is not _required_ for any milestone, though it may help you provide additional features. **
 *
 * The Layer-1 API handles parsing the OSM XML data, and removes some features which are not of interest (eg. timestamps, the user
 * names who made changes) to conserve time and space. It also loads/saves a binary format that is pre-parsed and so more compact and
 * faster to load.
 *
 *
 *
 * The object model of this library and API follows the OSM model closely. It is a lean, flexible model with only three distinct
 * entity types, each of which carries all of its non-spatial data as attributes.
 *
 * There are three types of OSM Entities:
 * 		Node			A point with lat/lon coordinates and zero
 * 		Way				A collection of nodes, either a path (eg. street, bike path) or closed polygon (eg. pond, building)
 * 		Relation		A collection of nodes, ways, and/or relations that share some common meaning (eg. large lakes/rivers)
 *
 * Each entity may have associated zero or more attributes of the form key=value, eg. name="CN Tower" or type="tourist trap".
 *
 * After processing by osm2bin, files are stored in {mapname}.osm.bin ready to use in this API.
 */

// load the optional layer-1 OSM database
bool loadOSMDatabaseBIN(const std::string&);
void closeOSMDatabase();




/**********************************************************************************************************************************
 * Entity access
 *
 * Provides functions to iterate over all nodes, ways, relations in the database.
 *
 * NOTE: The indices here have no relation at all to the indices used in the layer-2 API, or to the OSM IDs.
 *
 * Once you have the OSMNode/OSMWay/OSMRelation pointer, you can use it to access methods of those types or the tag interface
 * described in the section below.
 */

unsigned getNumberOfNodes();
unsigned getNumberOfWays();
unsigned getNumberOfRelations();

const OSMNode* 		getNodeByIndex		(unsigned idx);
const OSMWay* 		getWayByIndex		(unsigned idx);
const OSMRelation* 	getRelationByIndex	(unsigned idx);




/**********************************************************************************************************************************
 * Entity tag access
 *
 * OSMNode, OSMWay, and OSMRelation are all objects derived from OSMEntity, which carries attribute tags.
 * The functions below allow you to iterate through the tags on a given entity acquired above, for example:
 *
 * for(unsigned i=0;i<getTagCount(e); ++i)
 * {
 * 		std::string key,value;
 * 		std::tie(key,value) = getTagPair(e,i);
 * 		// ... do useful stuff ...
 * }
 */

unsigned getTagCount(const OSMEntity* e);
std::pair<std::string, std::string> getTagPair(const OSMEntity* e, unsigned idx);
#endif
