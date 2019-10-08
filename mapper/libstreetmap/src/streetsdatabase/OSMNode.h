#pragma once
#include "OSMEntity.h"

#include <boost/serialization/vector.hpp>

#include "LatLon.h"

#include <limits>

/** Layer-1 OSM node (attributes + coordinates)
 *
 */

class OSMNode : public OSMEntity
{
public:
	OSMNode(){}
    OSMNode(OSMID id,std::vector<std::pair<TagStringFlyweight,TagStringFlyweight>>&& tags,LatLon coords);

    OSMNode(OSMNode&&) = delete;
    OSMNode(const OSMNode&) = default;
    OSMNode& operator=(const OSMNode&)=default;
    OSMNode& operator=(OSMNode&&) = delete;

    LatLon	coords()	const { return m_coords; }

private:
    LatLon m_coords;

    friend class boost::serialization::access;
    template<class Archive>void serialize(Archive& ar, const unsigned)
    	{ ar & boost::serialization::base_object<OSMEntity>(*this) & m_coords; }

};

std::ostream& operator<<(std::ostream&, const OSMNode&);
