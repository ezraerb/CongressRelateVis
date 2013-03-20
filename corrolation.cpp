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
#include<iostream>
#include<iomanip>
#include<vector>
#include"congressData.h" // Needed by voteFactory.h
#include"voteFactory.h"
#include"regionMapper.h" // Needed by clusterFactory.h
#include"clusterFactory.h"
#include"corrolation.h"

using std::vector;
using std::cerr;
using std::endl;

/* Find corrolation percentages of pairs of conresspeople, given their vote differences.
    This method exists to find useful differentiating factors between groups for the
    graph symbols */
void CorrolationFactory::getCorrolation(const CongressGroupDataList& congress, const VoteDiffMatrix& votes,
                                        bool party, short partyFilter, CorrolationMatrix& corrolation)
{
    corrolation.clear();
    if (congress.empty() || votes.empty())
        return;

    // Find number of categories being tested
    unsigned short categoryCount;
    if (party)
        categoryCount = congress[0]._parties.size();
    else
        categoryCount = congress[0]._regions.size();

    // Create the results array. To avoid double counting, it is ragged
    vector<int> temp;
    corrolation.assign(categoryCount, temp);
    for (unsigned short index = 0; index < corrolation.size(); index++)
        corrolation[index].assign(index + 1, 0);

    /* Iterate through the votes. The indexes are the same as the congress data.
       For every vote difference count, find the characteristic groups of the
       congresspeople who made those votes, and the number of congresspeople who
       had each characteristic. The vote total gets updated by the product of the
       vote difference times the number of congressmen with one characteristic times
       the number of congressmen with the other. This is best illustrated with an
       example:

       Group 1 has 1 D and 5 R
       Group 2 has 4 D and 3 R

       The vote difference for the two groups is 100.

       The 1 D in group 1 had 100 vote differences with each of 4 D in group 2,
       leading to a total of 100 * 1 * 4 = 400 differences
       The 5 R in group 1 had 100 vote differences (the same votes) with each of 3 R
       in group 2, leading to a total of 100 * 5 * 3 = 1500 differences.

       The differences between D and R is ((1 * 3) + (4 * 5)) * 100 = 2300.

       Different categories can have different number of congresspeople. To control for
       this count the total number of votes included in the calculation between two
       categories, and then normalized. Since the vote differneces are normalized,
       the normalized total number of votes equals the number of people that
       participated
    */

    CorrolationMatrix voteCount;
    voteCount.assign(categoryCount, temp);
    for (unsigned short index = 0; index < corrolation.size(); index++)
        voteCount[index].assign(index + 1, 0);

    // The vote list is a ragged array
    unsigned short index1, index2, index3, index4;
    for (index1 = 1; index1 < votes.size(); index1++)
        for (index2 = 0; index2 < index1; index2++)
            for (index3 = 0; index3 < categoryCount; index3++)
                for (index4 = 0; index4 < categoryCount; index4++)
                    /* Party loyalty can be so high that it swamps other factors. To show
                        the rest, need to filter by party */
                    if ((partyFilter >= 3) ||
                        ((congress[index1]._parties[partyFilter] > 0) &&
                         (congress[index2]._parties[partyFilter] > 0))) {
                        /* Find number of congresspeople with the wanted category in
                            each group */
                        short groupCount1, groupCount2;
                        if (party) {
                            groupCount1 = congress[index1]._parties[index3];
                            groupCount2 = congress[index2]._parties[index4];
                        }
                        else {
                            groupCount1 = congress[index1]._regions[index3];
                            groupCount2 = congress[index2]._regions[index4];
                        }

                        /* Results is also a ragged array, so the lower index
                            must be first */
                        /* SUBTLE NOTE: Many category counts will be zero. Put these
                            first in the evaluation since multiply by zero is often
                            optimized. */
                        // If the votes are too large to consider for the layout, treat it as 100%
                        int voteDiff = votes[index1][index2];
                        if (voteDiff < 0)
                            voteDiff = 1000;
                        if (index3 >= index4) {
                            corrolation[index3][index4] += groupCount1 * groupCount2 * voteDiff;
                            voteCount[index3][index4] += groupCount1 * groupCount2;
                        }
                        else {
                            corrolation[index4][index3] += groupCount1 * groupCount2 * voteDiff;
                            voteCount[index4][index3] += groupCount1 * groupCount2;
                        }
                    } // If within for loops

    // Now normalize
    for (index1 = 0; index1 < corrolation.size(); index1++)
        for (index2 = 0; index2 < corrolation[index1].size(); index2++)
            if (voteCount[index1][index2] != 0)
                corrolation[index1][index2] /= voteCount[index1][index2];
}

// Print the corrolation results
void CorrolationFactory::debugOutputCorrolation(const CorrolationMatrix& corrolation, bool party)
{
    // Set up formatted numeric output: width 4 right justified
    cerr << std::setw(4);

    // Output the column header
    cerr << "     ";
    unsigned short index, index2;
    for (index = 0; index < corrolation.size(); index++) {
        // For the party matrix, people want symbols
        if (party && (index < 3)) {
            if (index == 0)
                cerr << "   D ";
            else if (index == 1)
                cerr << "   R ";
            else
                cerr << "   I ";
        }
        else
            cerr << std::setw(4) << index << " ";
    }
    cerr << endl;

    for (index = 0; index < corrolation.size(); index++) {
        // For the party matrix, people want symbols
        if (party && (index < 3)) {
            if (index == 0)
                cerr << "   D ";
            else if (index == 1)
                cerr << "   R ";
            else
                cerr << "   I ";
        }
        else
            cerr << std::setw(4) << index << " ";
        for (index2 = 0; index2 < corrolation[index].size(); index2++)
            cerr << std::setw(4) << corrolation[index][index2];
        cerr << endl;
    } // Outer for loop
}
