/***************************************
 $Header: /home/amb/CVS/routino/src/segments.h,v 1.25 2009-02-04 18:23:33 amb Exp $

 A header file for the segments.
 ******************/ /******************
 Written by Andrew M. Bishop

 This file Copyright 2009 Andrew M. Bishop
 It may be distributed under the GNU Public License, version 2, or
 any higher version.  See section COPYING of the GNU Public license
 for conditions under which this file may be redistributed.
 ***************************************/


#ifndef SEGMENTS_H
#define SEGMENTS_H    /*+ To stop multiple inclusions. +*/

#include <stdint.h>

#include "types.h"
#include "nodes.h"
#include "profiles.h"


/* Data structures */


/*+ An extended structure used for processing. +*/
typedef struct _SegmentX
{
 node_t       node1;            /*+ The starting node. +*/
 node_t       node2;            /*+ The finishing node. +*/

 Segment      segment;          /*+ The real segment data. +*/
}
 SegmentX;

/*+ A structure containing a set of segments (mmap format). +*/
typedef struct _Segments
{
 uint32_t number;               /*+ How many entries are used in total? +*/

 Segment *segments;             /*+ An array of segments. +*/

 void    *data;                 /*+ The memory mapped data. +*/
}
 Segments;

/*+ A structure containing a set of segments (memory format). +*/
typedef struct _SegmentsX
{
 uint32_t  sorted;              /*+ Is the data sorted and therefore searchable? +*/
 uint32_t  alloced;             /*+ How many entries are allocated? +*/
 uint32_t  number;              /*+ How many entries are used? +*/

 SegmentX *xdata;               /*+ The extended segment data. +*/
}
 SegmentsX;


/* Functions in segments.c */


Segments *LoadSegmentList(const char *filename);

SegmentsX *NewSegmentList(void);
void FreeSegmentList(SegmentsX *segmentsx);

void SaveSegmentList(SegmentsX *segmentsx,const char *filename);

SegmentX *FindFirstSegmentX(SegmentsX* segmentsx,node_t node);
SegmentX *FindNextSegmentX(SegmentsX* segmentsx,SegmentX *segmentx);

Segment *NextSegment(Segments *segments,Segment *segment);

Segment *AppendSegment(SegmentsX* segmentsx,node_t node1,node_t node2);

void SortSegmentList(SegmentsX *segmentsx);

void RemoveBadSegments(SegmentsX *segmentsx);

void MeasureSegments(SegmentsX *segmentsx,NodesX *nodesx);
void FixupSegments(SegmentsX* segmentsx,NodesX *nodesx,SegmentsX* supersegmentsx);

distance_t DistanceX(NodeX *nodex1,NodeX *nodex2);

float Distance(float lat1,float lon1,float lat2,float lon2);

duration_t Duration(Segment *segment,Way *way,Profile *profile);

#define LookupSegmentX(xxx,yyy) (&(xxx)->xdata[yyy])

#define IndexSegmentX(xxx,yyy)  ((yyy)-&(xxx)->xdata[0])


#endif /* SEGMENTS_H */
