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

// Main driver for the Congressional Vote Similiarity program
#include <stdlib.h>
#include <utility>
#include <GL/glut.h>
#include <iostream>
#include <sstream>
#include <vector>
#include "congressData.h"
#include "regionMapper.h"
#include "voteFactory.h"
#include "clusterFactory.h"
#include "corrolation.h"
#include "forceLayout.h"
#include "displayGroup.h"

using std::cerr;
using std::endl;
using std::stringstream;

/* UGLY HACK: Windows has an error where any output to the console makes the drawing winddow
   break. Just moving the window will cause the program to hang. The painful result is that all
   data needs to be calculated BEFORE creating the drawing window. GLUT does not allow the drawing
   routine to have parameters, with the consquence that everything must be passed in using
   global variables. Here they are */
CongressGroupDataList congressGroupData;
VoteDiffMatrix clusteredVotes;
LayoutVector congressPositions;

void keyboard(unsigned char key, int x, int y)
{
  switch (key)
  {
    case '\x1B':
      exit(EXIT_SUCCESS);
      break;
  }
}


// Initial setup of 2D scene
void sceneInit(void)
{
    // Black background
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glPointSize(1.0);
    // Initialize a 2D drawing surface
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 640.0, 0.0, 640.0);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    unsigned short index, index2;
    // Draw the groups
    for (index = 0; index < congressPositions.size(); index++)
        DisplayGroup::drawGroup(congressGroupData.at(index), congressPositions.at(index));

    // Draw the major connections between them

    glBegin(GL_LINES);
    for (index = 0; index < clusteredVotes.size(); index++)
        for (index2 = 0; index2 < index; index2++)
            if (clusteredVotes.at(index).at(index2) > 0)
                DisplayGroup::drawLink(congressGroupData.at(index), congressGroupData.at(index2),
                                        congressPositions.at(index), congressPositions.at(index2));
    glEnd();

    glFlush();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);

    // Need to compute the data before creating the window
    /* Get the wanted Congressional session to graph. It can be specified
        as either a session number or the starting year. Note that the starting
        year is the year AFTER the election. If no year is specified, use the
        most recent.
        SEMI-HACK: If the value has less than four digits, assume its a session
        number. This will be valid for at least the next thousand years */
    if (argc != 2) {
        cerr << "Invalid arguments. Specify starting year or number of Congressioanl session" << endl;
        exit(1);
    }

    short firstYear = atoi(argv[1]);
    if (firstYear < 1000)
        /* The files are in terms of years, so need to convert. Sessions are numbered
            every two years, starting with the first Congress. This procudes a
            straightforward mapping
            NOTE: Remember that the first session was 1, not 0, so need to subtract two
            years from when Congress actally started in the formula */
        firstYear = (firstYear * 2) + 1787;
    /* If the year specified is even, assume the last year of the wanted session
        was specified. Technically an error, but easy to deal with */
    else if ((firstYear % 2) != 0)
        firstYear--;
    short lastYear = firstYear + 1; // Get other year of session

    // Load in data to process, with debug off
    CongressData congress(firstYear);
    RegionMapper regions;

    // Find vote differcnes between every member of Congress
    VoteDiffMatrix voteResults;
    VoteFactory::getVoteMatrix(voteResults, congress, firstYear, lastYear);

    /* Print the matrix. Have a bunch of congressmen with no votes, should be treated as
        no similarity with anyone else */

    // Cluster Congresspeople whose votes are close enough that differences are meaningless
    CongressGroupVector clusteredCongress;
    ClusterFactory::formClusters(voteResults, clusteredCongress, congress, 150, 20);

    // Recalculate the vote differences to be between the clusters
    /* Drop large vote differences in the results afterward, they add lots
        of compute without affecting the final results much */
    ClusterFactory::getClusterDistanceMap(voteResults, clusteredCongress, clusteredVotes);
    VoteFactory::filterLargeMismatch(clusteredVotes, ClusterFactory::meaningfulDifferenceLimit);

    // Find how the groups distribute based on wanted characteristics
    ClusterFactory::getClusterCongressData(clusteredCongress, congress, regions, congressGroupData);

    // Layout the groups based on vote similarity
    ForceLayout::makeLayout(clusteredVotes, congressGroupData, congressPositions);

    /* To avoid cluttering the finalgraph, only retain the strongest correlations
        for output */
    VoteFactory::filterLargeMismatch(clusteredVotes, 350);

    glutInitWindowPosition(-1, -1);
    glutInitWindowSize(640, 640);
    stringstream title;
    title << "Congress voting similiarity " << firstYear << "-" << lastYear;
    glutCreateWindow(title.str().c_str());

    glutKeyboardFunc(&keyboard);
    glutDisplayFunc(&display);
    glutIdleFunc(&display);
    sceneInit();
    glutMainLoop();

    return EXIT_SUCCESS;
}

