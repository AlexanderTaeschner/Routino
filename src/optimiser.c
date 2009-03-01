/***************************************
 $Header: /home/amb/CVS/routino/src/optimiser.c,v 1.55 2009-03-01 17:24:21 amb Exp $

 Routing optimiser.
 ******************/ /******************
 Written by Andrew M. Bishop

 This file Copyright 2008,2009 Andrew M. Bishop
 It may be distributed under the GNU Public License, version 2, or
 any higher version.  See section COPYING of the GNU Public license
 for conditions under which this file may be redistributed.
 ***************************************/


#include <string.h>
#include <stdio.h>

#include "types.h"
#include "functions.h"
#include "nodes.h"
#include "segments.h"
#include "ways.h"
#include "results.h"


/*+ The option not to print any progress information. +*/
extern int option_quiet;

/*+ The option to calculate the quickest route insted of the shortest. +*/
extern int option_quickest;


/*++++++++++++++++++++++++++++++++++++++
  Find the optimum route between two nodes.

  Results *FindRoute Returns a set of results.

  Nodes *nodes The set of nodes to use.

  Segments *segments The set of segments to use.

  Ways *ways The set of ways to use.

  index_t start The start node.

  index_t finish The finish node.

  Profile *profile The profile containing the transport type, speeds and allowed highways.

  int all A flag to indicate that a big results structure is required.
  ++++++++++++++++++++++++++++++++++++++*/

Results *FindRoute(Nodes *nodes,Segments *segments,Ways *ways,index_t start,index_t finish,Profile *profile,int all)
{
 Results *results;
 index_t node1,node2;
 distance_t finish_distance;
 duration_t finish_duration;
 float finish_lat,finish_lon;
 speed_t max_speed=0;
 Result *result1,*result2;
 Segment *segment;
 Way *way;
 int i;

 /* Set up the finish conditions */

 finish_distance=~0;
 finish_duration=~0;

 GetLatLong(nodes,LookupNode(nodes,finish),&finish_lat,&finish_lon);

 for(i=0;i<sizeof(profile->speed)/sizeof(profile->speed[0]);i++)
    if(profile->speed[i]>max_speed)
       max_speed=profile->speed[i];

 /* Insert the first node into the queue */

 if(all)
    results=NewResultsList(65536);
 else
    results=NewResultsList(8);

 result1=InsertResult(results,start);

 result1->node=start;
 result1->prev=0;
 result1->next=0;
 result1->distance=0;
 result1->duration=0;
 result1->sortby=0;

 insert_in_queue(result1);

 /* Loop across all nodes in the queue */

 while((result1=pop_from_queue()))
   {
    if((option_quickest==0 && result1->sortby>finish_distance) ||
       (option_quickest==1 && result1->sortby>finish_duration))
       continue;

    node1=result1->node;

    segment=FirstSegment(segments,LookupNode(nodes,node1));

    while(segment)
      {
       distance_t cumulative_distance;
       duration_t cumulative_duration;

       if(!IsNormalSegment(segment))
          goto endloop;

       if(profile->oneway && IsOnewayTo(segment,node1))
          goto endloop;

       node2=OtherNode(segment,node1);

       if(result1->prev==node2)
          goto endloop;

       way=LookupWay(ways,segment->way);

       if(!(way->allow&profile->allow))
          goto endloop;

       if(!profile->highways[HIGHWAY(way->type)])
          goto endloop;

       if(way->weight<profile->weight)
          goto endloop;

       if(way->height<profile->height || way->width<profile->width || way->length<profile->length)
          goto endloop;

       cumulative_distance=result1->distance+DISTANCE(segment->distance);
       cumulative_duration=result1->duration+Duration(segment,way,profile);

       if((option_quickest==0 && cumulative_distance>finish_distance) ||
          (option_quickest==1 && cumulative_duration>finish_duration))
          goto endloop;

       result2=FindResult(results,node2);

       if(!result2)                         /* New end node */
         {
          result2=InsertResult(results,node2);
          result2->node=node2;
          result2->prev=node1;
          result2->next=0;
          result2->distance=cumulative_distance;
          result2->duration=cumulative_duration;

          if(node2==finish)
            {
             finish_distance=cumulative_distance;
             finish_duration=cumulative_duration;
            }
          else if(!all)
            {
             result2->sortby=result2->distance;
             insert_in_queue(result2);
            }
          else
            {
             float lat,lon;
             GetLatLong(nodes,LookupNode(nodes,node2),&lat,&lon);

             if(option_quickest==0)
                result2->sortby=result2->distance+Distance(lat,lon,finish_lat,finish_lon);
             else
                result2->sortby=result2->duration+distance_speed_to_duration(Distance(lat,lon,finish_lat,finish_lon),max_speed);

             insert_in_queue(result2);
            }
         }
       else if(option_quickest==0) /* shortest */
         {
          if(cumulative_distance<result2->distance ||
             (cumulative_distance==result2->distance &&
              cumulative_duration<result2->duration)) /* New end node is shorter */
            {
             result2->prev=node1;
             result2->distance=cumulative_distance;
             result2->duration=cumulative_duration;

             if(node2==finish)
               {
                finish_distance=cumulative_distance;
                finish_duration=cumulative_duration;
               }
             else if(!all)
               {
                result2->sortby=result2->distance;
                insert_in_queue(result2);
               }
             else
               {
                float lat,lon;
                GetLatLong(nodes,LookupNode(nodes,node2),&lat,&lon);

                result2->sortby=result2->distance+Distance(lat,lon,finish_lat,finish_lon);
                insert_in_queue(result2);
               }
            }
         }
       else if(option_quickest==1) /* quickest */
         {
          if(cumulative_duration<result2->duration ||
             (cumulative_duration==result2->duration &&
              cumulative_distance<result2->distance)) /* New end node is quicker */
            {
             result2->prev=node1;
             result2->distance=cumulative_distance;
             result2->duration=cumulative_duration;

             if(node2==finish)
               {
                finish_distance=cumulative_distance;
                finish_duration=cumulative_duration;
               }
             else if(!all)
               {
                result2->sortby=result2->distance;
                insert_in_queue(result2);
               }
             else
               {
                float lat,lon;
                GetLatLong(nodes,LookupNode(nodes,node2),&lat,&lon);

                result2->sortby=result2->duration+distance_speed_to_duration(Distance(lat,lon,finish_lat,finish_lon),max_speed);
                insert_in_queue(result2);
               }
            }
         }

      endloop:

       if(!option_quiet && !(results->number%10000))
         {
          printf("\rRouting: End Nodes=%d %.1fkm %.0fmin  ",results->number,
                 distance_to_km(result1->distance),duration_to_minutes(result1->duration));
          fflush(stdout);
         }

       segment=NextSegment(segments,segment,node1);
      }
   }

 if(!option_quiet)
   {
    printf("\rRouted: End Nodes=%d %.1fkm %.0fmin \n",results->number,
           distance_to_km(finish_distance),duration_to_minutes(finish_duration));
    fflush(stdout);
   }

 /* Check it worked */

 result2=FindResult(results,finish);

 if(!result2)
   {
    FreeResultsList(results);
    return(NULL);
   }

 /* Reverse the results */

 result2=FindResult(results,finish);

 do
   {
    if(result2->prev)
      {
       index_t node1=result2->prev;

       result1=FindResult(results,node1);

       result1->next=result2->node;

       result2=result1;
      }
    else
       result2=NULL;
   }
 while(result2);

 return(results);
}


/*++++++++++++++++++++++++++++++++++++++
  Find the optimum route between two nodes.

  Results *FindRoute3 Returns a set of results.

  Nodes *nodes The set of nodes to use.

  Segments *segments The set of segments to use.

  Ways *ways The set of ways to use.

  index_t start The start node.

  index_t finish The finish node.

  Results *begin The initial portion of the route.

  Results *end The final portion of the route.

  Profile *profile The profile containing the transport type, speeds and allowed highways.
  ++++++++++++++++++++++++++++++++++++++*/

Results *FindRoute3(Nodes *nodes,Segments *segments,Ways *ways,index_t start,index_t finish,Results *begin,Results *end,Profile *profile)
{
 Results *results;
 index_t node1,node2;
 distance_t finish_distance;
 duration_t finish_duration;
 float finish_lat,finish_lon;
 speed_t max_speed=0;
 Result *result1,*result2,*result3;
 Segment *segment;
 Way *way;
 int i;

 /* Set up the finish conditions */

 finish_distance=~0;
 finish_duration=~0;

 GetLatLong(nodes,LookupNode(nodes,finish),&finish_lat,&finish_lon);

 for(i=0;i<sizeof(profile->speed)/sizeof(profile->speed[0]);i++)
    if(profile->speed[i]>max_speed)
       max_speed=profile->speed[i];

 /* Insert the start node */

 results=NewResultsList(65536);

 result1=InsertResult(results,start);
 result3=FindResult(begin,start);

 *result1=*result3;

 /* Insert the finish points of the beginning part of the path into the queue */

 result3=FirstResult(begin);

 while(result3)
   {
    if(IsSuperNode(LookupNode(nodes,result3->node)))
      {
       if(!(result2=FindResult(results,result3->node)))
         {
          result2=InsertResult(results,result3->node);

          *result2=*result3;

          result2->prev=start;

          result2->sortby=result2->distance;
         }

       insert_in_queue(result2);
      }

    result3=NextResult(begin,result3);
   }

 /* Loop across all nodes in the queue */

 while((result1=pop_from_queue()))
   {
    if((option_quickest==0 && result1->sortby>finish_distance) ||
       (option_quickest==1 && result1->sortby>finish_duration))
       continue;

    node1=result1->node;

    segment=FirstSegment(segments,LookupNode(nodes,node1));

    while(segment)
      {
       distance_t cumulative_distance;
       duration_t cumulative_duration;

       if(!IsSuperSegment(segment))
          goto endloop;

       if(profile->oneway && IsOnewayTo(segment,node1))
          goto endloop;

       node2=OtherNode(segment,node1);

       if(result1->prev==node2)
          goto endloop;

       way=LookupWay(ways,segment->way);

       if(!(way->allow&profile->allow))
          goto endloop;

       if(!profile->highways[HIGHWAY(way->type)])
          goto endloop;

       if(way->weight<profile->weight)
          goto endloop;

       if(way->height<profile->height || way->width<profile->width || way->length<profile->length)
          goto endloop;

       cumulative_distance=result1->distance+DISTANCE(segment->distance);
       cumulative_duration=result1->duration+Duration(segment,way,profile);

       if((option_quickest==0 && cumulative_distance>finish_distance) ||
          (option_quickest==1 && cumulative_duration>finish_duration))
          goto endloop;

       result2=FindResult(results,node2);

       if(!result2)                         /* New end node */
         {
          result2=InsertResult(results,node2);
          result2->node=node2;
          result2->prev=node1;
          result2->next=0;
          result2->distance=cumulative_distance;
          result2->duration=cumulative_duration;

          if((result3=FindResult(end,node2)))
            {
             finish_distance=cumulative_distance+result3->distance;
             finish_duration=cumulative_duration+result3->duration;
            }
          else
            {
             float lat,lon;
             GetLatLong(nodes,LookupNode(nodes,node2),&lat,&lon);

             if(option_quickest==0)
                result2->sortby=result2->distance+Distance(lat,lon,finish_lat,finish_lon);
             else
                result2->sortby=result2->duration+distance_speed_to_duration(Distance(lat,lon,finish_lat,finish_lon),max_speed);

             insert_in_queue(result2);
            }
         }
       else if(option_quickest==0) /* shortest */
         {
          if(cumulative_distance<result2->distance ||
             (cumulative_distance==result2->distance &&
              cumulative_duration<result2->duration)) /* New end node is shorter */
            {
             result2->prev=node1;
             result2->distance=cumulative_distance;
             result2->duration=cumulative_duration;

             if((result3=FindResult(end,node2)))
               {
                if((cumulative_distance+result3->distance)<finish_distance ||
                   ((cumulative_distance+result3->distance)==finish_distance &&
                    (cumulative_duration+result3->duration)<finish_duration))
                  {
                   finish_distance=cumulative_distance+result3->distance;
                   finish_duration=cumulative_duration+result3->duration;
                  }
               }
             else
               {
                float lat,lon;
                GetLatLong(nodes,LookupNode(nodes,node2),&lat,&lon);

                result2->sortby=result2->distance+Distance(lat,lon,finish_lat,finish_lon);
                insert_in_queue(result2);
               }
            }
         }
       else if(option_quickest==1) /* quickest */
         {
          if(cumulative_duration<result2->duration ||
             (cumulative_duration==result2->duration &&
              cumulative_distance<result2->distance)) /* New end node is quicker */
            {
             result2->prev=node1;
             result2->distance=cumulative_distance;
             result2->duration=cumulative_duration;

             if((result3=FindResult(end,node2)))
               {
                if((cumulative_duration+result3->duration)<finish_duration ||
                   ((cumulative_duration+result3->duration)==finish_duration &&
                    (cumulative_distance+result3->distance)<finish_distance))
                  {
                   finish_distance=cumulative_distance+result3->distance;
                   finish_duration=cumulative_duration+result3->duration;
                  }
               }
             else
               {
                float lat,lon;
                GetLatLong(nodes,LookupNode(nodes,node2),&lat,&lon);

                result2->sortby=result2->duration+distance_speed_to_duration(Distance(lat,lon,finish_lat,finish_lon),max_speed);
                insert_in_queue(result2);
               }
            }
         }

      endloop:

       if(!option_quiet && !(results->number%10000))
         {
          printf("\rRouting: End Nodes=%d %.1fkm %.0fmin  ",results->number,
                 distance_to_km(result1->distance),duration_to_minutes(result1->duration));
          fflush(stdout);
         }

       segment=NextSegment(segments,segment,node1);
      }
   }

 if(!option_quiet)
   {
    printf("\rRouted: End Super-Nodes=%d %.1fkm %.0fmin \n",results->number,
           distance_to_km(finish_distance),duration_to_minutes(finish_duration));
    fflush(stdout);
   }

 /* Finish off the end part of the route. */

 if(!FindResult(results,finish))
   {
    result2=InsertResult(results,finish);
    result3=FindResult(end,finish);

    *result2=*result3;

    result2->distance=~0;
    result2->duration=~0;

    result3=FirstResult(end);

    while(result3)
      {
       if(IsSuperNode(LookupNode(nodes,result3->node)))
          if((result1=FindResult(results,result3->node)))
            {
             if(option_quickest==0) /* shortest */
               {
                if((result1->distance+result3->distance)<result2->distance ||
                   ((result1->distance+result3->distance)==result2->distance &&
                    (result1->duration+result3->duration)<result2->duration))
                  {
                   result2->distance=result1->distance+result3->distance;
                   result2->duration=result1->duration+result3->duration;
                   result2->prev=result1->node;
                  }
               }
             else
               {
                if((result1->duration+result3->duration)<result2->duration ||
                   ((result1->duration+result3->duration)==result2->duration &&
                    (result1->distance+result3->distance)<result2->distance))
                  {
                   result2->distance=result1->distance+result3->distance;
                   result2->duration=result1->duration+result3->duration;
                   result2->prev=result1->node;
                  }
               }
            }

       result3=NextResult(end,result3);
      }
   }

 /* Check it worked */

 if(finish_distance == ~0)
   {
    FreeResultsList(results);
    return(NULL);
   }

 /* Reverse the results */

 result2=FindResult(results,finish);

 do
   {
    if(result2->prev)
      {
       index_t node1=result2->prev;

       result1=FindResult(results,node1);

       result1->next=result2->node;

       result2=result1;
      }
    else
       result2=NULL;
   }
 while(result2);

 return(results);
}


/*++++++++++++++++++++++++++++++++++++++
  Print the optimum route between two nodes.

  Results *Results The set of results to print.

  Nodes *nodes The list of nodes.

  Segments *segments The set of segments to use.

  Ways *ways The list of ways.

  index_t start The start node.

  index_t finish The finish node.

  Profile *profile The profile containing the transport type, speeds and allowed highways.
  ++++++++++++++++++++++++++++++++++++++*/

void PrintRoute(Results *results,Nodes *nodes,Segments *segments,Ways *ways,index_t start,index_t finish,Profile *profile)
{
 FILE *textfile,*gpxfile;
 Result *result;

 if(option_quickest==0)
   {
    /* Print the result for the shortest route */

    textfile=fopen("shortest.txt","w");
    gpxfile=fopen("shortest.gpx","w");
   }
 else
   {
    /* Print the result for the quickest route */

    textfile=fopen("quickest.txt","w");
    gpxfile=fopen("quickest.gpx","w");
   }

 fprintf(gpxfile,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
 fprintf(gpxfile,"<gpx version=\"1.0\" creator=\"Router\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n");
 fprintf(gpxfile,"<trk>\n");
 fprintf(gpxfile,"<trkseg>\n");

 result=FindResult(results,start);

 do
   {
    float latitude,longitude;
    Node *node=LookupNode(nodes,result->node);

    GetLatLong(nodes,node,&latitude,&longitude);

    fprintf(gpxfile,"<trkpt lat=\"%.6f\" lon=\"%.6f\"></trkpt>\n",
            (180/M_PI)*latitude,(180/M_PI)*longitude);

    if(result->prev)
      {
       Segment *segment;
       Way *way;

       segment=FirstSegment(segments,LookupNode(nodes,result->prev));
       while(OtherNode(segment,result->prev)!=result->node)
          segment=NextSegment(segments,segment,result->prev);

       way=LookupWay(ways,segment->way);

       fprintf(textfile,"%8.4f %9.4f %8d%c %5.3f %5.2f %7.2f %5.1f %3d %s\n",
               (180/M_PI)*latitude,(180/M_PI)*longitude,
               result->node,IsSuperNode(node)?'*':' ',
               distance_to_km(DISTANCE(segment->distance)),duration_to_minutes(Duration(segment,way,profile)),
               distance_to_km(result->distance),duration_to_minutes(result->duration),
               profile->speed[HIGHWAY(way->type)],WayName(ways,way));
      }
    else
       fprintf(textfile,"%8.4f %9.4f %8d%c %5.3f %5.2f %7.2f %5.1f\n",
               (180/M_PI)*latitude,(180/M_PI)*longitude,
               result->node,IsSuperNode(node)?'*':' ',
               0.0,0.0,0.0,0.0);

    if(result->next)
       result=FindResult(results,result->next);
    else
       result=NULL;
   }
    while(result);

 fprintf(gpxfile,"</trkseg>\n");
 fprintf(gpxfile,"</trk>\n");
 fprintf(gpxfile,"</gpx>\n");

 fclose(textfile);
 fclose(gpxfile);
}


/*++++++++++++++++++++++++++++++++++++++
  Find all routes from a specified node to any super-node.

  Results *FindStartRoutes Returns a set of results.

  Nodes *nodes The set of nodes to use.

  Segments *segments The set of segments to use.

  Ways *ways The set of ways to use.

  index_t start The start node.

  Profile *profile The profile containing the transport type, speeds and allowed highways.
  ++++++++++++++++++++++++++++++++++++++*/

Results *FindStartRoutes(Nodes *nodes,Segments *segments,Ways *ways,index_t start,Profile *profile)
{
 Results *results;
 index_t node1,node2;
 Result *result1,*result2;
 Segment *segment;
 Way *way;

 /* Insert the first node into the queue */

 results=NewResultsList(8);

 result1=InsertResult(results,start);

 result1->node=start;
 result1->prev=0;
 result1->next=0;
 result1->distance=0;
 result1->duration=0;
 result1->sortby=0;

 insert_in_queue(result1);

 /* Loop across all nodes in the queue */

 while((result1=pop_from_queue()))
   {
    node1=result1->node;

    segment=FirstSegment(segments,LookupNode(nodes,node1));

    while(segment)
      {
       distance_t cumulative_distance;
       duration_t cumulative_duration;

       if(!IsNormalSegment(segment))
          goto endloop;

       if(profile->oneway && IsOnewayTo(segment,node1))
          goto endloop;

       node2=OtherNode(segment,node1);

       if(result1->prev==node2)
          goto endloop;

       way=LookupWay(ways,segment->way);

       if(!(way->allow&profile->allow))
          goto endloop;

       if(!profile->highways[HIGHWAY(way->type)])
          goto endloop;

       if(way->weight<profile->weight)
          goto endloop;

       if(way->height<profile->height || way->width<profile->width || way->length<profile->length)
          goto endloop;

       cumulative_distance=result1->distance+DISTANCE(segment->distance);
       cumulative_duration=result1->duration+Duration(segment,way,profile);

       result2=FindResult(results,node2);

       if(!result2)                         /* New end node */
         {
          result2=InsertResult(results,node2);
          result2->node=node2;
          result2->prev=node1;
          result2->next=0;
          result2->distance=cumulative_distance;
          result2->duration=cumulative_duration;

          if(!IsSuperNode(LookupNode(nodes,node2)))
            {
             result2->sortby=result2->distance;
             insert_in_queue(result2);
            }
         }
       else if(option_quickest==0) /* shortest */
         {
          if(cumulative_distance<result2->distance ||
             (cumulative_distance==result2->distance &&
              cumulative_duration<result2->duration)) /* New end node is shorter */
            {
             result2->prev=node1;
             result2->distance=cumulative_distance;
             result2->duration=cumulative_duration;

             if(!IsSuperNode(LookupNode(nodes,node2)))
               {
                result2->sortby=result2->distance;
                insert_in_queue(result2);
               }
            }
         }
       else if(option_quickest==1) /* quickest */
         {
          if(cumulative_duration<result2->duration ||
             (cumulative_duration==result2->duration &&
              cumulative_distance<result2->distance)) /* New end node is quicker */
            {
             result2->prev=node1;
             result2->distance=cumulative_distance;
             result2->duration=cumulative_duration;

             if(!IsSuperNode(LookupNode(nodes,node2)))
               {
                result2->sortby=result2->duration;
                insert_in_queue(result2);
               }
            }
         }

      endloop:

       segment=NextSegment(segments,segment,node1);
      }
   }

 /* Check it worked */

 if(results->number==1)
   {
    FreeResultsList(results);
    return(NULL);
   }

 return(results);
}


/*++++++++++++++++++++++++++++++++++++++
  Find all routes from any super-node to a specific node.

  Results *FindFinishRoutes Returns a set of results.

  Nodes *nodes The set of nodes to use.

  Segments *segments The set of segments to use.

  Ways *ways The set of ways to use.

  index_t finish The finishing node.

  Profile *profile The profile containing the transport type, speeds and allowed highways.
  ++++++++++++++++++++++++++++++++++++++*/

Results *FindFinishRoutes(Nodes *nodes,Segments *segments,Ways *ways,index_t finish,Profile *profile)
{
 Results *results;
 index_t node1,node2;
 Result *result1,*result2;
 Segment *segment;
 Way *way;

 /* Insert the first node into the queue */

 results=NewResultsList(8);

 result1=InsertResult(results,finish);

 result1->node=finish;
 result1->prev=0;
 result1->next=0;
 result1->distance=0;
 result1->duration=0;
 result1->sortby=0;

 insert_in_queue(result1);

 /* Loop across all nodes in the queue */

 while((result1=pop_from_queue()))
   {
    node1=result1->node;

    segment=FirstSegment(segments,LookupNode(nodes,node1));

    while(segment)
      {
       distance_t cumulative_distance;
       duration_t cumulative_duration;

       if(!IsNormalSegment(segment))
          goto endloop;

       if(profile->oneway && IsOnewayFrom(segment,node1))
          goto endloop;

       node2=OtherNode(segment,node1);

       if(result1->next==node2)
          goto endloop;

       way=LookupWay(ways,segment->way);

       if(!(way->allow&profile->allow))
          goto endloop;

       if(!profile->highways[HIGHWAY(way->type)])
          goto endloop;

       if(way->weight<profile->weight)
          goto endloop;

       if(way->height<profile->height || way->width<profile->width || way->length<profile->length)
          goto endloop;

       cumulative_distance=result1->distance+DISTANCE(segment->distance);
       cumulative_duration=result1->duration+Duration(segment,way,profile);

       result2=FindResult(results,node2);

       if(!result2)                         /* New end node */
         {
          result2=InsertResult(results,node2);
          result2->node=node2;
          result2->prev=0;
          result2->next=node1;
          result2->distance=cumulative_distance;
          result2->duration=cumulative_duration;

          if(!IsSuperNode(LookupNode(nodes,node2)))
            {
             result2->sortby=result2->distance;
             insert_in_queue(result2);
            }
         }
       else if(option_quickest==0) /* shortest */
         {
          if(cumulative_distance<result2->distance ||
             (cumulative_distance==result2->distance &&
              cumulative_duration<result2->duration)) /* New end node is shorter */
            {
             result2->next=node1;
             result2->distance=cumulative_distance;
             result2->duration=cumulative_duration;

             if(!IsSuperNode(LookupNode(nodes,node2)))
               {
                result2->sortby=result2->distance;
                insert_in_queue(result2);
               }
            }
         }
       else if(option_quickest==1) /* quickest */
         {
          if(cumulative_duration<result2->duration ||
             (cumulative_duration==result2->duration &&
              cumulative_distance<result2->distance)) /* New end node is quicker */
            {
             result2->next=node1;
             result2->distance=cumulative_distance;
             result2->duration=cumulative_duration;

             if(!IsSuperNode(LookupNode(nodes,node2)))
               {
                result2->sortby=result2->duration;
                insert_in_queue(result2);
               }
            }
         }

      endloop:

       segment=NextSegment(segments,segment,node1);
      }
   }

 /* Check it worked */

 if(results->number==1)
   {
    FreeResultsList(results);
    return(NULL);
   }

 return(results);
}


/*++++++++++++++++++++++++++++++++++++++
  Create an optimum route given the set of super-nodes to follow.

  Results *CombineRoutes Returns the results from joining the super-nodes.

  Results *results The set of results from the super-nodes.

  Nodes *nodes The list of nodes.

  Segments *segments The set of segments to use.

  Ways *ways The list of ways.

  index_t start The start node.

  index_t finish The finish node.

  Profile *profile The profile containing the transport type, speeds and allowed highways.
  ++++++++++++++++++++++++++++++++++++++*/

Results *CombineRoutes(Results *results,Nodes *nodes,Segments *segments,Ways *ways,index_t start,index_t finish,Profile *profile)
{
 Result *result1,*result2,*result3,*result4;
 Results *combined;
 int quiet=option_quiet;

 combined=NewResultsList(64);

 option_quiet=1;

 /* Sort out the combined route */

 result1=FindResult(results,start);

 result3=InsertResult(combined,start);

 result3->node=result1->node;

 result3->distance=0;
 result3->duration=0;
 result3->next=0;
 result3->prev=0;

 do
   {
    if(result1->next)
      {
       Results *results2=FindRoute(nodes,segments,ways,result1->node,result1->next,profile,0);

       result2=FindResult(results2,result1->node);

       result3->next=result2->next;

       result2=FindResult(results2,result2->next);

       do
         {
          result4=InsertResult(combined,result2->node);

          result4->node=result2->node;

          *result4=*result2;
          result4->distance+=result3->distance;
          result4->duration+=result3->duration;

          if(result2->next)
             result2=FindResult(results2,result2->next);
          else
             result2=NULL;
         }
       while(result2);

       FreeResultsList(results2);

       result1=FindResult(results,result1->next);

       result3=result4;
      }
    else
       result1=NULL;
   }
 while(result1);

 /* Now print out the result normally */

 option_quiet=quiet;

 return(combined);
}
