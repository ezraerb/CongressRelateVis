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
/* This file defines a helper class that converts roll call data into the
    matrix of the vote differences between congresspeople. Also defines the
    actual results */

#include<bitset>

using std::bitset;
using std::vector; // Does not include header; widely used and clients should have it

/* The number of votes is large. This class can process them in blocks, to give memory
    savings at the cost of performance. The parameter below controls the block size
    (Bitwise operators only work on Bitset, not vector<bit>, and bitsize requires a size
    at compilation, so need to use a define, not a configuration parameter) */
#define ROLL_CALL_BLOCK_SIZE 8

typedef vector<vector<short> > VoteDiffMatrix;

class VoteFactory {
    private:
        typedef bitset<ROLL_CALL_BLOCK_SIZE> Votes;
        typedef vector<Votes> VoteResults;

        static void convVoteResultToDiff(VoteResults& passVotes, VoteResults& voted, VoteDiffMatrix& results);

        /* Finds the vote differences for a range of bills for a single year and adds them to the results.
            This method is private because it depends on consistency conditions enforced elsewhere */
        static void getVoteDiff(VoteDiffMatrix& results, short& billCount,
                                const CongressData& congress, short year);

    public:
        /* Calculate the vote differences. The congress data is passed in because
            the caller will also need it for other purposes */
        static void getVoteMatrix(VoteDiffMatrix& results, const CongressData& congress, short firstYear,
                                  short lastYear = 0);


        /* The final layout is based on the similarities of votes. Pairs of groups with large numbers of different
            votes will have little efffect on the final layout. Filter them out to reduce compute */
        static void filterLargeMismatch(VoteDiffMatrix& results, short threshold = 750);

        // Debug method to print out the vote resuls matrix
        static void debugOutputVoteMatrix(const VoteDiffMatrix& results);
};
