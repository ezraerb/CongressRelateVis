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

// Extract data about members of the House of Representatives from XML files
#include<iostream>
#include<sstream>
#include<string>
#include<cctype>
#include<utility>
#include<vector>
#include<cstdlib>
#include"congressData.h"

/* This objerct reads roll call results from XML files, one per roll call.
   The design is very straightforward: look for key fields as defined in the
   data dictionary, extract the data between them, and return it. Searching for
   the key fields instead of blindly burning data future proofs the design
   against format charges, one of XMLs strenthgs */
/* WARNING: This object burns text searching for keywords, so it DOES assume that
    the order of the keywords in the file will not change */

using std::string;
using std::stringstream;
using std::istringstream;
using std::map;
using std::vector;
using std::endl;
using std::cerr;
using std::cout;
using std::ios_base;

const string CongressData::_refIdKey = "<person id=";
const string CongressData::_nameKey = " name="; // Note the leading space
const string CongressData::_roleKey = "<role type=";
const string CongressData::_startDateKey = " startdate="; // Note the leading space
const string CongressData::_partyKey = " party="; // Note the leading space
const string CongressData::_stateKey = " state="; // Note the leading space

// Constructor. Throws ios_base::failure if initialization fails
CongressData::CongressData(short sessionStartYear, bool parseTrace)
{
    try {
        if (parseTrace)
            setTrace();
        XmlParser::open("people.xml");
        if (!isOpen()) {
            stringstream errorText;
            errorText << "Required data file people.xml missing";
            cerr << errorText.str() << endl;
            throw ios_base::failure(errorText.str());
        }

        // Find first key, not finding it indicates data is out
        burnToKey(_refIdKey);
        while (haveTextToProcess()) {
            /* The XML keys representing a given congressperson are in
                a specific order, which this code depends on. It does not
                do sophisticated error checking, beyond ensuring that
                lines are well formed */
            burnChars(1); // Eliminate the '"'
            int refNo;
            if (!getNumberToToken('"', refNo)) {
                stringstream errorText;
                // Note the escape to insert a litteral " into the message
                errorText << "people.xml parse fail, bad value for " << _refIdKey << " key. Closing \" missing";
                cerr << errorText.str() << endl;
                throw ios_base::failure(errorText.str());
            }

            CongressPerson newData;
            newData._name = getTextForKey(_nameKey);
            string roleType(getTextForKey(_roleKey));
            string startDate(getTextForKey(_startDateKey));
            newData._party = getTextForKey(_partyKey, false);
            newData._state = getTextForKey(_stateKey, false);

            // The file mixes representatives and senators. Discard non-represntatives.
            /* NOTE: A senator may appear in the name list anyway. This is a quirk of
                the data, not an error in the code. When someone serves in the house and
                then takes over a Senate term, the input title uses their later title for
                the house term data */
            if (roleType == "rep") {
                /* Need to filter for the wanted Congressional session. For representatives, this
                    is easy, since they are elected every session. Extract the year from the
                    start date and compare it to the wanted session year. The year is first in
                    the date format, and always has four digits
                    TRICKY NOTE: A Representative can start in the middle of the session thanks
                    to death, resignations, etc. Allow a start date one year later as well
                    SEMI-HACK: This is not the 'official' way of extracting a numeric from a
                    string, but it is the fastest */
                short year = atoi(string(startDate,0, 4).c_str());

                if ((year == sessionStartYear) ||
                    (year == (sessionStartYear + 1))) {
                    /* Insert the new reference ID in the map to internal indexes. Since indexes
                        start from zero, the current size of the vector gives the index value.
                        A failed insert indicates a duplicate reference ID, which is a huge
                        data error */
                    if (!_refIndexMap.insert(std::make_pair(refNo,
                                                            (unsigned short)_congressData.size())).second) {
                        stringstream errorText;
                        errorText << "people.xml parse fail, person id key value " << refNo << " duplicated";
                        cerr << errorText.str() << endl;
                        throw ios_base::failure(errorText.str());
                    } // Map insert failed
                    _congressData.push_back(newData);
                } // Year matches wanted session
            } // Role is Representative
            // Find next set of keys
            burnToKey(_refIdKey);
        } // Text to search

        /* Check for the correct number of Congressmen. This is harder than it sounds. People can
            resign, die, or otherwise get replaced during the term, in which case two names
            appear for the same district, with different IDs. The parser is more likely to skip
            data rather than generate extra, so an estimate of an upper limit should be good enough */
        if ((_congressData.size() < 435) || (_congressData.size() > 460)) {
            stringstream errorText;
            errorText << "people.xml parse fail, expected between 435 and 460 values, found " << _congressData.size();
            cerr << errorText.str() << endl;
            throw ios_base::failure(errorText.str());
        }

        // Add an extra blank data, used to handle errors
        CongressPerson temp;
        _congressData.push_back(temp);
    }
    catch (...) {
        // Ensure memory does not leak
        _refIndexMap.clear();
        _congressData.clear();
        /* NOTE: Superclass constructor will be called, so don't need
            to clean up the actual file */
        throw;
    }

}

CongressData::~CongressData()
{
    // Do nothing; subclass destructor does everything
}

/* Extracts the text value for a given key. Throws ios_base::failure
    if not found. The flag indicates whether the key field must appear on the
    current line to be valid */
string CongressData::getTextForKey(const string& key, bool wrapLine)
{
    // Find first key, not finding it indicates an inconsistency
    burnToKey(key, wrapLine);
    if (haveLineEnd()) {
        stringstream errorText;
        errorText << "people.xml parse fail, key " << key << " missing";
        cerr << errorText.str() << endl;
        throw ios_base::failure(errorText.str());
    }
    burnChars(1); // Eliminate the '"'
    string result(getTextToToken('"'));

    // The end token must exist, so reaching the end of the line indicates an error
    if (haveLineEnd()) {
        stringstream errorText;
        // Note the escape to insert a litteral " into the message
        errorText << "people.xml parse fail, bad value for key " << _refIdKey << ". Closing \" missing";
        cerr << errorText.str() << endl;
        throw ios_base::failure(errorText.str());
    }
    return result;
}
