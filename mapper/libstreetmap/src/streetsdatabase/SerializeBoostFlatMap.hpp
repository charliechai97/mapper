/*
 * SerializeBoostFlatMap.hpp
 *
 *  Created on: Jan 6, 2017
 *      Author: jcassidy
 */

#ifndef LIBS_STREETSDATABASE_SRC_SERIALIZEBOOSTFLATMAP_HPP_
#define LIBS_STREETSDATABASE_SRC_SERIALIZEBOOSTFLATMAP_HPP_

#include <boost/container/flat_map.hpp>
#include <boost/range/algorithm.hpp>

template<class Archive,typename Key,typename T,typename Compare,typename Allocator>
	void serialize_flat_map(Archive& ar,boost::container::flat_map<Key,T,Compare,Allocator>& m,const unsigned)
{
	BOOST_STATIC_ASSERT(Archive::is_saving::value ^ Archive::is_loading::value);

	if (Archive::is_saving::value)
	{
		std::vector<std::pair<Key,T>> v(m.size());
		boost::copy(m,v.begin());

		ar & v;
	}
	else if (Archive::is_loading::value)
	{
		std::vector<std::pair<Key,T>> v;
		ar & v;

		m.clear();
		m.insert(boost::container::ordered_unique_range,v.begin(),v.end());
	}
}


#endif /* LIBS_STREETSDATABASE_SRC_SERIALIZEBOOSTFLATMAP_HPP_ */
