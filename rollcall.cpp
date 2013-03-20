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

/* Extract Roll call votes from an XML file and return as a bit array
    indexed by the internal congressperson refno values */
#include<iostream>
#include<sstream>
#include<string>
#include<cctype>
#include<utility>
#include"rollcall.h"

/* This objerct reads roll call results from XML files, one per roll call.
   The design is very straightforward: look for key fields as defined in the
   data dictionary, extract the data between them, and return it. Searching for
   the key fields instead of blindly burning data future proofs the design
   against format charges, one of XMLs strenthgs */
/* WARNING: This object burns text searching for keywords, so it DOES assume that
    the order of the keywords in the file will not change */

using std::string;
using std::stringstream;
using std::endl;
using std::cerr;
using std::cout;

const string RollCall::_optionKey = "<option key=";
const string RollCall::_voterId = "<voter id=";
const string RollCall::_vote = " vote="; // Note the leading space

// Constructor
RollCall::RollCall()
{
   clearVoteData();
}

RollCall::~RollCall()
{
    // Do nothing; subclass destructor does everything
}

// ReclearFile the ojbect to process a file
void RollCall::clearVoteData()
{
    _haveVotes = false;
    _nextVote.first = -1;
    _nextVote.second = false;
}

// Opens a roll call vote and validate the file. Returns true if successful
bool RollCall::open(short year, int rollCallNo)
{
    // Clear processing data from previous file, if any
    clearVoteData();
    /* The roll call file name has the following format: h-yyyy-xxxx.
    NOTE: The code below is the 'official' way of forming file names in
    C++. It's clunky enough that many just do a sprintf into a formatted C style
    string. The code below has more formatting flexibility however
    NOTE: Remember to include the file extension in the name, even though
    most people don't see it in Windows! */
  stringstream fileName;
  fileName << "h" << year << "-" << rollCallNo << ".xml";
  XmlParser::open(fileName.str());
  if (!isOpen())
    cerr << "Could not find roll call " << rollCallNo << " for year " << year << "." << endl;
  else {
    // Validate the file
    /* Roll call votes are recorded using particular words, which vary
        with the votes. Each XML file contains a header showing how the
        words translate to actual votes. Not all votes affect bills.
        Search the header for translations to votes on bills, and discard
        files that don't have them
        Since the fields are always in the header, just burn the file until
        find them */
    bool rightVotes = false;
    burnToKey(_optionKey);
    if (!haveLineEnd()) {
        /* Found the XML key field. Extract its value and compare to the list of
            allowed values. A value that indicates an up-or-down vote signals a
            valid file
            NOTE 1: This routine assumes the vote file has enough consistency that
            the presence of one value implies the presence of the rest.
            NOTE 2: This routine ALSO assumes that sets of allowed values for votes
            are exclusive between files, so the presence of an up-or-down vote token
            indicatees there will not be other types of votes in the same file. At
            the time this was written, it was true by definition because each file
            contains one and only one roll call vote */
        string value(getText(3));
        /* The format of all fields is [key]="[vakye]". All the wanted fields are single
            chars, so test for the quotes to detect a longer value */
        if (value.length() == 3) {
           if ((value[0] == '"') &&
               (value[2] == '"') &&
               ((value[1] == '+') || // For
                (value[1] == '-') || // Against
                (value[1] == '0') || // Abstain
                (value[2] == 'P'))) // Present
                rightVotes = true;
        } // String long enough to have the value
    } // Found the key value in the file
    if (!rightVotes) {
        /* Either did not find the vote type key field, or the vote is the wrong type
            Close file and reject */
        cerr << "Roll Call Vote " << rollCallNo << " rejected. ";
        if (!haveEOF())
            cerr << "Not an up or down vote";
        else
            cerr << "No Vote Data";
        cerr << endl;
        close(); // Close after message so EOF test works properly
   }
    else
        // Prefetch the first vote.
        loadNextVote();
  } // Opened the file
  return _haveVotes;
}

// Returns the next vote from the file
// NOTE: Can't be a reference due to the prefetch!
RollCall::VoteRecord RollCall::getNextVote(void)
{
    /* This method uses a prefetch to set the next vote
        status properly. It saves the previous result,
        finds the next one, and then returns it. This will
        cause a minimum of three copies per call, but the
        structure is small enough the overhead is worth it */
    RollCall::VoteRecord result(_nextVote);
    if (_haveVotes)
        loadNextVote();
    return result;
}

// Finds the next vote in the file. Sets _haveVotes based on the result
void RollCall::loadNextVote(void)
{
    /* A vote is composed of two values, with different key values. Search for
        one, than the other. Don't bother searching for a lack of file text,
        because the search routine is smart enough to handle this */
    _haveVotes = false;

    /* Certain votes in the file are not considered vote results, so need to burn
        them when found */

    while ((!_haveVotes) && haveTextToProcess()) {
        burnToKey(_voterId);
        burnChars(1); // Eliminate the '"'
        if (getNumberToToken('"', _nextVote.first)) {
            burnToKey(_vote, false); // If this key is not on same line as previous, bad input
            if (!haveLineEnd()) { // Found something
                burnChars(1); // Eliminate the '"'
                string voteChar(getTextToToken('"'));
                if (voteChar.length() == 1) {
                    // Data is right size
                    if ((voteChar[0] == '+') ||
                        (voteChar[0] == '-')) {
                        // Votes to pass or reject a bill
                        _nextVote.second = (voteChar[0] == '+');
                        _haveVotes = true;
                    } // Pass or reject vote result
                } // Value is correct length
            } // Second key found on same line as first
        } // Value is numeric
    } // No vote and still have data to search
    if (!_haveVotes)
        // Didn't find anything, so clear previous result
        clearVoteData();
    return;
}

