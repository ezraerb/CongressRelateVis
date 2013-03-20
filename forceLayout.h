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
#include<cmath>

/* Not including STL headers here, because the file including this file
    should already have them included for other purposes */
using std::pair;
using std::vector;

// Homogonous 2D coordinate system. If third field is 1 it is a point. else a vector
class Coordinate
{
public:
    // Creates a point
    /* NOTE: Can't directly create a vector, because in this algorithm
        all of them are found as the difference between two points */
    Coordinate(float x, float y);

    // Default constructor, needed to use in containers. Creates a zero vector
    Coordinate(void);

    // Use default copy constructor and assignment operator

    // Adds a coordinate to this one. If one is a point, the point is moved
    void operator+=(const Coordinate& other);

    /* Subtracts a coordinate from this one and returns the result. If both
        are vectors, they are combined. If one is a point, the result is it
        moved to the vector, if both are points the result is the vector that
        connects them */
    Coordinate operator-(const Coordinate& other) const;

    // Multiplies the coordinate by a constant. Points are moved, vectors change size
    void operator*=(float value);

    /* Finds the distance. This is defined as the length for vectors, the distance
        from the origin for points */
    float getDistance() const;

    // Getters
    float getX() const;
    float getY() const;
    bool isPoint() const;

private:
    float _x;
    float _y;
    short _point;
};

inline float Coordinate::getX() const
{
        return _x;
}

inline float Coordinate::getY() const
{
        return _y;
}

inline bool Coordinate::isPoint() const
{
    return (_point > 0);
}

/* Subtracts a coordinate from this one and returns the result. If both
    are vectors, they are combined. If one is a point, the result is it
    moved to the vector, if both are points the result is the vector that
    connects them */
inline Coordinate Coordinate::operator-(const Coordinate& other) const
{
    Coordinate temp(_x - other._x, _y - other._y);
    /* Point value is calculated from the other two values, one of the reasons
        homogonous cooordinates work so well */
    temp._point = _point - other._point;
    if (temp._point < 0)
        temp._point = 1;
    return temp;
}

// Multiplies the coordinate by a constant. Points are moved, vectors change size
inline void Coordinate::operator*=(float value)
{
    _x *= value;
    _y *= value;
}

/* Finds the distance. This is defined as the length for vectors, the distance
    from the origin for points */
inline float Coordinate::getDistance() const
{
    return sqrt((_x * _x) + (_y * _y));
}

typedef vector<Coordinate> LayoutVector;

/* This class creates the layout of congressperson groups,
   using a force directed layout algorithm */
class ForceLayout
{
public:
   static void makeLayout(const VoteDiffMatrix& votes, const CongressGroupDataList& congressGroupData,
                          LayoutVector& congressPositions);

private:
    // Number of iterations to find a solution
    static short _iterationLimit;

    // Initial movement amount for a given force
    static float _forceMoveRatio;

    // Strenth of attraction force to repulsion for a given distance
    static float _attractVsRepulse;

    // Amount of overlap allowed for spots in the graph
    static float _overlapAllowed;

    // Find the initial spot for the next group to place
    static Coordinate findInitialCoordinate(short groupPerSide, float distPerGroup,
                                            short counter);

    // Find the force that a given point exerts on the test point
    static Coordinate findForce(const CongressGroupData& testData,
                                const CongressGroupData& otherData,
                                short voteDifference,
                                const Coordinate& testLoc, const Coordinate& otherLoc);
};
