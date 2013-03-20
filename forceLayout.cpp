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

/* This file holds code for laying out the groups with proximity based on similiarity.
    It uses a classic force layout algorithm to do so. Every pair of points has an
    attraction and a repulsion force. The attraction is based on their distance and
    the similarity of their votes; the repulsion is based on the inverse of the
    square of the distance.
    Force layout algorithms are non-polynomial and usually don't have an analytic
    solution. The classic approach, used here, is an iterative solver. For each
    iteration, add up all the forces on each point, and then move it by some amount
    proportional to the total. The number of iterations and the propotion are
    constants in the class */
#include<algorithm>
#include<vector>
#include<iostream>
#include<cmath>
#include<exception>
#include"congressData.h"
#include"voteFactory.h" // Needed for clusterFactory.h
#include"regionMapper.h" // Needed for clussterFactory.h
#include"clusterFactory.h"
#include"forceLayout.h"
#include"displayGroup.h" // Needed for display size methods

using std::vector;

using std::cerr;
using std::endl;
using std::exception;

// Creates a point at the given position
Coordinate::Coordinate(float x, float y)
{
    _x = x;
    _y = y;
    _point = 1;
}

// Creates s zero vector
/* NOTE: Chosen because anything can be added to or subtracted
    from a vector, but not a point */
Coordinate::Coordinate(void)
{
    _x = 0.0;
    _y = 0.0;
    _point = 0;
}

// Adds a coordinate to this one. If one is a point, the point is moved
void Coordinate::operator+=(const Coordinate& other)
{
    if (isPoint() && other.isPoint())
        throw exception();
    _x += other._x;
    _y += other._y;
    /* Derive the new point value from the existing values, one of the
        advantages of homogonous coordinates */
    _point += other._point;
}

// Number of iterations to find a solution
short ForceLayout::_iterationLimit = 50;

// Initial movement amount for a given force
float ForceLayout::_forceMoveRatio = 0.1;

// Strenth of attraction force to repulsion for a given distance
float ForceLayout::_attractVsRepulse = 1.0;

// Amount of overlap allowed for spots in the graph
float ForceLayout::_overlapAllowed = 0.0;

// Find the initial spot for the next group to place
inline Coordinate ForceLayout::findInitialCoordinate(short groupPerSide, float distPerGroup, short counter)
{
    return Coordinate((distPerGroup / 2) + (distPerGroup * (counter % groupPerSide)),
                     (distPerGroup / 2) + (distPerGroup * (counter / groupPerSide)));
}

// Find the force that a given point exerts on the test point
Coordinate ForceLayout::findForce(const CongressGroupData& testData,
                                  const CongressGroupData& otherData,
                                  short voteDifference,
                                  const Coordinate& testLoc, const Coordinate& otherLoc)
{
    // The calculation depends on the distance, and also need the direction
    Coordinate travel = otherLoc - testLoc;
    float distance = travel.getDistance();

    /* To get a usable graph, points should not move such that they substantially
        overlap. To get this effect while still keeping the force calculations constant,
        shrink the distance by the radii of the two groups. This makes the force go to zero
        when they touch */
    float netDistance = distance - (DisplayGroup::getGroupRadius(testData) +
                                    DisplayGroup::getGroupRadius(otherData) -
                                    _overlapAllowed);

    /* TRICKY NOTE: What happens if two groups occupy the exact same point? It means
        they touch. Need a repulsive force here to drive them apart, which balances the
        forces from other particles still attracing them */
    if (netDistance <= 0.0) {
        travel *= (-10.0 / distance);
        return travel;
    }
    // Repulsion, clasic inverse square law
    float repulsion = 1.0 / sqrtf(netDistance);

    /* Attraction, strength times the log of the distance. Strictly speaking, this
        should be strength times distance, but that causes outsized values when points
        are far apart.

        Strength is based on vote similarity. What is passed in is the
        difference, so need to subtract. Very large vote differences have been filtered
        out of the results by this point; to get a meaningful graph, need to scale the
        attraction based on the upper limit of the remainder */
    /* TRICKY NOTE: The log of a value less than one is negative, which screws up this
        algorithm. Set the attraction values for such distances as zero */
    float attraction = 0.0;
    if ((voteDifference >= 0) && (netDistance > 1.0))
        attraction = (((float)(ClusterFactory::meaningfulDifferenceLimit - voteDifference)) /
                       ClusterFactory::meaningfulDifferenceLimit) * logf(netDistance);

    // Combine to get the amount of the force between two congresspeople
    float force = (attraction * _attractVsRepulse) - repulsion;

    /* Finallly, multiply by the normalized direction vector to get the final force
        OPTIMIZATION: The normalized vector is the travel vector divided by its
        size, which is the distance. Since its already calculated, include it in
        the calculation instead of finding it again by calling a normalization
        method */
    travel *= (force / distance);
    return travel;
}

// Find the layout of congresspeople using the classic force based algorithm
void ForceLayout::makeLayout(const VoteDiffMatrix& votes, const CongressGroupDataList& congressGroupData,
                             LayoutVector& congressPositions)
{
    congressPositions.clear();
    Coordinate temp(0, 0);
    congressPositions.assign(congressGroupData.size(), temp);

    // Intial layout is a square centered in the middle of the drawing area
    /* TRICKY NOTE: sqrt on a short normally rounds down. In this case, need to
        round up. Calcualte as a float and truncate */
    short groupPerSide = (short)(sqrtf((float)congressGroupData.size()) + 0.999999);
    float distPerGroup = 640.0 / (float)groupPerSide;

    /* The House of Representatives is traditionally very partisan. Use this fact to create the
        initial layout: Democrats at the top, then mixed or other groups, then Republicans. A top to bottom
        sort is used instead of left to right to avoid giving the impression the layout is based
        on anything other than vote similarity */
    short positionCounter = 0;
    vector<short> democrat, mixed;
    vector<short>::iterator groupIndex;

    unsigned short index;
    for (index = 0; index < congressGroupData.size(); index++) {
        if (congressGroupData[index]._parties[0] == (short)congressGroupData[index]._group.size())
            // All democrat
           democrat.push_back(index);
        else if (congressGroupData[index]._parties[1] == (short)congressGroupData[index]._group.size()) {
            // All republican
            congressPositions[index] = findInitialCoordinate(groupPerSide, distPerGroup, positionCounter);
            positionCounter++;
        }
        else
            // Mixed or third party
            mixed.push_back(index);
    } // For loop through congress data

    // Insert all mixed groups
    for (groupIndex = mixed.begin(); groupIndex != mixed.end(); groupIndex++) {
        congressPositions[*groupIndex] = findInitialCoordinate(groupPerSide, distPerGroup, positionCounter);
        positionCounter++;
    }

    // Insert all democrats
    for (groupIndex = democrat.begin(); groupIndex != democrat.end(); groupIndex++) {
        congressPositions[*groupIndex] = findInitialCoordinate(groupPerSide, distPerGroup, positionCounter);
        positionCounter++;
    }

    /* A force layout algorithmm creates a force between every pair of points. Points repel
       each other based on how far apart they are, with repulsion increasing with the number
       of congresspeople in each point and decreasing as some function of distance. Points
       also attract each other based on their vote similarity, incresing with the number of
       congresspeople in each point, their similarity, and some function of their distance
       apart. Note that if the two points have no vote similarity or the matrix is marked to
       ignroe it, there is no attraction force. The sum of forces on the point from all other
       points becomes the overall force.
           The goal is to find a layout where the sum of the forces on all points is zero. This
        system is incredibly non-linear and complex so an analytic solution is not possible.
        Instead, these algorithms use any number of iterative solvers for energy state systems.
        The simplest is to iteratively move each point based on the forces on it and
        recalculate,over and over. The amount to move for a given force decreases over time to
        ensure the solution coverges. This method is straightforward to implement but suffers
        from the problem of finding local minima at the expense of better values elsewhere.
        For graph layout problems, this is considered an acceptable tradeoff */
    LayoutVector forces(congressPositions.size(), Coordinate());

    unsigned short iteration, index1, index2;
    for (iteration = 0; iteration < _iterationLimit; iteration++) {
        // Clear out force data from last iteration
        forces.assign(congressPositions.size(), Coordinate());
        for (index1 = 0; index1 < congressPositions.size(); index1++) {
            // Find sum of forces on this point
            for (index2 = 0; index2 < congressPositions.size(); index2++)
                if (index1 != index2) {
                    // Vote difference array is ragged
                    short voteDifference;
                    if (index1 < index2)
                        voteDifference = votes.at(index2).at(index1);
                    else
                        voteDifference = votes.at(index1).at(index2);
                    forces[index1] += findForce(congressGroupData[index1],
                                                congressGroupData[index2],
                                                voteDifference, congressPositions[index1],
                                                congressPositions[index2]);
                } // For every group not this group

            /* Convert the force into the amount to move the point. The distance
                is the force times the amount per force unit times a linearly
                declining term based on the iteration */
            forces[index1] *= _forceMoveRatio * (((float)(_iterationLimit - iteration)) / _iterationLimit);
        } // For every group
        // Now move the points.
        for (index1 = 0; index1 < congressPositions.size(); index1++) {
            congressPositions[index1] += forces[index1];
            // Do not allow people to slide off the display area
            if (congressPositions[index1].getX() < 0)
                congressPositions[index1] = Coordinate(0, congressPositions[index1].getY());
            else if (congressPositions[index1].getX() > 640)
                congressPositions[index1] = Coordinate(640, congressPositions[index1].getY());
            if (congressPositions[index1].getY() < 0)
                congressPositions[index1] = Coordinate(congressPositions[index1].getX(), 0);
            else if (congressPositions[index1].getY() > 640)
                congressPositions[index1] = Coordinate(congressPositions[index1].getX(), 640);
        }
    } // For each iteration
 }
