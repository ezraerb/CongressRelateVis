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
/* This file defines a class that reads vote data and then converts it
    to the matrix of the vote differences between congresspeople, plus
    some routines for manipulating the results matrix */
#include<iostream>
#include<vector>
#include<climits>
#include<iomanip>
#include<sstream>
#include"congressData.h"
#include"voteFactory.h"
#include"rollCall.h"

// Number of consecutive bad vote files that will make this class quit
#define FAILURES_FOR_QUIT 5
#define SCREEN_WIDTH 80

using std::bitset;
using std::vector;
using std::cerr;
using std::endl;
using std::ios_base;
using std::stringstream;

/* Given how congresspeople voted on bills, finds the number of differences in their
    votes and updates the results matrix */
void VoteFactory::convVoteResultToDiff(VoteResults& passVotes, VoteResults& voted, VoteDiffMatrix& results)
{
    /* The definition of two congresspersons having different votes on a given roll call:
        (A not voted) OR (B not voted) OR (A vote XOR B vote). Declarinng a mismatch if
        either did not vote is required to satisfy the triangle principle so distance based
        clustering can be used later on. The triangle principle states that
        value [A, B] + value [B + C] <= value [A, C]. (if the vote differneces are thought of
        as physical spacings on a map, the three congresspeople will form a traingle) */

    /* The vote results come in with bits set for when people voted. Need to know
        when they did NOT vote, so flip them */
    // NASTY HACK: Avoid undefined behavior if results not set up correctly
    if (!results.size())
        return;
    VoteResults notVoted(voted);
    VoteResults::iterator tempIndex;
    for (tempIndex = notVoted.begin(); tempIndex != notVoted.end(); tempIndex++)
        tempIndex->flip();
    unsigned short first, second;
    // The last congressperson has nobody to compare with, so ignore them with the '-1'
    for (first = 0; first < results.size() - 1; first++) {
        for (second = first + 1; second < results.size(); second++) {
            Votes temp(passVotes[first]);
            temp ^= passVotes[second];
            temp |= notVoted[first];
            temp |= notVoted[second];
            results.at(first).at(second) += temp.count();
            results.at(second).at(first) += temp.count();
        } // Inner congressperson loop
    } // Outer congressperson loop
}

/* Finds the vote differences for a single year and adds them to the results.
    This method is private because it depends on consistency conditions enforced elsewhere */
void VoteFactory::getVoteDiff(VoteDiffMatrix& results, short& billCount,
                              const CongressData& congress, short year)
{
    RollCall rollCall;
//    rollCall.setTrace();

    Votes resetVotes; // Vote block with all votes false
    VoteResults passVotes(congress.getSize(), resetVotes);
    VoteResults voted(congress.getSize(), resetVotes);

    int rollCallNo = 1;
    bool haveFileToProcess = false;
    int failureCount = 0;
    bool haveVotes = false;
    int successCount = 0;
    while (failureCount < FAILURES_FOR_QUIT) {
        /* If this roll call is the start of a new block, and the previous one
            read any votes, process it
            SUBTLE NOTE: Don't bother checking for the first block, since no votes
            have been read! */
        int rollBlockOffset = (rollCallNo - 1) % ROLL_CALL_BLOCK_SIZE;
        if ((rollBlockOffset == 0) && haveFileToProcess) {
            convVoteResultToDiff(passVotes, voted, results);
            // Clear the block for next pass
            passVotes.assign(congress.getSize(), resetVotes);
            voted.assign(congress.getSize(), resetVotes);
            haveFileToProcess = false;
        }
        rollCall.open(year, rollCallNo);
        if (!rollCall.haveVotes())
            // Not finding any votes implies a bad file
            failureCount++;
        else {
            // Found a good one, so clear failure count
            failureCount = 0;
            haveFileToProcess = true;
            haveVotes = false; // None read yet
            billCount++;
            RollCall::VoteRecord newVote;
            while (rollCall.haveVotes()) {
                newVote = rollCall.getNextVote();
                unsigned short congressIndex = congress.getIndexNo(newVote.first);
                if (congressIndex == SHRT_MAX) {
                    // Not found. people.xml configured incorrectly
                    cerr << "WARNING: people.xml corrupt. Congressperson reference number "
                        << newVote.first << " not defined but in rollcall " << rollCallNo
                        << " for year " << year << endl;
                }
                else {
                    if (newVote.second)
                        passVotes.at(congressIndex).set(rollBlockOffset);
                    voted.at(congressIndex).set(rollBlockOffset);
                    haveVotes = true;
                } // Congressperson valid
            } // Votes to process
            // If read votes out of the file, increment the success count
            /* TRICKY NOTE: Success plus failure may be less than the total files processed.
                Files that contain no processed voted don't count in either category */
            if (haveVotes)
                successCount++;
        } // Roll call file is valid
        rollCallNo++;
    } // While files to process and under error limi

    // Process any unfinished block here
    if (haveFileToProcess)
        convVoteResultToDiff(passVotes, voted, results);

    // Processing zero votes successfully for a given year indicates corrupt data
    if (!successCount) {
        // Major problem, needed data is missing
        stringstream errorText;
        errorText << "Required roll call data files for year " << year << " missing";
        cerr << errorText.str() << endl;
        throw ios_base::failure(errorText.str());
    }
}

void VoteFactory::getVoteMatrix(VoteDiffMatrix& results, const CongressData& congress,
                                short firstYear, short lastYear)
{
    // Insure previous results do not carry over
    vector<short> tempResult(congress.getSize(), 0);
    results.assign(congress.getSize(), tempResult);

    /* HACK: Ideally, this routine will use system calls to find the last roll call
        file, and from that know the upper limits of votes. Instead, it assumes that
        vote files are resonably consecutive, so it can quit after a certain
        number of consecutive failures. Setting the failure number higher makes this
        routine more robust at the cost of less performance. Remember that roll
        call votes that are not up or down count as failures! */
    // SEMI-HACK: Handle this common problem
    if (lastYear < firstYear) {
        short temp = lastYear;
        lastYear = firstYear;
        firstYear = temp;
    }

    short billCount = 0;
    short year;
    for (year = firstYear; year <= lastYear; year++)
        getVoteDiff(results, billCount, congress, year);

    // Normalize the vote differences on a scale of 1 to 1000
    unsigned short rowIndex, columnIndex;
    for (rowIndex = 0; rowIndex < results.size(); rowIndex++)
        for (columnIndex = 0; columnIndex < results.size(); columnIndex++) {
            int tempResult = (int)results.at(rowIndex).at(columnIndex); // Prevent overflow
            tempResult = (tempResult * 1000) / (int)billCount;
            results.at(rowIndex).at(columnIndex) = (short)tempResult;
        }
}

/* The final layout is based on the similarities of votes. Pairs of groups with large numbers of different
    votes will have little efffect on the final layout. Filter them out to reduce compute */
void VoteFactory::filterLargeMismatch(VoteDiffMatrix& results, short threshold)
{
    unsigned short index1, index2;
    for (index1 = 0; index1 < results.size(); index1++)
        for (index2 = 0; index2 < results[index1].size(); index2++)
            if (results[index1][index2] > threshold)
                results[index1][index2] = -1;
}

// Debug method to print out the vote resuls matrix
void VoteFactory::debugOutputVoteMatrix(const VoteDiffMatrix& results)
{
    /* Outputing a large matrix has a big problem with line wrap. The
        algorithm used here is to break the matrix into pieces
        by columns, and output each piece seperately */
    /* Find number of columns per screen. Vote count is in the thousands, plue
        one space
        HACK/WORKAROUND: Many terminals wrap the line when it is equal to the line
        width, rather than just over it. Subtract one when finding blocks to avoid
        this problem */
    unsigned short columnsPerBlock = (SCREEN_WIDTH - 1) / 5;
    // Need one column for the row numbers
    columnsPerBlock--;

    unsigned short blockCount = results.size() / columnsPerBlock;
    if (results.size() % columnsPerBlock != 0)
        blockCount++; // One extra for the partial set at the end

    // Set up formatted numeric output: width 4 right justified
    cerr << std::setw(4);

    unsigned short blockIndex = 0;
    for (blockIndex = 0; blockIndex < blockCount; blockIndex++) {
        unsigned short startColumn = blockIndex * columnsPerBlock;
        unsigned short endColumn = startColumn + columnsPerBlock;
        if (endColumn > results.size())
            endColumn = results.size();

        unsigned short rowIndex, columnIndex;
       // Output column header
        cerr << endl << endl << "     ";
        for (columnIndex = startColumn; columnIndex < endColumn; columnIndex++)
            cerr << std::setw(4) << columnIndex << " ";
        cerr << endl;

        for (rowIndex = 0; rowIndex < results.size(); rowIndex++) {
            cerr << std::setw(4) << rowIndex << " ";
            for (columnIndex = startColumn; columnIndex < endColumn; columnIndex++)
                cerr << std::setw(4) << results.at(rowIndex).at(columnIndex) << " ";
            cerr << endl;
        }  // For every row
    } // Block Count
}
