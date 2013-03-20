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
// This file defines an object to hold basic data about congressmen.

/* NOTE: Including headers in other headers is a double edged sowrd. Sources that
   don't need those classes don't need the headers just to satisfy this one, but
   sources that DO use those classes don't need to include them either (and often won't),
   which creates a real pain finding where the header ultimately came from. In this case,
   the structures will not likely be used by users of these structures, so the risk is
   worth it. */
#include<string>
#include<fstream>
#include<map>
#include<climits>
#include"xmlParser.h"

using std::string;
using std::fstream;
using std::map;

// NOTE: This header is NOT listed above, since it is used in many source files (and they should include it)
using std::vector;

/* This clsss provides data about Congresspeople. In the XML file, it is indexed by
   a ref_id value. These are assigned based on when someone was first elected, going back
   to the first Congress! Using this as an index for the data creates a sparse array. It
   cries out for a map, except that most implementations are memory hogs. The design used
   here is to collapse the map of all data into an array with an internal index. The actual
   map then holds only the internal index, reducing memory usage significantly. Indexing
   arrays by congressperson is used elsewhere in this project, so the ref_id translation
   needs to be public */
class CongressData : public XmlParser {
public:
    struct CongressPerson {
        string _name;
        string _party;
        string _state;
        // Use default constructor and destructor
    };

private:
  // XNL tage with wanted data
  static const string _refIdKey;
  static const string _nameKey;
  static const string _roleKey;
  static const string _startDateKey;
  static const string _partyKey;
  static const string _stateKey;

  map<int, unsigned short> _refIndexMap;
  vector<CongressPerson> _congressData;

  /* Extracts the text value for a given key. Throws ios_base::failure
    if not found. The flag indicates whether the key field must appear on the
    current line to be valid */
  string getTextForKey(const string& key, bool wrapLine = true);

  /* Assignment operator and copy constructor. The parser can't be copied
     (this requires the ability to open the same file twice, which is
     controlled by the operating system) so these are private */
  CongressData(CongressData &other);
  CongressData operator= (CongressData &other);

public:
    /* Load data about congresspersons for the session starting the given year.
        Second flag logs the file input during parsing */
    CongressData(short sessionStartYear, bool parseTrace=false);

    ~CongressData();

	/* Gets the data on a representative given their array index.
        Note that this is not the RefNo from the vote data (see below for that) */
    const CongressPerson& getData(unsigned short indexNo) const;

    /* Given the refNo from the XML data, gets the array index. Internally this
        is a lookup on a map. A missing congressman returns MAXSHORT */
    int getIndexNo(int refNo) const;

	/* Returns the number of congressmen in the system. Thanks to midterm changes
        (resignations, death, etc) the number may not equal 435. */
    unsigned short getSize() const;
};

/* Gets the data on a representative given their array index.
    Note that this is not the RefNo from the vote data (see below for that) */
inline const CongressData::CongressPerson& CongressData::getData(unsigned short indexNo) const
{
    if (indexNo > _congressData.size())
        return _congressData.back();
    else
        return _congressData.at(indexNo);
}

/* Given the refNo from the XML data, gets the array index. Internally this
    is a lookup on a map. A missing congressman returns MAXSHORT; */
inline int CongressData::getIndexNo(int refNo) const
{
    // Yes, its a clunky definition, but this is the only iterator in the class
    map<int, unsigned short>::const_iterator index = _refIndexMap.find(refNo);
    if (index != _refIndexMap.end())
        return index->second;
    else
        return SHRT_MAX;
}

inline unsigned short CongressData::getSize() const
{
    /* Remember that the last entry in the vector is an out of bounds
        default. Do not include it in the size results */
    if (!_congressData.size()) // Some error setting up the object
        return 0;
    else
        return _congressData.size() - 1;
}
