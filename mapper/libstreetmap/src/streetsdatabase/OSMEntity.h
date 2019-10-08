#pragma once
#include <vector>
#include <boost/container/flat_map.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>

#include "SerializeBoostFlatMap.hpp"

#include "OSMID.h"
#include "IndexedFlyweight.hpp"

typedef IndexedFlyweightFactory<std::string,unsigned> 	TagStringFlyweightFactory;
typedef TagStringFlyweightFactory::Flyweight 			TagStringFlyweight;
typedef TagStringFlyweightFactory::BoundFlyweight		TagStringBoundFlyweight;


/** OSMEntity is a base class for OSMNode, OSMWay, and OSMRelation that holds the OSM ID and the tags.
 *
 * To save space, tag keys and values are stored as "flyweights" that store a reference (index) into a vector
 * of strings held in the OSMDatabase. That permits encoding a key-value pair as two integers rather than saving
 * all of the bytes of the string, which has advantages when there is heavy reuse.
 *
 * On its own, the flyweight (which is really just an index) can only be assigned and compared for equality.
 *
 * It must be bound to the table that generated it to be able to produce a string by looking up.
 * Once bound, the implicit conversion operator permits accessing the bound flyweight as if it was a const std::string&.
 *
 */

class OSMEntity
{
private:
	typedef boost::container::flat_map<TagStringFlyweight,TagStringFlyweight,TagStringFlyweight::IndexOrder>	container_type;

public:
	OSMEntity(){}
    OSMEntity(
    		OSMID id,
			std::vector<std::pair<TagStringFlyweight,TagStringFlyweight>>&& tags);

    OSMEntity(OSMEntity&&) = default;
    OSMEntity(const OSMEntity&) = default;
    OSMEntity& operator=(const OSMEntity&) = default;
    OSMEntity& operator=(OSMEntity&&) = default;

    virtual ~OSMEntity();

    OSMID id() const { return m_id; }

    /** Provides an object that permits iteration ( .begin()/.end() ) and lookup by TagStringFlyweight key []
     * Values returned are TagStringFlyweight type, and must be passed to the corresponding TagStringFlyweightFactory to be
     * converted to strings. */
    class TagLookup;
    TagLookup tags() const;

private:
    OSMID			m_id;
    container_type 	m_tags;

    // load/save via boost serialization
    template<class Archive>void serialize(Archive& ar, const unsigned ver)
    	{ ar & m_id;
    		serialize_flat_map(ar,m_tags,ver); }
    friend class boost::serialization::access;
};

class OSMEntity::TagLookup
{
public:
	TagLookup(const container_type& tags) :
		m_tags(tags){}

	// provide lookup, returning an invalid TagStringFlyweight if key does not exist
	TagStringFlyweight operator[](const TagStringFlyweight k) const;
	std::pair<TagStringFlyweight,TagStringFlyweight> operator[](unsigned i) const;

	std::size_t			size() const { return m_tags.size(); }

	// provide iteration
	OSMEntity::container_type::const_iterator begin() 	const { return m_tags.begin(); }
	OSMEntity::container_type::const_iterator end() 	const { return m_tags.end(); }

private:
	const container_type& 		m_tags;
};

inline OSMEntity::TagLookup OSMEntity::tags() const { return TagLookup(m_tags); }
