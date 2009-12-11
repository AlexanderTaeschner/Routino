/***************************************
 $Header: /home/amb/CVS/routino/src/planetsplitter.c,v 1.65 2009-12-11 19:27:39 amb Exp $

 OSM planet file splitter.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2008,2009 Andrew M. Bishop

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "typesx.h"
#include "types.h"
#include "functionsx.h"
#include "functions.h"
#include "nodesx.h"
#include "segmentsx.h"
#include "waysx.h"
#include "superx.h"
#include "profiles.h"
#include "ways.h"


/* Variables */

/*+ The option to use a slim mode with file-backed read-only intermediate storage. +*/
int option_slim=0;

/*+ The name of the temporary directory. +*/
char *option_tmpdirname=NULL;


int main(int argc,char** argv)
{
 NodesX *Nodes;
 SegmentsX *Segments,*SuperSegments=NULL,*MergedSegments=NULL;
 WaysX *Ways;
 int iteration=0,quit=0;
 int max_iterations=10;
 char *dirname=NULL,*prefix=NULL;
 Profile profile={0};
 int i;

 /* Fill in the default profile. */

 profile.transport=Transport_None; /* Not used by planetsplitter */

 profile.allow=0;

 for(i=1;i<Way_Count;i++)
    profile.highway[i]=1;

 for(i=1;i<Property_Count;i++)
    profile.props_yes[i]=1;

 profile.oneway=1; /* Not used by planetsplitter */

 /* Parse the command line arguments */

 while(--argc>=1)
   {
    if(!strcmp(argv[argc],"--help"))
       goto usage;
    else if(!strcmp(argv[argc],"--slim"))
       option_slim=1;
    else if(!strncmp(argv[argc],"--dir=",6))
       dirname=&argv[argc][6];
    else if(!strncmp(argv[argc],"--tmpdir=",9))
       option_tmpdirname=&argv[argc][9];
    else if(!strncmp(argv[argc],"--prefix=",9))
       prefix=&argv[argc][9];
    else if(!strncmp(argv[argc],"--max-iterations=",17))
       max_iterations=atoi(&argv[argc][17]);
    else if(!strncmp(argv[argc],"--transport=",12))
      {
       Transport transport=TransportType(&argv[argc][12]);
       if(transport==Transport_None)
          goto usage;
       profile.allow|=ALLOWED(transport);
      }
    else if(!strncmp(argv[argc],"--not-highway=",14))
      {
       Highway highway=HighwayType(&argv[argc][14]);
       if(highway==Way_Count)
          goto usage;
       profile.highway[highway]=0;
      }
    else if(!strncmp(argv[argc],"--not-property=",15))
      {
       Property property=PropertyType(&argv[argc][15]);
       if(property==Property_Count)
          goto usage;
       profile.props_yes[property]=0;
      }
    else
      {
      usage:

       fprintf(stderr,"Usage: planetsplitter\n"
                      "                      [--help]\n"
                      "                      [--dir=<name>] [--prefix=<name>]\n"
                      "                      [--slim] [--tmpdir=<name>]\n"
                      "                      [--max-iterations=<number>]\n"
                      "                      [--transport=<transport> ...]\n"
                      "                      [--not-highway=<highway> ...]\n"
                      "                      [--not-property=<property> ...]\n"
                      "\n"
                      "<transport> defaults to all but can be set to:\n"
                      "%s"
                      "\n"
                      "<highway> can be selected from:\n"
                      "%s"
                      "\n"
                      "<property> can be selected from:\n"
                      "%s",
               TransportList(),HighwayList(),PropertyList());

       return(1);
      }
   }

 if(!option_tmpdirname)
   {
    if(!dirname)
       option_tmpdirname=".";
    else
       option_tmpdirname=dirname;
   }

 if(!profile.allow)
    profile.allow=Allow_ALL;

 /* Create new node, segment and way variables */

 Nodes=NewNodeList();

 Segments=NewSegmentList();

 Ways=NewWayList();

 /* Parse the file */

 printf("\nParse OSM Data\n==============\n\n");
 fflush(stdout);

 ParseXML(stdin,Nodes,Segments,Ways,&profile);

 /* Process the data */

 printf("\nProcess OSM Data\n================\n\n");
 fflush(stdout);

 /* Sort the nodes, segments and ways */

 SortNodeList(Nodes);

 SortSegmentList(Segments);

 SortWayList(Ways);

 /* Remove bad segments (must be after sorting the nodes and segments) */

 RemoveBadSegments(Nodes,Segments);

 /* Remove non-highway nodes (must be after removing the bad segments) */

 RemoveNonHighwayNodes(Nodes,Segments);

 /* Measure the segments and replace node/way id with index (must be after removing non-highway nodes) */

 UpdateSegments(Segments,Nodes,Ways);


 /* Repeated iteration on Super-Nodes and Super-Segments */

 do
   {
    printf("\nProcess Super-Data (iteration %d)\n================================%s\n\n",iteration,iteration>9?"=":"");
    fflush(stdout);

    if(iteration==0)
      {
       /* Select the super-nodes */

       ChooseSuperNodes(Nodes,Segments,Ways);

       /* Select the super-segments */

       SuperSegments=CreateSuperSegments(Nodes,Segments,Ways,iteration);
      }
    else
      {
       SegmentsX *SuperSegments2;

       /* Select the super-nodes */

       ChooseSuperNodes(Nodes,SuperSegments,Ways);

       /* Select the super-segments */

       SuperSegments2=CreateSuperSegments(Nodes,SuperSegments,Ways,iteration);

       if(SuperSegments->xnumber==SuperSegments2->xnumber)
          quit=1;

       FreeSegmentList(SuperSegments);

       SuperSegments=SuperSegments2;
      }

    /* Sort the super-segments */

    SortSegmentList(SuperSegments);

    /* Remove duplicated super-segments */

    DeduplicateSegments(SuperSegments,Nodes,Ways);

    iteration++;

    if(iteration>max_iterations)
       quit=1;
   }
 while(!quit);

 /* Combine the super-segments */

 printf("\nCombine Segments and Super-Segments\n===================================\n\n");
 fflush(stdout);

 /* Merge the super-segments */

 MergedSegments=MergeSuperSegments(Segments,SuperSegments);

 FreeSegmentList(Segments);

 FreeSegmentList(SuperSegments);

 Segments=MergedSegments;

 /* Rotate segments so that node1<node2 */

 RotateSegments(Segments);

 /* Sort the segments */

 SortSegmentList(Segments);

 /* Remove duplicated segments */

 DeduplicateSegments(Segments,Nodes,Ways);

 /* Cross reference the nodes and segments */

 printf("\nCross-Reference Nodes and Segments\n==================================\n\n");
 fflush(stdout);

 /* Sort the node list geographically */

 SortNodeListGeographically(Nodes);

 /* Create the real segments and nodes */

 CreateRealNodes(Nodes,iteration);

 CreateRealSegments(Segments,Ways);

 /* Fix the segment and node indexes */

 IndexNodes(Nodes,Segments);

 IndexSegments(Segments,Nodes);

 /* Output the results */

 printf("\nWrite Out Database Files\n========================\n\n");
 fflush(stdout);

 /* Write out the nodes */

 SaveNodeList(Nodes,FileName(dirname,prefix,"nodes.mem"));

 FreeNodeList(Nodes);

 /* Write out the segments */

 SaveSegmentList(Segments,FileName(dirname,prefix,"segments.mem"));

 FreeSegmentList(Segments);

 /* Write out the ways */

 SaveWayList(Ways,FileName(dirname,prefix,"ways.mem"),&profile);

 FreeWayList(Ways);

 return(0);
}
