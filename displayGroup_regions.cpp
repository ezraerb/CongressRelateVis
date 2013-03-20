#include<vector>
#include<iostream>
#include<cmath>
#include"congressData.h" // Needed for voteFactory.h
#include"RegionMapper.h" // Needed for clusterFactory.h
#include"voteFactory.h" // Needed for clusterFactory.h
#include"clusterFactory.h" // Defines CongressGroupData
#include"forceLayout.h"
#include"displayGroup.h"
#include <GL/glut.h>

const float DisplayGroup::sizeMultiplier = 1.5; // Display every group 50% larger than the calculated size

// Draws the specified group centered on the wanted prosition on the screen
void DisplayGroup::drawGroup(const CongressGroupData& group, const Coordinate& placement)
{
    setColor(group);
    /* The group is set as a circle with lines in different directions, representing the
        size of the group in different regions. To avoid having big groups overlap much,
        the size of the line grows non-linearly with the size. The circle has radius 2,
        and the lines have that width */
    short index;
    glBegin(GL_POLYGON);
    float angle = (3.14159265 * 2) / 32; // Avoid recalculating it all the time
    for (index = 0; index < 32; index++) {
        float tempX, tempY;
        tempX = placement.getX() + 2.0 * sizeMultiplier * cos(angle * index);
        tempY = placement.getY() + 2.0 * sizeMultiplier * sin(angle * index);
        glVertex2f(tempX, tempY);
    }
    glEnd();

    // Need the sine and cosine of a 45 degree angle. Both are 1/sqrt(2)
    float sincos45 = 1 / sqrt(2.0);

    if (group._regions[1] > 0) {
        // Northeast, this line is diagonal to the upper right
        float lineLength = findGroupLineLength(group._regions[1]) * sizeMultiplier;
        glBegin(GL_POLYGON);
        glVertex2f(placement.getX() - (sincos45 * sizeMultiplier), placement.getY() + (sincos45 * sizeMultiplier));
        glVertex2f(placement.getX() - (sincos45 * sizeMultiplier) + (lineLength * sincos45), placement.getY() + (sincos45 * sizeMultiplier) + (lineLength * sincos45));
        glVertex2f(placement.getX() + (sincos45 * sizeMultiplier) + (lineLength * sincos45), placement.getY() - (sincos45 * sizeMultiplier) + (lineLength * sincos45));
        glVertex2f(placement.getX() + (sincos45 * sizeMultiplier), placement.getY() - (sincos45 * sizeMultiplier));
        glEnd();
    }

    if (group._regions[2] > 0) {
        // Southeast, this line is diagonal to the lower right
        float lineLength = findGroupLineLength(group._regions[2]) * sizeMultiplier;
        glBegin(GL_POLYGON);
        glVertex2f(placement.getX() + (sincos45 * sizeMultiplier), placement.getY() + (sincos45 * sizeMultiplier));
        glVertex2f(placement.getX() + (sincos45 * sizeMultiplier) + (lineLength * sincos45), placement.getY() + (sincos45 * sizeMultiplier) - (lineLength * sincos45));
        glVertex2f(placement.getX() - (sincos45 * sizeMultiplier) + (lineLength * sincos45), placement.getY() - (sincos45 * sizeMultiplier) - (lineLength * sincos45));
        glVertex2f(placement.getX() - (sincos45 * sizeMultiplier), placement.getY() - (sincos45 * sizeMultiplier));
        glEnd();
    }

    if (group._regions[3] > 0) {
        // Central, this line is straight down (Texas dominates in the congressperson count)
        float lineLength = findGroupLineLength(group._regions[3]) * sizeMultiplier;
        glBegin(GL_POLYGON);
        glVertex2f(placement.getX() + sizeMultiplier, placement.getY());
        glVertex2f(placement.getX() + sizeMultiplier, placement.getY() - lineLength);
        glVertex2f(placement.getX() - sizeMultiplier, placement.getY() - lineLength);
        glVertex2f(placement.getX() - sizeMultiplier, placement.getY());
        glEnd();
    }

    if (group._regions[4] > 0) {
        // West, this line is straight to the left
        float lineLength = findGroupLineLength(group._regions[4]) * sizeMultiplier;
        glBegin(GL_POLYGON);
        glVertex2f(placement.getX(), placement.getY() - sizeMultiplier);
        glVertex2f(placement.getX() - lineLength, placement.getY() - sizeMultiplier);
        glVertex2f(placement.getX() - lineLength, placement.getY() + sizeMultiplier);
        glVertex2f(placement.getX(), placement.getY() + sizeMultiplier);
        glEnd();
    }
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

// Finds the length of a line to represent a region of a given size
float DisplayGroup::findGroupLineLength(short groupSize)
{
    /* To avoid excessive overlap, the amount the segment grows per group
       member shrinks as the group gets larger. Need a formula that is quick
       to calculate but still produces usable results. The one chosen here is
       group member add to segment
        1 - 10         2 - (0.2(n-1)) =
                       2 - 0.2n + 0.2 =
                       2.2 - 0.2n

        11 +          0.3 - (0.006(n-10)) =
                      0.3 - 0.006n - 0.06 =
                      0.36 - 0.006n

        Sum these to get the totals:
        1 - 10        sum(2.2 - 0.2n) =
                      2.2n - 0.2(sum n) =
                      2.2n - 0.2(n)(n+1)0.5 =
                      2.2n - 0.1n^2 - 0.1n =
                      2.1n - 0.1n^2

                    sum[1..11](2.2 - 0.2n) + sum[11..n](0.36 - 0.006n) =
        11 +        11 + sum[11..n](0.36 - 0.006n) =
                    11 + 0.36(n - 10) - 0.006(sum[11..n] n) =
                    11 + 0.36(n - 10) - 0.006((n)(n + 1)(0.5) - (10)(11)(0.5)) =
                    11 + 0.36(n - 10) - 0.003((n)(n + 1) - 110) =
                    11 + 0.36n - 3.6 - 0.003n^2 - 0.003n + 0.33 =
                    7.73 + 0.357n - 0.003n^2


        Finally, add two more to those totals to ensure the line sticks beyond the
        central circle */
    float size = (float)groupSize;
    if (size > 60)
        size = 60;
    if (size < 11)
        return (2.1 * size) - (0.1 * size * size) + 2.0;
    else
        return 7.73 + (0.357 * size) - (0.003 * size * size) + 2.0;
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
