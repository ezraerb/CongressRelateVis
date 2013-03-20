/* This file is part of CongressRelationVis. This project creates graphs
   showing simmilarity of voting of different members of the US House of
   Representatives.

    Copyright (C) 2013   Ezra Erb

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    I'd appreciate a note if you find this program useful or make
    updates. Please contact me through LinkedIn or github (my profile also has
    a link to the code depository)
*/
#include<string>
#include<algorithm>
#include "regionMapper.h"

using std::string;
using std::make_pair;

/* This class sorts states into regions, numbered 1 to 4. Its designed for fast
   lookup. Since everything is hardcoded it should really be a singleton, but
   not worth the overhead */
RegionMapper::RegionMapper()
{
    _regionMap.insert(make_pair("AL", 2));
    _regionMap.insert(make_pair("AK", 4));
    _regionMap.insert(make_pair("AZ", 4));
    _regionMap.insert(make_pair("AR", 2));
    _regionMap.insert(make_pair("CA", 4));
    _regionMap.insert(make_pair("CO", 3));
    _regionMap.insert(make_pair("CT", 1));
    _regionMap.insert(make_pair("DE", 1));
    _regionMap.insert(make_pair("DC", 1));
    _regionMap.insert(make_pair("FL", 2));
    _regionMap.insert(make_pair("GA", 2));
    _regionMap.insert(make_pair("HI", 4));
    _regionMap.insert(make_pair("ID", 3));
    _regionMap.insert(make_pair("IL", 1));
    _regionMap.insert(make_pair("IN", 1));
    _regionMap.insert(make_pair("IA", 3));
    _regionMap.insert(make_pair("KS", 3));
    _regionMap.insert(make_pair("KY", 2));
    _regionMap.insert(make_pair("LA", 2));
    _regionMap.insert(make_pair("ME", 1));
    _regionMap.insert(make_pair("MD", 1));
    _regionMap.insert(make_pair("MA", 1));
    _regionMap.insert(make_pair("MI", 1));
    _regionMap.insert(make_pair("MN", 3));
    _regionMap.insert(make_pair("MS", 2));
    _regionMap.insert(make_pair("MO", 3));
    _regionMap.insert(make_pair("MT", 3));
    _regionMap.insert(make_pair("NE", 3));
    _regionMap.insert(make_pair("NV", 3));
    _regionMap.insert(make_pair("NH", 1));
    _regionMap.insert(make_pair("NJ", 1));
    _regionMap.insert(make_pair("NM", 3));
    _regionMap.insert(make_pair("NY", 1));
    _regionMap.insert(make_pair("NC", 2));
    _regionMap.insert(make_pair("ND", 3));
    _regionMap.insert(make_pair("OH", 1));
    _regionMap.insert(make_pair("OK", 3));
    _regionMap.insert(make_pair("OR", 4));
    _regionMap.insert(make_pair("PA", 1));
    _regionMap.insert(make_pair("RI", 1));
    _regionMap.insert(make_pair("SC", 2));
    _regionMap.insert(make_pair("SD", 3));
    _regionMap.insert(make_pair("TN", 2));
    _regionMap.insert(make_pair("TX", 2));
    _regionMap.insert(make_pair("UT", 3));
    _regionMap.insert(make_pair("VT", 1));
    _regionMap.insert(make_pair("VA", 2));
    _regionMap.insert(make_pair("WA", 4));
    _regionMap.insert(make_pair("WV", 1));
    _regionMap.insert(make_pair("WI", 3));
    _regionMap.insert(make_pair("WY", 3));
    _regionMap.insert(make_pair("GU", 4));
    _regionMap.insert(make_pair("VI", 2));
    _regionMap.insert(make_pair("AS", 4));
    _regionMap.insert(make_pair("PR", 2));
    _regionMap.insert(make_pair("MP", 4));

    /* Find the maximum region in the map
        NOTE: Why go through all this work? The regions are hardcoded above,
        just set it! Future code changes could add or drop regions, without
        updating the hardcoded value, leading to all sorts of problems. This
        ensures consistency */
    _maxRegion = 0;
    map<string, short>::iterator index;
    for (index = _regionMap.begin(); index != _regionMap.end(); index++)
        if (index->second > _maxRegion)
            _maxRegion = index->second;
}
