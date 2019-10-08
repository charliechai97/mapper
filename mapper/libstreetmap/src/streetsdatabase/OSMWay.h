#pragma once
#include "OSMEntity.h"

#include <vector>

/** Layer-1 OSM way
 *
 * An OSM way is an OSMEntity with a sequence of node references.
 *
 * The way may be an open, linear feature (eg. road) or a closed polygon (eg. park)
 */

class OSMWay : public OSMEntity
{
public:
	OSMWay(){}
    OSMWay(const OSMWay&) = default;
    OSMWay(OSMWay&& w) = default;
    OSMWay& operator=(const OSMWay&) = default;

    OSMWay(OSMID id_,std::vector<std::pair<TagStringFlyweight,TagStringFlyweight>>&& tags_,std::vector<OSMID>&& ndrefs_) :
    	OSMEntity(id_,std::move(tags_)),
		m_ndrefs(std::move(ndrefs_)){}

    /// Determine if the way is closed (represents a polygon) or open (represents a linear feature)
    bool isClosed() const {
        return m_ndrefs.front() == m_ndrefs.back();
    }

    // data access
    const std::vector<OSMID>& ndrefs() const {
        return m_ndrefs;
    }

private:
    std::vector<OSMID> m_ndrefs;

    friend std::ostream& operator<<(std::ostream& os,const OSMWay& w)
    {
    	return os << "Way id  W" << w.id() << " with " << w.ndrefs().size() << " node refs";
    }

    friend class boost::serialization::access;
    template<class Archive>void serialize(Archive& ar, unsigned int)
    	{ ar & boost::serialization::base_object<OSMEntity>(*this) & m_ndrefs; }
};
