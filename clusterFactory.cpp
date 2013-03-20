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
/* This file defines classes to clump Congresspeople with the most similiar
    voting records into clusters. Clustering reduces the overall number of
    graph points, which makes it easier to comprehend. A relatively low
    number of points is also needed to compute their positions within a
    resonable amount of time (force layout algorithms are non-polynomial) */
#include<iostream>
#include<vector>
#include<algorithm>
#include<iomanip>
#include"congressData.h"
#include"voteFactory.h" // Defines VoteDiffMatrix
#include"regionMapper.h"
#include"clusterFactory.h"

#define SCREEN_WIDTH 80

using std::cerr;
using std::endl;
using std::vector;
using std::multimap;
using std::make_pair;

// Ignore difference above this limit because their effect on the graph is too weak
short ClusterFactory::meaningfulDifferenceLimit = 600;

// Constructor, based on the distances between the initial set of clusters
GroupDistanceMap::GroupDistanceMap(const VoteDiffMatrix& distances)
{
    /* Create a ragged array from the distances matrix. For every element
        insert it in the multimap, and insert the resulting iterator into
        the array.

        SEMI-HACK: This routine is very sensitive to inconsistencies in the
        distance matrix. In particular, it assumes that it is square, and
        reflective along the diagonal (so only half has to be read). This
        routine should really check. Since the matrix was generated internally,
        it just writes out warnings and inserts zeros for missing elements */
    vector<DistancePtr> newRow;
    unsigned short clusterCount = distances.size();
    if (clusterCount < 2) {
        cerr << "Cluster merge failed, initial distance data is empty" << endl;
        return; // Leave class empty, nothing to do!
    }
    unsigned short clusterIndex1, clusterIndex2, rowSize;
    // First row is empty, will never be referenced
    _distanceByCluster.push_back(newRow);
    for (clusterIndex1 = 1; clusterIndex1 < clusterCount; clusterIndex1++) {
        _distanceByCluster.push_back(newRow);

        // Find number of elements to process
        rowSize = distances.at(clusterIndex1).size();
        if (rowSize > clusterIndex1)
            rowSize = clusterIndex1;

        for (clusterIndex2 = 0; clusterIndex2 < rowSize; clusterIndex2++) {
            // NOTE CAREFULLY: The swap of the indexes on insert, which makes the lower one appear first
            DistancePtr sortedPtr = _sortedDistances.insert(make_pair(distances.at(clusterIndex1).at(clusterIndex2),
                                                                      make_pair(clusterIndex2, clusterIndex1)));
            _distanceByCluster.at(clusterIndex1).push_back(sortedPtr);
        } // For columns of given row
        // Fill in missing values with zeroes
        if (rowSize < clusterIndex1) {
            cerr << "Cluster distance setup warning: Row " << clusterIndex1 << " not enough columns" << endl;
            for (clusterIndex2 = rowSize; clusterIndex2 < clusterIndex1; clusterIndex2++)
                _distanceByCluster.at(clusterIndex1).push_back(_sortedDistances.end());
        } // Not enougn column values for all clusters
    } // For each row
}

/* Updates a cluster distance to a new value. Deletes the map iterator,
    inserts the new value, and stores the new iterator. If no existing
    distance is defined, nothing happens. If the distance shrinks
    (indicating an error in the algorithm) a warning is issued. */
void GroupDistanceMap::updateDistance(unsigned short cluster1, unsigned short cluster2,
                                      short distance)
{
    // NOTE: Variables passed by value, so can butcher them at will
    sortClusterRequest(cluster1, cluster2);
    if (haveDistanceData(cluster1, cluster2)) {
        /* OPTIMIZATION NOTE: Its tempting to set a reference to the wanted
            ragged array entry, to avoid looking it up four times. Don't bother, the
            vector at() method is incredibly optimized, and inline */
        if (distance < _distanceByCluster.at(cluster1).at(cluster2)->first)
            cerr << "WARNING: Distance reduced to " << distance << " for cluster pair ("
                << cluster1 << "," << cluster2 << ")" << endl;
        _sortedDistances.erase(_distanceByCluster.at(cluster1).at(cluster2));
        _distanceByCluster.at(cluster1).at(cluster2) =
            _sortedDistances.insert(make_pair(distance, make_pair(cluster2, cluster1)));
    } // Distance defined for the cluster pair
}

/* Given the map of vote differences indexed by congresspersons and the level
    of vote differences considered to be noise, returns the groups of congressmen.
    The last optional parameter is a lower limit on the number of groups, used to
    prevent returning a few huge blobs if the noise threshold is chosen badly.
    Internally, the method implements the classic complete linkage grouping algorithm */
void ClusterFactory::formClusters(const VoteDiffMatrix& congressVotes,
                                  CongressGroupVector& congressMatchGroups,
                                  const CongressData& congressData, // Needed for trace
                                  short noiseThreshold, short minGroups,
                                  bool traceOutput)
{
    // Convert congresspeople into groups of one each
    congressMatchGroups.clear();
    // SANITY CHECK
    if (congressVotes.empty()) {
        cerr << "Grouping failed, matrix of vote differences is empty!" << endl;
        return;
    }
    unsigned short clusterIndex;
    for (clusterIndex = 0; clusterIndex < congressVotes.size(); clusterIndex++) {
        CongressGroup newGroup;
        newGroup.insert(clusterIndex);
        congressMatchGroups.push_back(newGroup);
    }
    GroupDistanceMap distances(congressVotes);

    if (traceOutput) {
        cerr << "Initial group distances" << endl;
        debugOutputDistances(distances);
    }

    if (minGroups < 1)
        minGroups = 1; // Ensure merge loop terminates

    unsigned short clusterCount = distances.getClusterNoLimit();
    GroupDistanceMap::ClusterPair nextMerge = distances.getShortestDistanceCluster();
    short mergeDistance = distances.getDistance(nextMerge);
    while ((clusterCount > minGroups) && (mergeDistance <=  noiseThreshold)) {
        // Merge actual cluster contents
        unsigned short newCluster = findMergeClusterIndex(nextMerge.first, nextMerge.second);
        if (nextMerge.first == newCluster)
            mergeClusters(congressMatchGroups, nextMerge.second, newCluster);
        else
            mergeClusters(congressMatchGroups, nextMerge.first, newCluster);
        clusterCount--;
        // Merge the distance data, and find next cluster
        mergeClusters(distances, nextMerge.first, nextMerge.second);
        if (traceOutput) {
            cerr << "Merge cluster " << nextMerge.first << " and " << nextMerge.second << endl;
            cerr << "New distances:" << endl;
            debugOutputDistances(distances);
        }

        nextMerge = distances.getShortestDistanceCluster();
        mergeDistance = distances.getDistance(nextMerge);
    } // while clusters to merge

    /* Since sets are removed as they are merged, the final vector may have holes in it
        Go through the list and remove them. Since the final order has no meaning, search
        from both ends to limit the number of swaps */
    unsigned short fillIndex = 0;
    while (fillIndex < congressMatchGroups.size()) {
         // Eject all empty entries at the end of the vector
        while ((fillIndex < congressMatchGroups.size()) &&
               congressMatchGroups.back().empty())
            congressMatchGroups.pop_back();
       // Search forward for the first empty slot
        while ((fillIndex < congressMatchGroups.size()) && (!congressMatchGroups.at(fillIndex).empty()))
            fillIndex++;
        if (fillIndex < congressMatchGroups.size()) // Found empty entry, swap to end
            congressMatchGroups.at(fillIndex).swap(congressMatchGroups.back());
    } // While not all entries in vector have been processed
    if (traceOutput) {
        cerr << "Final groups:" << endl;
        debugOutputClusterList(congressMatchGroups, congressData);
    }
}

// Helper method to calculate distance data for a newly merged cluster
void ClusterFactory::mergeClusters(GroupDistanceMap& data, unsigned short cluster1,
                                    unsigned short cluster2)
{
    // First, erase the distance between the two merged clusters
    data.eraseDistance(cluster1, cluster2);

    /* For every cluster in the list NOT being merged, the distance to the new
        cluster is the higher of the two entries */
    unsigned short newCluster = findMergeClusterIndex(cluster1, cluster2);
    unsigned short eraseCluster = cluster2;
    if (eraseCluster == newCluster) // Picked the wrong one!
        eraseCluster = cluster1;

    unsigned short clusterIndex;
    for (clusterIndex = 0; clusterIndex < data.getClusterNoLimit(); clusterIndex++) {
        if ((clusterIndex != cluster1) && (clusterIndex != cluster2)) {
            /* Since previous clusters may have been merged, the list will have
                empty entries. Finding only ONE empty entry for a cluster, however
                indicates data corruption. Issue a warning and erase it */
            bool haveEntry1 = data.haveDistanceData(clusterIndex, cluster1);
            bool haveEntry2 = data.haveDistanceData(clusterIndex, cluster2);

            if (!haveEntry1) {
                if (haveEntry2) {
                    cerr << "Clustering error. Distance exists for (" << clusterIndex
                        << "," << cluster2 << ") but not (" << clusterIndex << ","
                        << cluster1 << ")" << endl;
                    data.eraseDistance(clusterIndex, cluster2);
                }
            } // No data for first cluster to merge
            else if (!haveEntry2) {
                cerr << "Clustering error. Distance exists for (" << clusterIndex
                    << "," << cluster1 << ") but not (" << clusterIndex << ","
                    << cluster2 << ")" << endl;
                data.eraseDistance(clusterIndex, cluster1);
            } // No data for second cluster to merge
            else {
                if (data.getDistance(newCluster, clusterIndex) <
                        data.getDistance(eraseCluster, clusterIndex))
                    data.updateDistance(newCluster, clusterIndex,
                                        data.getDistance(eraseCluster, clusterIndex));
                data.eraseDistance(eraseCluster, clusterIndex);
            } // Have distance data for both clusters to be merged
        } // If the cluster being processed is NOT one of those being merged
    } // For every cluster in the data
}

void ClusterFactory::debugOutputDistances(const GroupDistanceMap& distances)
{
    /* Outputing a large matrix has a bit problem with line wrap. The
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

    /* The distances are stored in a ragged array. The number of rows goes from 1 to
        number clusters, and the number of columns from 0 to (rowNo - 1). The maximum
        number of columns is the number of clusters minus 1 */
    unsigned short maxNoColumns = distances.getClusterNoLimit() - 1;

    unsigned short blockCount = maxNoColumns / columnsPerBlock;
    if (maxNoColumns % columnsPerBlock != 0)
        blockCount++; // One extra for the partial set at the end

    // Set up formatted numeric output: width 4 right justified
    cerr << std::setw(4);

    unsigned short blockIndex = 0;
    for (blockIndex = 0; blockIndex < blockCount; blockIndex++) {
        unsigned short startColumn = blockIndex * columnsPerBlock;
        unsigned short endColumn = startColumn + columnsPerBlock;
        if (endColumn > maxNoColumns)
            endColumn = maxNoColumns;

        unsigned short rowIndex, columnIndex;
       // Output column header
        cerr << endl << endl << "     ";
        for (columnIndex = startColumn; columnIndex < endColumn; columnIndex++)
            cerr << std::setw(4) << columnIndex << " ";
        cerr << endl;

        // Skip first row, since nothing in it
        for (rowIndex = 1; rowIndex < distances.getClusterNoLimit(); rowIndex++) {
            cerr << std::setw(4) << rowIndex << " ";
            unsigned short endColumnForRow = endColumn;
            // Handle effect of ragged array
            if (endColumnForRow > rowIndex)
                endColumnForRow = rowIndex;
            if (startColumn < endColumnForRow)
                for (columnIndex = startColumn; columnIndex < endColumnForRow; columnIndex++)
                    cerr << std::setw(4) << distances.getDistance(rowIndex, columnIndex) << " ";
            cerr << endl;
        }  // For every row
    } // Block Count
}

// Outputs the contents of the cluster list
void ClusterFactory::debugOutputClusterList(const CongressGroupVector& congressMatchGroups,
                                            const CongressData& congressData)
{
    unsigned short index;
    for (index = 0; index < congressMatchGroups.size(); index++) {
        cerr << index << ": ";
        CongressGroup::iterator temp;
        for (temp = congressMatchGroups.at(index).begin();
            temp != congressMatchGroups.at(index).end(); temp++) {
                CongressData::CongressPerson data = congressData.getData(*temp);
                cerr << *temp << "[" << data._party[0] << ":" << data._state << "] ";
            }
        cerr << endl;
    }
}

/* Finds the average vote difference between every pair of clusters. The
    difference map is indexed by the order clusters appear in the supplied
    group list. This method assumes that the noise limit for the clustering
    is high enough that rounding from the average calculation will not
    materially affect the results. */
void ClusterFactory::getClusterDistanceMap(const VoteDiffMatrix& congressVoteMap,
                                            const CongressGroupVector& congressGroupList,
                                            VoteDiffMatrix& groupVotesMap)
{
    if (!congressGroupList.size()) { // No groups!
        cerr << "Calculation of cluster distances failed, no clusters in list" << endl;
        return;
    }

    // Insure previous results do not carry over
    vector<short> tempResult(congressGroupList.size(), 0);
    groupVotesMap.assign(congressGroupList.size(), tempResult);

    // If only one cluster, can quit now
    if (congressGroupList.size() == 1)
        return;

    /* Every cluster combination appears in the matrix twice. To avoid a double
        calculation, find the value once and insert it in both spots */
    unsigned short index1, index2;
    for (index1 = 0; index1 < (congressGroupList.size() - 1); index1++)
        for (index2 = index1 + 1; index2 < congressGroupList.size(); index2++) {
            groupVotesMap.at(index1).at(index2) = findClusterDistance(congressVoteMap,
                                                                        congressGroupList.at(index1),
                                                                        congressGroupList.at(index2));
            groupVotesMap.at(index2).at(index1) = groupVotesMap.at(index1).at(index2);
        }
    return;
}

// Find the average distance between two clusters
short ClusterFactory::findClusterDistance(const VoteDiffMatrix& congressVoteMap,
                                        const CongressGroup& cluster1,
                                        const CongressGroup& cluster2)
{
    CongressGroup::const_iterator index1, index2;
    int totalVoteDiff = 0; // Sum values in int to prevent overflow
    int voteCount = 0;

    for (index1 = cluster1.begin(); index1 != cluster1.end(); index1++)
        for (index2 = cluster2.begin(); index2 != cluster2.end(); index2++) {
            totalVoteDiff += (int)congressVoteMap.at(*index1).at(*index2);
            voteCount++;
        }
    /* This division will have rounding error. The code assumes the number of
        congresspeople per cluster is high enough that it will be smaller than
        the rounding from the normalization of the vote counts */
    return (short)(totalVoteDiff / voteCount);
}

// Given groups of congress persons, summarizes data about their members
void ClusterFactory::getClusterCongressData(const CongressGroupVector& congressGroupList,
                                            const CongressData& congressData,
                                            const RegionMapper& regions,
                                            CongressGroupDataList& congressGroupData)
{
    congressGroupData.clear();
    congressGroupData.reserve(congressGroupList.size());
    CongressGroupVector::const_iterator groupIndex;
    for (groupIndex = congressGroupList.begin(); groupIndex != congressGroupList.end(); groupIndex++) {
        CongressGroupData newData;
        newData._group = *groupIndex;
        newData._parties.assign(3, 0);
        newData._regions.assign(regions.getRegionCount() + 1, 0);
        CongressGroup::const_iterator memberIndex;

        // Get data for each congressperson in group and updated counts accordingly
        for (memberIndex = groupIndex->begin(); memberIndex != groupIndex->end(); memberIndex++) {
            CongressData::CongressPerson data = congressData.getData(*memberIndex);
            if (data._party[0] == 'D')
                newData._parties[0]++;
            else if (data._party[0] == 'R')
                newData._parties[1]++;
            else // Third party
                newData._parties[2]++;
            newData._regions[regions.getRegion(data._state)]++;
        }
        congressGroupData.push_back(newData);
    }
}

// Outputs the contents of the cluster data list
void ClusterFactory::debugOutputClusterList(const CongressGroupDataList& congressGroupData)
{
    unsigned short index;
    for (index = 0; index < congressGroupData.size(); index++) {
        cerr << index << ": ";
        CongressGroup::iterator temp;
        for (temp = congressGroupData[index]._group.begin();
            temp != congressGroupData[index]._group.end(); temp++)
                cerr << *temp << " ";
        if (congressGroupData[index]._parties[0] > 0)
            cerr << "D:" << congressGroupData[index]._parties[0] << " ";
        if (congressGroupData[index]._parties[1] > 0)
            cerr << "R:" << congressGroupData[index]._parties[1] << " ";
        if (congressGroupData[index]._parties[2] > 0)
            cerr << "I:" << congressGroupData[index]._parties[2] << " ";
        cerr << "Regions: ";
        unsigned short index2;
        for (index2 = 0; index2 < congressGroupData[index]._regions.size(); index2++)
            if (congressGroupData[index]._regions[index2] > 0)
                cerr << index2 << ": " << congressGroupData[index]._regions[index2] << " ";
        cerr << endl;
    }
}


