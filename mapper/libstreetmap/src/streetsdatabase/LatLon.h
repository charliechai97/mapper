/*
 * LatLon.h
 *
 *  Created on: Dec 17, 2015
 *      Author: jcassidy
 */

#ifndef LATLON_H_
#define LATLON_H_

#include <limits>
#include <iostream>
#include <utility>
#include <array>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <boost/serialization/access.hpp>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

/** Latitude and longitude in decimal degrees
 *
 * (lat,lon) are held as single-precision float to save space and match the precision given by OSM, but returned as double because
 * single-precision arithmetic has been found to cause precision errors. Always use double math. The return cast ensures this is
 * the default case.
 *
 * +lat is north
 * +lon is east
 */

class LatLon
{
public:
    LatLon(){}
    explicit LatLon(float lat_, float lon_) : m_lat(lat_),m_lon(lon_){}

    double lat() const { return m_lat; }
    double lon() const { return m_lon; }

private:
    float m_lat = std::numeric_limits<float>::quiet_NaN();
    float m_lon = std::numeric_limits<float>::quiet_NaN();

    friend class boost::serialization::access;
    template<class Archive>void serialize(Archive& ar, unsigned int)
    	{ ar & m_lat & m_lon; }
};

std::ostream& operator<<(std::ostream& os,LatLon);


std::array<LatLon,4> bounds_to_corners(std::pair<LatLon,LatLon> bounds);


#endif /* LATLON_H_ */
