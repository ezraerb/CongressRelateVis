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

/* Code to draw the final congressperson groups on the screen. They are displayed
    as circles with size based on group size and color based on party composition */
#include<vector>
#include<iostream>
#include<cmath>
#include"congressData.h" // Needed for voteFactory.h
#include"regionMapper.h" // Needed for clusterFactory.h
#include"voteFactory.h" // Needed for clusterFactory.h
#include"clusterFactory.h" // Defines CongressGroupData
#include"forceLayout.h"
#include"displayGroup.h"
#include <GL/glut.h>

// Ratio of group size to area of corresponding circle
const float DisplayGroup::sizeMultiplier = 8;

// Draws the specified group centered on the wanted prosition on the screen
void DisplayGroup::drawGroup(const CongressGroupData& group, const Coordinate& placement)
{
    setColor(group);
    /* The group is set as a circle whose size is propotional to the overall size of the
        group. In the current Congress data party dominates so completely that trying to
        plot other characteristics of groups leads to meaningless results */
    short index;
    glBegin(GL_POLYGON);
    float angle = (3.14159265 * 2) / 32; // Avoid recalculating it all the time

    /* Size on screen is proportional to group size, so radius is propotional to its
        square root. sizeMultiplier tunes the exact ratio */
    float radius = getGroupRadius(group);
    for (index = 0; index < 32; index++) {
        float tempX, tempY;
        tempX = placement.getX() + radius * cos(angle * index);
        tempY = placement.getY() + radius * sin(angle * index);
        glVertex2f(tempX, tempY);
    }
    glEnd();
}

// Given a group of congresspeople, return the radius of the circle to display it
float DisplayGroup::getGroupRadius(const CongressGroupData& group)
{
    /* Size on screen is proportional to group size, so radius is propotional to its
        square root. sizeMultiplier tunes the exact ratio */
    return sqrt((float)group.getCount() * sizeMultiplier / 3.14159265);
}

// Draws a connection between two groups
void DisplayGroup::drawLink(const CongressGroupData& group1, const CongressGroupData& group2,
                            const Coordinate& start, const Coordinate& end)
{
    setColor(group1);
    glVertex2f(start.getX(), start.getY());
    setColor(group2);
    glVertex2f(end.getX(), end.getY());
}

// Sets the color for a group, which depends on the party composition
void DisplayGroup::setColor(const CongressGroupData& group)
{
    /* The color is a blend of the party composition of the group:
        blue for D, red for R, green for other.
        NOTE: The last is not a political commentary; green was the only
        primary color left! */
    float blue, red, green;
    short groupSize = group._parties[0] + group._parties[1] + group._parties[2];
    blue = (float)group._parties[0] / (float) groupSize;
    red = (float)group._parties[1] / (float) groupSize;
    green = (float)group._parties[2] / (float) groupSize;
    glColor3f(red, green, blue);
}
