/***************************************
 $Header: /home/amb/CVS/routino/src/planetsplitter.c,v 1.70 2010-04-03 14:20:16 amb Exp $

 OSM planet file splitter.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2008-2010 Andrew M. Bishop

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
#include <errno.h>

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


/* Local functions */

static void print_usage(int detail);


/*++++++++++++++++++++++++++++++++++++++
  The main program for the router.
  ++++++++++++++++++++++++++++++++++++++*/

int main(int argc,char** argv)
{
 NodesX    *Nodes;
 SegmentsX *Segments,*SuperSegments=NULL,*MergedSegments=NULL;
 WaysX     *Ways;
 int        iteration=0,quit=0;
 int        max_iterations=10;
 char      *dirname=NULL,*prefix=NULL;
 int        option_parse_only=0,option_process_only=0;
 int        option_filenames=0;
 Profile    profile={0};
 int        arg,i;

 /* Fill in the default profile. */

 profile.transport=Transport_None; /* Not used by planetsplitter */

 profile.allow=0;

 for(i=1;i<Way_Count;i++)
    profile.highway[i]=1;

 for(i=1;i<Property_Count;i++)
    profile.props_yes[i]=1;

 profile.oneway=1; /* Not used by planetsplitter */

 /* Parse the command line arguments */

 for(arg=1;arg<argc;arg++)
   {
    if(!strcmp(argv[arg],"--help"))
       print_usage(1);
    else if(!strcmp(argv[arg],"--slim"))
       option_slim=1;
    else if(!strncmp(argv[arg],"--dir=",6))
       dirname=&argv[arg][6];
    else if(!strncmp(argv[arg],"--tmpdir=",9))
       option_tmpdirname=&argv[arg][9];
    else if(!strncmp(argv[arg],"--prefix=",9))
       prefix=&argv[arg][9];
    else if(!strcmp(argv[arg],"--parse-only"))
       option_parse_only=1;
    else if(!strcmp(argv[arg],"--process-only"))
       option_process_only=1;
    else if(!strncmp(argv[arg],"--max-iterations=",17))
       max_iterations=atoi(&argv[arg][17]);
    else if(!strncmp(argv[arg],"--transport=",12))
      {
       Transport transport=TransportType(&argv[arg][12]);
       if(transport==Transport_None)
          print_usage(0);
       profile.allow|=ALLOWED(transport);
      }
    else if(!strncmp(argv[arg],"--not-highway=",14))
      {
       Highway highway=HighwayType(&argv[arg][14]);
       if(highway==Way_Count)
          print_usage(0);
       profile.highway[highway]=0;
      }
    else if(!strncmp(argv[arg],"--not-property=",15))
      {
       Property property=PropertyType(&argv[arg][15]);
       if(property==Property_Count)
          print_usage(0);
       profile.props_yes[property]=0;
      }
    else if(argv[arg][0]=='-' && argv[arg][1]=='-')
       print_usage(0);
    else
       option_filenames++;
   }

 if(option_parse_only && option_process_only)
    print_usage(0);

 if(option_filenames && option_process_only)
    print_usage(0);

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

 Nodes=NewNodeList(option_parse_only||option_process_only);

 Segments=NewSegmentList(option_parse_only||option_process_only);

 Ways=NewWayList(option_parse_only||option_process_only);

 /* Parse the file */

 if(option_filenames)
   {
    for(arg=1;arg<argc;arg++)
      {
       FILE *file;

       if(argv[arg][0]=='-' && argv[arg][1]=='-')
          continue;

       file=fopen(argv[arg],"rb");

       if(!file)
         {
          fprintf(stderr,"Cannot open file '%s' for reading [%s].\n",argv[arg],strerror(errno));
          exit(EXIT_FAILURE);
         }

       printf("\nParse OSM Data [%s]\n==============\n\n",argv[arg]);
       fflush(stdout);

       ParseOSM(file,Nodes,Segments,Ways,&profile);

       fclose(file);
      }
   }
 else if(!option_process_only)
   {
    printf("\nParse OSM Data\n==============\n\n");
    fflush(stdout);

    ParseOSM(stdin,Nodes,Segments,Ways,&profile);
   }

 if(option_parse_only)
   {
    FreeNodeList(Nodes,1);
    FreeSegmentList(Segments,1);
    FreeWayList(Ways,1);

    return(0);
   }

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

       FreeSegmentList(SuperSegments,0);

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

 FreeSegmentList(Segments,0);

 FreeSegmentList(SuperSegments,0);

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

 FreeNodeList(Nodes,0);

 /* Write out the segments */

 SaveSegmentList(Segments,FileName(dirname,prefix,"segments.mem"));

 FreeSegmentList(Segments,0);

 /* Write out the ways */

 SaveWayList(Ways,FileName(dirname,prefix,"ways.mem"),&profile);

 FreeWayList(Ways,0);

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Print out the usage information.

  int detail The level of detail to use - 0 = low, 1 = high.
  ++++++++++++++++++++++++++++++++++++++*/

static void print_usage(int detail)
{
 fprintf(stderr,
         "Usage: planetsplitter [--help]\n"
         "                      [--dir=<dirname>] [--prefix=<name>]\n"
         "                      [--slim] [--tmpdir=<dirname>]\n"
         "                      [--parse-only | --process-only]\n"
         "                      [--max-iterations=<number>]\n"
         "                      [--transport=<transport> ...]\n"
         "                      [--not-highway=<highway> ...]\n"
         "                      [--not-property=<property> ...]\n"
         "                      [<filename.osm> ...]\n");

 if(detail)
    fprintf(stderr,
            "\n"
            "--help                    Prints this information.\n"
            "\n"
            "--dir=<dirname>           The directory containing the routing database.\n"
            "--prefix=<name>           The filename prefix for the routing database.\n"
            "\n"
            "--slim                    Use less RAM and more temporary files.\n"
            "--tmpdir=<dirname>        The directory name for temporary files.\n"
            "\n"
            "--parse-only              Parse the input OSM files and store the results.\n"
            "--process-only            Process the stored results from previous option.\n"
            "\n"
            "--max-iterations=<number> The number of iterations for finding super-nodes.\n"
            "\n"
            "--transport=<transport>   Store only data usable by the selected transports.\n"
            "--not-highway=<highway>   Ignore highways of the selected types.\n"
            "--not-property=<property> Ignore highway properties of the selected types.\n"
            "\n"
            "<filename.osm>            The name of the file to process (by default data\n"
            "                          is read from standard input).\n"
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

 exit(!detail);
}
