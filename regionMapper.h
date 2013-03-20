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
    updates. Please contact me through LinkedIn or github (my profile
    also has a link to the code depository)
*/
// A class to assign Congresspeople to regions of the country
#ifndef REGIONMAPPER_H
#define REGIONMAPPER_H

#include<map>
/* Deliberately not including string. Its so widely used the caller should
   deal with it */

using std::map;
using std::string;

/* This class sorts states into regions, numbered 1 to 4. Its designed for fast
   lookup. Since everything is hardcoded it should really be a singleton, but
   not worth the overhead */
class RegionMapper
{
    public:
        RegionMapper();

        // Returns the number of defined regions
        short getRegionCount() const;

        // Returns a region for a state abreviation. Invalid input returns 0
        short getRegion(const string& state) const;

        ~RegionMapper() {;} // Use the default

    private:
        map<string, short> _regionMap;
        short _maxRegion;
};

inline short RegionMapper::getRegion(const string& state) const
{
    map<string, short>::const_iterator index = _regionMap.find(state);
    if (index != _regionMap.end())
        return index->second;
    else
        return 0;
}

inline short RegionMapper::getRegionCount() const
{
    return _maxRegion;
}




#endif // REGIONMAPPER_H
