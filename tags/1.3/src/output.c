/***************************************
 $Header: /home/amb/CVS/routino/src/output.c,v 1.18 2010-01-13 18:49:41 amb Exp $

 Routing output generator.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2008,2009,2010 Andrew M. Bishop

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


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "types.h"
#include "functions.h"
#include "nodes.h"
#include "segments.h"
#include "ways.h"
#include "results.h"


/*+ The option to calculate the quickest route insted of the shortest. +*/
extern int option_quickest;

/*+ The files to write to. +*/
static FILE *gpxtrackfile=NULL,*gpxroutefile=NULL,*textfile=NULL,*textallfile=NULL;

/*+ Heuristics for determining if a junction is important. +*/
static char junction_other_way[Way_Count][Way_Count]=
 { /* M, T, P, S, T, U, R, S, T, C, P, S = Way type of route not taken */
  {   1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Motorway     */
  {   1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Trunk        */
  {   1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Primary      */
  {   1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, /* Secondary    */
  {   1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 }, /* Tertiary     */
  {   1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 }, /* Unclassified */
  {   1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 }, /* Residential  */
  {   1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* Service      */
  {   1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 }, /* Track        */
  {   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 }, /* Cycleway     */
  {   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, /* Path         */
  {   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, /* Steps        */
 };

static int junction_angle(Nodes *nodes,Segment *segment1,Segment *segment2,index_t node);
static int bearing_angle(Nodes *nodes,Segment *segment,index_t node);


/*++++++++++++++++++++++++++++++++++++++
  Open the files and print the head.

  const char *copyright The name of a file that might exist and contain copyright information.
  ++++++++++++++++++++++++++++++++++++++*/

void PrintRouteHead(const char *copyright)
{
 char *source=NULL,*license=NULL;

 if(copyright)
   {
    struct stat buf;

    if(!stat(copyright,&buf))
      {
       FILE *file=fopen(copyright,"r");
       char *string=(char*)malloc(buf.st_size+1);
       char *p;

       fread(string,buf.st_size,1,file);
       string[buf.st_size]=0;

       p=string;
       while(*p)
         {
          if(!strncmp(p,"Source:",7))
            {
             p+=7;
             while(*p==' ' || *p=='t')
                p++;
             source=p;
             while(*p && *p!='\r' && *p!='\n')
                p++;
             while(*p=='\r' || *p=='\n')
                *p++=0;
            }
          else if(!strncmp(p,"License:",8) || !strncmp(p,"Licence:",8))
            {
             p+=8;
             while(*p==' ' || *p=='t')
                p++;
             license=p;
             while(*p && *p!='\r' && *p!='\n')
                p++;
             while(*p=='\r' || *p=='\n')
                *p++=0;
            }
          else
            {
             while(*p && *p!='\r' && *p!='\n')
                p++;
             while(*p=='\r' || *p=='\n')
                *p++=0;
            }
         }

       fclose(file);
      }
   }

 /* Open the files */

 if(option_quickest==0)
   {
    /* Print the result for the shortest route */

    gpxtrackfile=fopen("shortest-track.gpx","w");
    gpxroutefile=fopen("shortest-route.gpx","w");
    textfile    =fopen("shortest.txt","w");
    textallfile =fopen("shortest-all.txt","w");

    if(!gpxtrackfile)
       fprintf(stderr,"Warning: Cannot open file 'shortest-track.gpx' to write.\n");
    if(!gpxroutefile)
       fprintf(stderr,"Warning: Cannot open file 'shortest-route.gpx' to write.\n");
    if(!textfile)
       fprintf(stderr,"Warning: Cannot open file 'shortest.txt' to write.\n");
    if(!textallfile)
       fprintf(stderr,"Warning: Cannot open file 'shortest-all.txt' to write.\n");
   }
 else
   {
    /* Print the result for the quickest route */

    gpxtrackfile=fopen("quickest-track.gpx","w");
    gpxroutefile=fopen("quickest-route.gpx","w");
    textfile    =fopen("quickest.txt","w");
    textallfile =fopen("quickest-all.txt","w");

    if(!gpxtrackfile)
       fprintf(stderr,"Warning: Cannot open file 'quickest-track.gpx' to write.\n");
    if(!gpxroutefile)
       fprintf(stderr,"Warning: Cannot open file 'quickest-route.gpx' to write.\n");
    if(!textfile)
       fprintf(stderr,"Warning: Cannot open file 'quickest.txt' to write.\n");
    if(!textallfile)
       fprintf(stderr,"Warning: Cannot open file 'quickest-all.txt' to write.\n");
   }

 /* Print the head of the files */

 if(gpxtrackfile)
   {
    fprintf(gpxtrackfile,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(gpxtrackfile,"<gpx version=\"1.1\" creator=\"Routino\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/1\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n");

    fprintf(gpxtrackfile,"<metadata>\n");
    fprintf(gpxtrackfile,"<desc><![CDATA[%s route between 'start' and 'finish' waypoints]]></desc>\n",option_quickest?"Quickest":"Shortest");
    if(source)
       fprintf(gpxtrackfile,"<copyright author=\"%s\">\n",source);
    if(license)
       fprintf(gpxtrackfile,"<license>%s</license>\n",license);
    if(source)
       fprintf(gpxtrackfile,"</copyright>\n");
    fprintf(gpxtrackfile,"</metadata>\n");

    fprintf(gpxtrackfile,"<trk>\n");
   }

 if(gpxroutefile)
   {
    fprintf(gpxroutefile,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(gpxroutefile,"<gpx version=\"1.1\" creator=\"Routino\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/1\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n");

    fprintf(gpxroutefile,"<metadata>\n");
    fprintf(gpxroutefile,"<desc><![CDATA[%s route between 'start' and 'finish' waypoints]]></desc>\n",option_quickest?"Quickest":"Shortest");
    if(source)
       fprintf(gpxroutefile,"<copyright author=\"%s\">\n",source);
    if(license)
       fprintf(gpxroutefile,"<license>%s</license>\n",license);
    if(source)
       fprintf(gpxroutefile,"</copyright>\n");
    fprintf(gpxroutefile,"</metadata>\n");

    fprintf(gpxroutefile,"<rte>\n");
    fprintf(gpxroutefile,"<name>%s route</name>\n",option_quickest?"Quickest":"Shortest");
   }

 if(textfile)
   {
    if(source)
       fprintf(textfile,"# Source: %s\n",source);
    if(license)
       fprintf(textfile,"# License: %s\n",license);
    if(source || license)
       fprintf(textfile,"#\n");
    fprintf(textfile,"#Latitude\tLongitude\tSection \tSection \tTotal   \tTotal   \tPoint\tTurn\tBearing\tHighway\n");
    fprintf(textfile,"#        \t         \tDistance\tDuration\tDistance\tDuration\tType \t    \t       \t       \n");
                     /* "%10.6f\t%11.6f\t%6.3f km\t%4.1f min\t%5.1f km\t%4.0f min\t%s\t %+d\t %+d\t%s\n" */
   }

 if(textallfile)
   {
    if(source)
       fprintf(textallfile,"# Source: %s\n",source);
    if(license)
       fprintf(textallfile,"# License: %s\n",license);
    if(source || license)
       fprintf(textallfile,"#\n");
    fprintf(textallfile,"#Latitude\tLongitude\t    Node\tType\tSegment\tSegment\tTotal\tTotal  \tSpeed\tBearing\tHighway\n");
    fprintf(textallfile,"#        \t         \t        \t    \tDist   \tDurat'n\tDist \tDurat'n\t     \t       \t       \n");
                        /* "%10.6f\t%11.6f\t%8d%c\t%s\t%5.3f\t%5.2f\t%5.2f\t%5.1f\t%3d\t%4d\t%s\n" */
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Print the optimum route between two nodes.

  Results **results The set of results to print (some may be NULL - ignore them).

  int nresults The number of results in the list.

  Nodes *nodes The list of nodes.

  Segments *segments The set of segments to use.

  Ways *ways The list of ways.

  Profile *profile The profile containing the transport type, speeds and allowed highways.
  ++++++++++++++++++++++++++++++++++++++*/

void PrintRoute(Results **results,int nresults,Nodes *nodes,Segments *segments,Ways *ways,Profile *profile)
{
 int point=1;
 distance_t cum_distance=0;
 duration_t cum_duration=0;
 double finish_lat,finish_lon;
 int segment_count=0;
 int route_count=0;

 while(!results[point])
    point++;

 while(point<=nresults)
   {
    int nextpoint=point;
    double start_lat,start_lon;
    distance_t junc_distance=0;
    duration_t junc_duration=0;
    Result *result;

    fprintf(gpxtrackfile,"<trkseg>\n");

    if(IsFakeNode(results[point]->start))
       GetFakeLatLong(results[point]->start,&start_lat,&start_lon);
    else
       GetLatLong(nodes,results[point]->start,&start_lat,&start_lon);

    if(IsFakeNode(results[point]->finish))
       GetFakeLatLong(results[point]->finish,&finish_lat,&finish_lon);
    else
       GetLatLong(nodes,results[point]->finish,&finish_lat,&finish_lon);

    result=FindResult(results[point],results[point]->start);

    do
      {
       double latitude,longitude;
       Result *nextresult;

       if(result->node==results[point]->start)
         {latitude=start_lat; longitude=start_lon;}
       else if(result->node==results[point]->finish)
         {latitude=finish_lat; longitude=finish_lon;}
       else
          GetLatLong(nodes,result->node,&latitude,&longitude);

       if(gpxtrackfile)
          fprintf(gpxtrackfile,"<trkpt lat=\"%.6f\" lon=\"%.6f\"/>\n",
                  radians_to_degrees(latitude),radians_to_degrees(longitude));

       nextresult=FindResult(results[point],result->next);

       if(!nextresult)
          for(nextpoint=point+1;nextpoint<=nresults;nextpoint++)
             if(results[nextpoint])
               {
                nextresult=FindResult(results[nextpoint],results[nextpoint]->start);
                nextresult=FindResult(results[nextpoint],nextresult->next);
                break;
               }

       if(result->node!=results[point]->start)
         {
          distance_t seg_distance=0;
          duration_t seg_duration=0;
          Way *resultway;
          int important=0;

          /* Get the properties of this segment */

          resultway=LookupWay(ways,result->segment->way);

          seg_distance+=DISTANCE(result->segment->distance);
          seg_duration+=Duration(result->segment,resultway,profile);
          junc_distance+=seg_distance;
          junc_duration+=seg_duration;
          cum_distance+=seg_distance;
          cum_duration+=seg_duration;

          /* Decide if this is an important junction */

          if(result->node==results[point]->finish)
             important=10;
          else
            {
             Segment *segment=FirstSegment(segments,nodes,result->node);

             do
               {
                index_t othernode=OtherNode(segment,result->node);

                if(othernode!=result->prev && segment!=result->segment)
                   if(IsNormalSegment(segment) && (!profile->oneway || !IsOnewayTo(segment,result->node)))
                     {
                      Way *way=LookupWay(ways,segment->way);

                      if(othernode==result->next) /* the next segment that we follow */
                        {
                         if(HIGHWAY(way->type)!=HIGHWAY(resultway->type))
                            if(important<2)
                               important=2;
                        }
                      else /* a segment that we don't follow */
                        {
                         if(junction_other_way[HIGHWAY(resultway->type)-1][HIGHWAY(way->type)-1])
                            if(important<3)
                               important=3;

                         if(important<1)
                            important=1;
                        }
                     }

                segment=NextSegment(segments,segment,result->node);
               }
             while(segment);
            }

          /* Print out the important points (junctions / waypoints) */

          if(important>1 && important<10)
            {
             /* Don't print the intermediate finish points (the final finish point is special) */

             if(gpxroutefile)
                fprintf(gpxroutefile,"<rtept lat=\"%.6f\" lon=\"%.6f\"><name>TRIP%03d</name></rtept>\n",
                        radians_to_degrees(latitude),radians_to_degrees(longitude),
                        ++route_count);
            }

          if(important>1)
            {
             char *type;

             if(important==10)
                type="Waypt";
             else
                type="Junct";

             /* Do print the intermediate finish points (because they have correct junction distances) */

             if(textfile)
               {
                if(nextresult)
                   fprintf(textfile,"%10.6f\t%11.6f\t%6.3f km\t%4.1f min\t%5.1f km\t%4.0f min\t%s\t %+d\t %+d\t%s\n",
                           radians_to_degrees(latitude),radians_to_degrees(longitude),
                           distance_to_km(junc_distance),duration_to_minutes(junc_duration),
                           distance_to_km(cum_distance),duration_to_minutes(cum_duration),
                           type,
                           (22+junction_angle(nodes,result->segment,nextresult->segment,result->node))/45,
                           ((22+bearing_angle(nodes,nextresult->segment,nextresult->node))/45+4)%8-4,
                           WayName(ways,resultway));
                else
                   fprintf(textfile,"%10.6f\t%11.6f\t%6.3f km\t%4.1f min\t%5.1f km\t%4.0f min\t%s\t\t\t%s\n",
                           radians_to_degrees(latitude),radians_to_degrees(longitude),
                           distance_to_km(junc_distance),duration_to_minutes(junc_duration),
                           distance_to_km(cum_distance),duration_to_minutes(cum_duration),
                           type,
                           WayName(ways,resultway));
               }

             junc_distance=0;
             junc_duration=0;
            }

          /* Print out all of the results */

          if(textallfile)
            {
             char *type;

             if(important==10)
                type="Waypt";
             else if(important==2)
                type="Change";
             else if(important>=1)
                type="Junct";
             else
                type="Inter";

             fprintf(textallfile,"%10.6f\t%11.6f\t%8d%c\t%s\t%5.3f\t%5.2f\t%5.2f\t%5.1f\t%3d\t%4d\t%s\n",
                     radians_to_degrees(latitude),radians_to_degrees(longitude),
                     IsFakeNode(result->node)?-(result->node&(~NODE_SUPER)):result->node,
                     (!IsFakeNode(result->node) && IsSuperNode(nodes,result->node))?'*':' ',type,
                     distance_to_km(seg_distance),duration_to_minutes(seg_duration),
                     distance_to_km(cum_distance),duration_to_minutes(cum_duration),
                     profile->speed[HIGHWAY(resultway->type)],
                     bearing_angle(nodes,result->segment,result->node),
                     WayName(ways,resultway));
            }
         }
       else if(!cum_distance)
         {
          /* Print out the very first start point */

          if(gpxroutefile)
             fprintf(gpxroutefile,"<rtept lat=\"%.6f\" lon=\"%.6f\"><name>START</name></rtept>\n",
                     radians_to_degrees(latitude),radians_to_degrees(longitude));

          if(textfile)
             fprintf(textfile,"%10.6f\t%11.6f\t%6.3f km\t%4.1f min\t%5.1f km\t%4.0f min\t%s\t\t +%d\t\n",
                     radians_to_degrees(latitude),radians_to_degrees(longitude),
                     0.0,0.0,0.0,0.0,
                     "Waypt",
                     (22+bearing_angle(nodes,nextresult->segment,result->next))/45);

          if(textallfile)
             fprintf(textallfile,"%10.6f\t%11.6f\t%8d%c\t%s\t%5.3f\t%5.2f\t%5.2f\t%5.1f\t\t\t\n",
                     radians_to_degrees(latitude),radians_to_degrees(longitude),
                     IsFakeNode(result->node)?-(result->node&(~NODE_SUPER)):result->node,
                     (!IsFakeNode(result->node) && IsSuperNode(nodes,result->node))?'*':' ',"Waypt",
                     0.0,0.0,0.0,0.0);
         }
       else
         {
          /* Print out the intermediate start points */

          if(gpxroutefile)
             fprintf(gpxroutefile,"<rtept lat=\"%.6f\" lon=\"%.6f\"><name>INTER%d</name></rtept>\n",
                     radians_to_degrees(latitude),radians_to_degrees(longitude),
                     ++segment_count);
         }

       result=nextresult;
      }
    while(point==nextpoint);

    if(gpxtrackfile)
       fprintf(gpxtrackfile,"</trkseg>\n");

    point=nextpoint;
   }

 /* Print the very final point in the route */

 if(gpxroutefile)
    fprintf(gpxroutefile,"<rtept lat=\"%.6f\" lon=\"%.6f\"><name>FINISH</name></rtept>\n",
            radians_to_degrees(finish_lat),radians_to_degrees(finish_lon));
}


/*++++++++++++++++++++++++++++++++++++++
  Print the tail and close the files.
  ++++++++++++++++++++++++++++++++++++++*/

void PrintRouteTail(void)
{
 /* Print the tail of the files */

 if(gpxtrackfile)
   {
    fprintf(gpxtrackfile,"</trk>\n");
    fprintf(gpxtrackfile,"</gpx>\n");
   }

 if(gpxroutefile)
   {
    fprintf(gpxroutefile,"</rte>\n");
    fprintf(gpxroutefile,"</gpx>\n");
   }

 /* Close the files */

 if(gpxtrackfile)
    fclose(gpxtrackfile);
 if(gpxroutefile)
    fclose(gpxroutefile);
 if(textfile)
    fclose(textfile);
 if(textallfile)
    fclose(textallfile);
}


/*++++++++++++++++++++++++++++++++++++++
  Calculate the angle to turn at a junction from segment1 to segment2 at node.

  int junction_angle Returns a value in the range -4 to +4 indicating the angle to turn.

  Nodes *nodes The set of nodes.

  Segment *segment1 The current segment.

  Segment *segment2 The next segment.

  index_t node The node at which they join.

  Straight ahead is zero, turning to the right is positive (90 degrees) and turning to the left is negative.
  Angles are calculated using flat Cartesian lat/long grid approximation (after scaling longitude due to latitude).
  ++++++++++++++++++++++++++++++++++++++*/

static int junction_angle(Nodes *nodes,Segment *segment1,Segment *segment2,index_t node)
{
 double lat1,latm,lat2;
 double lon1,lonm,lon2;
 double angle1,angle2,angle;
 index_t node1,node2;

 node1=OtherNode(segment1,node);
 node2=OtherNode(segment2,node);

 if(IsFakeNode(node1))
    GetFakeLatLong(node1,&lat1,&lon1);
 else
    GetLatLong(nodes,node1,&lat1,&lon1);

 if(IsFakeNode(node))
    GetFakeLatLong(node,&latm,&lonm);
 else
    GetLatLong(nodes,node,&latm,&lonm);

 if(IsFakeNode(node2))
    GetFakeLatLong(node2,&lat2,&lon2);
 else
    GetLatLong(nodes,node2,&lat2,&lon2);

 angle1=atan2((lonm-lon1)*cos(latm),(latm-lat1));
 angle2=atan2((lon2-lonm)*cos(latm),(lat2-latm));

 angle=angle2-angle1;

 angle=radians_to_degrees(angle);

 angle=round(angle);

 if(angle<-180) angle+=360;
 if(angle> 180) angle-=360;

 return((int)angle);
}


/*++++++++++++++++++++++++++++++++++++++
  Calculate the bearing of a segment from the given node.

  int bearing_angle Returns a value in the range 0 to 359 indicating the bearing.

  Nodes *nodes The set of nodes.

  Segment *segment The segment.

  index_t node The node to start.

  Angles are calculated using flat Cartesian lat/long grid approximation (after scaling longitude due to latitude).
  ++++++++++++++++++++++++++++++++++++++*/

static int bearing_angle(Nodes *nodes,Segment *segment,index_t node)
{
 double lat1,lat2;
 double lon1,lon2;
 double angle;
 index_t node1,node2;

 node1=node;
 node2=OtherNode(segment,node);

 if(IsFakeNode(node1))
    GetFakeLatLong(node1,&lat1,&lon1);
 else
    GetLatLong(nodes,node1,&lat1,&lon1);

 if(IsFakeNode(node2))
    GetFakeLatLong(node2,&lat2,&lon2);
 else
    GetLatLong(nodes,node2,&lat2,&lon2);

 angle=atan2((lat2-lat1),(lon2-lon1)*cos(lat1));

 angle=radians_to_degrees(angle);

 angle=round(270-angle);

 if(angle<  0) angle+=360;
 if(angle>360) angle-=360;

 return((int)angle);
}