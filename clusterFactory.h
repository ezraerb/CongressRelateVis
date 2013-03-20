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
/* The classes in this file take the map of vote differences and group
    Congresspeople based on vote similarity. It uses a complete link
    clustering algorithm, so groups are formed such that the maximum number
    of vote differences between members of the group is under some limit.
    The class exists for a number of reasons:
    1.	No set of Congresspeople, no matter how partisan, vote the exact same
        way on all votes. The absences alone see to that. This creates noise
        in the results. The clustering declares that vote differences below
        some threshold are not meaningful and should be ignored. The
        groups show which Congressmen should be treated by the final display
        has having identical votes
    2.	Displaying a large number of Congressmen on the screen at once leads to a
        messy, hard to read result. Removing the least meaningful differences
        leads to a more comprehensible graph.
    3.	Force based layout algorithms are NP-complete, which means that
        computation time scales exponentially with the number of nodes. Reducing
        their number is essential to making the problem tractable, and clustering
        is the best method available for doing that.
    */

#include<map>
#include<set>

using std::multimap;
using std::set;

/* NOTE: The following are defined in headers deliberately not included,
    because they are widely used and the source should include the headers anyway */
using std::swap;
using std::pair;
using std::vector;

typedef set<unsigned short> CongressGroup;
typedef vector<CongressGroup> CongressGroupVector;

struct CongressGroupData
{
    CongressGroup _group; // Members of this group
    vector<short> _parties; // Count of members in each political party: D, R, other
    vector<short> _regions; // Count of members in each geographic region

    // Returns the total number of Congresspeople in a group
    unsigned short getCount(void) const;
};

typedef vector<CongressGroupData> CongressGroupDataList;

class GroupDistanceMap
{
    /* This class manages the data of distances between clusters. The data
        management is complex enough that it needs to be encapsulated in a
        separate class. The basic design issue is that the data is accessed
        in two very different ways: find the distance given a pair of clusters,
        and find the lowest distance within the set. The lowest distance
        changes with every cluster merge because the smaller distances of the
        old clusters to the remainder are always eliminated. The optimal data
        structures for each purpose are very different, so this class keeps
        two separate data representations and keeps them in sync. The risk of
        desynchronization should be obvious, which is why they are encapsulated
        in this class.

        For the purpose of getting distance given two clusters, it uses a ragged
        array, a two dimensional array (stored as vectors) where the length of
        each row varies. In particular, each row only records distances for
        columns less than the row number. The ragged array means that only one
        value update is needed for any pair of clusters.

        For the purpose of finding the lowest distance, it keeps a sorted list
        of cluster pairs indexed by distance, using a multi-map. Multimaps have
        the nice property that iterators are stable unless the entry itself is
        changed, so storing the iterators in the ragged array allows direct access
        to the entry in the sorted distances multimap. This way, distance data
        can be directly looked up without storing it twice.

        Efficiency overview: Every merge will require updating N distance
        entries, to account for the newly formed cluster relative to the remainder.
        Deletion of entries can be done in constant time due to the stored iterator.
        Updating an existing value will be done half the time on average, giving N/2
        map updates. The update is done by binary search, so the time required is the log
        of the number of entries. The size of the sorted list is N^2, so the cost of a
        merge is 2logN. Combine this to get a final cost of O(NlogN), an amazing coincidence.

        The first item in the multi-map is the next pair to merge, so finding the next
        pair to merge is constant time. The alternative is to search the multi-map
        every time. Updates are done in constant time, but every search is now O(N^2),
        because the ragged array is not sorted and must be linearly searched. Finally,
        if the merging process is reported until there is only one group left, N-1
        merges must be performed, giving an overall performance of O(NNlogN). This
        matches the current theoretical minimum for complete link clustering algorithms.
        */

public:
    typedef pair<unsigned short, unsigned short> ClusterPair;

    /* Initializes the class from a map of distances between clusters. The  input
        matches the distances between Congresspeople by deliberate coincidence,
        since every one is initially their own cluster */
    explicit GroupDistanceMap(const VoteDiffMatrix& distances);

    // Returns the highest valid cluster index
    unsigned short getClusterNoLimit(void) const;

    // Returns true if distance data exists for a given pair of clusters.
    bool haveDistanceData(unsigned short cluster1, unsigned short cluster2) const;
    bool haveDistanceData(const ClusterPair& clusters) const;

   /* Gets the distance between a pair of clusters. If either is no longer
        defined, the distance is zero. */
    short getDistance(unsigned short cluster1, unsigned short cluster2) const;
    short getDistance(const ClusterPair& clusters) const;

    /* Erases a cluster distance from the map. Also deletes the iterator.
        If the distance is not defined, nothing happens */
    void eraseDistance(unsigned short cluster1, unsigned short cluster2);

    /* Updates a cluster distance to a new value. Deletes the map iterator,
        inserts the new value, and stores the new iterator. If no existing
        distance is defined, nothing happens. If the distance shrinks
        (indicating an error in the algorithm) a warning is issued. */
    void updateDistance(unsigned short cluster1, unsigned short cluster2,
                        short distance);

    /* Returns the pair of clusters whose merger will create the shortest
        distance within any possible cluster.*/
    ClusterPair getShortestDistanceCluster(void);

private:
    // Prohibit copying, because it will screw up iterator data
    GroupDistanceMap(const GroupDistanceMap& other);
    GroupDistanceMap operator=(const GroupDistanceMap& other);

    typedef multimap<short, ClusterPair> DistanceMap;
    typedef DistanceMap::iterator DistancePtr;

    DistanceMap _sortedDistances;
    vector<vector<DistancePtr> > _distanceByCluster;

    // Sorts the requested cluster pair into order for the lookup
    void sortClusterRequest(unsigned short& cluster1, unsigned short& cluster2) const;

};

// Returns the total number of Congresspeople in a group
inline unsigned short CongressGroupData::getCount(void) const
{
    return _group.size();
}

// Returns the highest valid cluster index
inline unsigned short GroupDistanceMap::getClusterNoLimit(void) const
{
    return _distanceByCluster.size();
}

// Sorts the requested cluster pair into order for the lookup
inline void GroupDistanceMap::sortClusterRequest(unsigned short& cluster1, unsigned short& cluster2) const
{
    /* The ragged array is set up so the row is always the higher cluster number.
        This routine means that callers don't need to know about this, or care */
    if (cluster1 < cluster2)
        swap(cluster1, cluster2);
}

/* Returns the pair of clusters whose merger will create the shortest
    distance within any possible cluster.*/
inline GroupDistanceMap::ClusterPair GroupDistanceMap::getShortestDistanceCluster(void)
{
    if (_sortedDistances.empty())
        return std::make_pair(0, 0);
    else
        return _sortedDistances.begin()->second;
}

// Returns true if distance data exists for a given pair of clusters.
inline bool GroupDistanceMap::haveDistanceData(unsigned short cluster1, unsigned short cluster2) const
{
    // NOTE: Variables passed by value, so can butcher at will
    sortClusterRequest(cluster1, cluster2);
    if ((cluster1 < getClusterNoLimit()) && (cluster1 > cluster2))
        // An entry still exists if it has an iterator into the sorted distance list
        return (_distanceByCluster.at(cluster1).at(cluster2) != _sortedDistances.end());
    else // Outside the ragged array limits, so clearly no data
        return false;
}

inline bool GroupDistanceMap::haveDistanceData(const ClusterPair& clusters) const
{
    return haveDistanceData(clusters.first, clusters.second);
}


/* Gets the distance between a pair of clusters. If either is no longer
    defined, the distance is zero. */
inline short GroupDistanceMap::getDistance(unsigned short cluster1, unsigned short cluster2) const
{
    // NOTE: Variables passed by value, so can butcher them at will
    sortClusterRequest(cluster1, cluster2);
    if (!haveDistanceData(cluster1, cluster2))
        return 0; // Outside the ragged array
    else
        // The at() calls return an iterator. Dereference it to get the pair, first value is distance
        return _distanceByCluster.at(cluster1).at(cluster2)->first;
}

inline short GroupDistanceMap::getDistance(const ClusterPair& clusters) const
{
    return getDistance(clusters.first, clusters.second);
}

/* Erases a cluster distance from the map. Also deletes the iterator.
    If the distance is not defined, nothing happens */
inline void GroupDistanceMap::eraseDistance(unsigned short cluster1, unsigned short cluster2)
{
    // NOTE: Variables passed by value, so can butcher them at will
    sortClusterRequest(cluster1, cluster2);
    if (haveDistanceData(cluster1, cluster2)) {
        /* OPTIMIZATION NOTE: Its tempting to set a reference to the wanted
            ragged array entry, to avoid looking it up twice. Don't bother, the
            vector at() method is incredibly optimized, and inline */
        _sortedDistances.erase(_distanceByCluster.at(cluster1).at(cluster2));
        // Clear array entry
        _distanceByCluster.at(cluster1).at(cluster2) = _sortedDistances.end();
    } // Distance defined for the cluster pair
}

// This class does the clustering, using the classic complete link algorithm
class ClusterFactory
{
public:
    static short meaningfulDifferenceLimit;

    /* Given the map of vote differences indexed by congresspersons and the level
        of vote differences considered to be noise, returns the groups of congressmen.
        The last optional parameter is a lower limit on the number of groups, used to
        prevent returning a few huge blobs if the noise threshold is chosen badly.
        Internally, the method implements the classic complete linkage grouping algorithm */
    static void formClusters(const VoteDiffMatrix& congressVotes,
                            CongressGroupVector& congressMatchGroups,
                            const CongressData& congressData, // Needed for trace
                            short noiseThreshold = 100, short minGroups = 0,
                            bool traceOutput = false);

    /* Finds the average vote difference between every pair of clusters. The
        difference map is indexed by the order clusters appear in the supplied
        group list. This method assumes that the noise limit for the clustering
        is high enough that rounding from the average calculation will not
        materially affect the results. */
    static void getClusterDistanceMap(const VoteDiffMatrix& congressVoteMap,
                                      const CongressGroupVector& congressGroupList,
                                      VoteDiffMatrix& groupVotesMap);

    // Given groups of congress persons, summarizes data about their members
    static void getClusterCongressData(const CongressGroupVector& congressGroupList,
                                       const CongressData& congressData,
                                       const RegionMapper& regions,
                                       CongressGroupDataList& congressGroupData);

    // Outputs the contents of the cluster list, with identification informmation
    static void debugOutputClusterList(const CongressGroupVector& congressMatchGroups,
                                       const CongressData& congressData);

    // Outputs the contents of the cluster data list
    static void debugOutputClusterList(const CongressGroupDataList& congressGroupData);

private:
    // Finds the distances between a newly merged cluster and all other clusters
    static void mergeClusters(CongressGroupVector& groups, unsigned short source,
                              unsigned short destination);

    // Finds the cluster index number for a newly merged cluster
    static unsigned short findMergeClusterIndex(unsigned short cluster1,
                                                unsigned short cluster2);

    // Merges the cluster contents within the list
    static void mergeClusters(GroupDistanceMap& data, unsigned short cluster1,
                              unsigned short cluster2);

    // Find the average distance between two clusters
    static short findClusterDistance(const VoteDiffMatrix& congressVoteMap,
                                    const CongressGroup& cluster1,
                                    const CongressGroup& cluster2);

    // Prints the distance output
    static void debugOutputDistances(const GroupDistanceMap& distances);

};

inline unsigned short ClusterFactory::findMergeClusterIndex(unsigned short cluster1,
                                                            unsigned short cluster2)
{
    if (cluster1 < cluster2)
        return cluster1;
    else
        return cluster2;
}

// Finds the distances between a newly merged cluster and all other clusters
inline void ClusterFactory::mergeClusters(CongressGroupVector& groups, unsigned short source,
                                            unsigned short destination)
{
    groups.at(destination).insert(groups.at(source).begin(),
                                  groups.at(source).end());
    groups.at(source).clear();
}

