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
#ifndef CORROLATION_H_INCLUDED
#define CORROLATION_H_INCLUDED

// This class deliberately does not include headers, because the concepts are widely used. Callers should handle it
using std::vector;

/* This class finds vote totals between and within political parties and regions. Comparing
   this data shows the corrolation of votes by those two factors. This doesn't add to the
   visualization result (the distance between nodes shows the corrolation automatically) but
   is highly useful for designing grouping categories
   WARNING: If a category contains only a few congresspersons, their votes will have outsized
   influence and skewer the results
   WARNING: This clas assumes different factors have weak linkage, so their influence can be
   calculated independent of each other */

typedef vector<vector<int> > CorrolationMatrix; // Ragged array

class CorrolationFactory {
public:
    static void getCorrolation(const CongressGroupDataList& congress, const VoteDiffMatrix& votes, bool party,
                               short partyFilter, CorrolationMatrix& corrolation);

    static void debugOutputCorrolation(const CorrolationMatrix& corrolation, bool party);
};

#endif // CORROLATION_H_INCLUDED
