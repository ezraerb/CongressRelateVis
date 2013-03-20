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
// Class definition for an object to read roll call votes from XML files

/* NOTE: Including headers in other headers is a double edged sowrd. Sources that
   don't need those classes don't need the headers just to satisfy this one, but
   sources that DO use those classes don't need to include them either (and often won't),
   which creates a real pain finding where the header ultimately came from. In this case,
   the structures will not likely be used by users of these structures, so the risk is
   worth it. */
#include<string>
#include<fstream>
#include"xmlParser.h"

using std::string;
using std::fstream;
// NOTE: The header is NOT listed above, since it is used in many source files (and they should include it)
using std::pair;

class RollCall : public XmlParser {
public:
  /* A single vote from the file. Note that abstentions and the like
     do not count as votes in this context */
   typedef pair<int, bool> VoteRecord;

private:
  // XNL tage with wanted data
  static const string _optionKey;
  static const string _voterId;
  static const string _vote;

  bool _haveVotes; // True if votes exist which have not been processed yet

  /* The last vote read from the file. The class does a pre-fetch to set
     _haveVotes properly */
  VoteRecord _nextVote;

  // Resets object state for a new file
  void clearVoteData(void);

  // Finds the next vote in the file. Sets _haveVotes based on the result
  void loadNextVote(void);

  /* Assignment operator and copy constructor. The parser can't be copied
     (this requires the ability to open the same file twice, which is
     controlled by the operating system) so these are private */
  RollCall(RollCall &other);
  RollCall operator= (RollCall &other);

public:
  // Constructor and descructor
  RollCall();
  ~RollCall();

  // Opens a roll call vote and validate the file. Returns true if successful
  bool open(short year, int rollCallNo);

  // Returns true if more votes remain to read from the file
  inline bool haveVotes() { return _haveVotes; }

  // Returns the next vote from the file
  // NOTE: Can't be a reference due to the prefetch!
  VoteRecord getNextVote(void);
};
