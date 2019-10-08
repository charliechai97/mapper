#pragma once
#include "OSMEntity.h"
#include <vector>


/** Layer-1 OSM relation
 *
 * A relation is an OSM entity that also holds a set of members.
 * Each member has a role and is referred to by type (node/way/rel) and OSM ID.
 *
 * Role is stored as an integer index into a string table.
 */

class OSMRelation : public OSMEntity
{
public:
	struct Member;

	OSMRelation(){}
    OSMRelation(OSMID id_,std::vector<std::pair<TagStringFlyweight,TagStringFlyweight>>&& tags_,std::vector<Member>&& members_) :
    	OSMEntity(id_,std::move(tags_)),
    	m_members(std::move(members_))
		{}

    const std::vector<Member>& members() const { return m_members; }

private:
    std::vector<Member> m_members;

    template<class Archive>void serialize(Archive& ar, const unsigned)
    	{ ar & boost::serialization::base_object<OSMEntity>(*this) & m_members; }
    friend boost::serialization::access;

};

struct OSMRelation::Member
{
	TypedOSMID			tid;
    TagStringFlyweight 	role; 		// string table index

private:
    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive& ar, const unsigned)
    	{ ar & tid & role; }
};
