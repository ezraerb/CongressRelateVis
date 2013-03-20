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
// This class takes a Congressperson cluster and displays it on the screen

class DisplayGroup
{
  public:
    // Draws the specified group centered on the wanted prosition on the screen
    static void drawGroup(const CongressGroupData& group, const Coordinate& placement);

    // Draws a connection between two groups
    static void drawLink(const CongressGroupData& group1, const CongressGroupData& group2,
                         const Coordinate& start, const Coordinate& end);

    // Given a group of congresspeople, return the radius of the circle to display it
    static float getGroupRadius(const CongressGroupData& group);

private:
    // Allow easy adjustment of the size of the groups in code, so easy to adapt to new datasets
    static const float sizeMultiplier;

    // Sets the color for a group, which depends on the party composition
    static void setColor(const CongressGroupData& group);

    // Finds the length of a line to represent a region of a given size
    static float findGroupLineLength(short groupSize);
};
