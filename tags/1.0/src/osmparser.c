/***************************************
 $Header: /home/amb/CVS/routino/src/osmparser.c,v 1.34 2009-04-08 16:54:34 amb Exp $

 OSM XML file parser (either JOSM or planet)

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
#include <ctype.h>
#include <math.h>

#include "types.h"
#include "functions.h"
#include "nodesx.h"
#include "segmentsx.h"
#include "waysx.h"


#define BUFFSIZE 64

static char *fgets_realloc(char *buffer,FILE *file);


/*++++++++++++++++++++++++++++++++++++++
  Parse an OSM XML file (from JOSM or planet download).

  int ParseXML Returns 0 if OK or something else in case of an error.

  FILE *file The file to read from.

  NodesX *OSMNodes The array of nodes to fill in.

  SegmentsX *OSMSegments The array of segments to fill in.

  WaysX *OSMWays The arrray of ways to fill in.

  Profile profile A profile of the allowed transport types and included/excluded highway types.
  ++++++++++++++++++++++++++++++++++++++*/

int ParseXML(FILE *file,NodesX *OSMNodes,SegmentsX *OSMSegments,WaysX *OSMWays,Profile *profile)
{
 char *line=NULL;
 long nlines=0;
 long nnodes=0,nways=0,nrelations=0;
 int isnode=0,isway=0,isrelation=0;
 int way_oneway=0,way_roundabout=0;
 speed_t way_maxspeed=0;
 weight_t way_maxweight=0;
 height_t way_maxheight=0;
 width_t way_maxwidth=0;
 length_t way_maxlength=0;
 char *way_highway=NULL,*way_name=NULL,*way_ref=NULL;
 wayallow_t way_allow_no=0,way_allow_yes=0;
 node_t *way_nodes=NULL;
 int way_nnodes=0,way_nalloc=0;

 /* Parse the file */

 while((line=fgets_realloc(line,file)))
   {
    char *l=line,*m;

    nlines++;

    while(isspace(*l))
       l++;

    if(!strncmp(l,"<node",5)) /* The start of a node */
      {
       node_t id;
       float latitude,longitude;

       nnodes++;

       isnode=1; isway=0; isrelation=0;

       m=strstr(l,"id=");  m+=4; if(*m=='"' || *m=='\'') m++; id=atoll(m);
       m=strstr(l,"lat="); m+=5; if(*m=='"' || *m=='\'') m++; latitude=(M_PI/180)*atof(m);
       m=strstr(l,"lon="); m+=4; if(*m=='"' || *m=='\'') m++; longitude=(M_PI/180)*atof(m);

       AppendNode(OSMNodes,id,latitude,longitude);
      }
    else if(!strncmp(l,"</node",6)) /* The end of a node */
      {
       isnode=0; isway=0; isrelation=0;
      }
    else if(!strncmp(l,"<way",4)) /* The start of a way */
      {
       nways++;

       isnode=0; isway=1; isrelation=0;

       way_oneway=0; way_roundabout=0;
       way_maxspeed=0; way_maxweight=0; way_maxheight=0; way_maxwidth=0;
       way_maxlength=0;
       way_highway=NULL; way_name=NULL; way_ref=NULL;
       way_allow_no=0; way_allow_yes=0;
       way_nnodes=0;
      }
    else if(!strncmp(l,"</way",5)) /* The end of a way */
      {
       isnode=0; isway=0; isrelation=0;

       if(way_highway)
         {
          waytype_t type;
          wayallow_t allow;

          type=HighwayType(way_highway);

          switch(type)
            {
            case Way_Motorway:
             allow=Allow_Motorbike|Allow_Motorcar|Allow_PSV|Allow_Goods|Allow_HGV;
             break;
            case Way_Trunk:
             allow=Allow_Bicycle|Allow_Motorbike|Allow_Motorcar|Allow_PSV|Allow_Goods|Allow_HGV;
             break;
            case Way_Primary:
             allow=Allow_Foot|Allow_Bicycle|Allow_Horse|Allow_Motorbike|Allow_Motorcar|Allow_PSV|Allow_Goods|Allow_HGV;
             break;
            case Way_Secondary:
             allow=Allow_Foot|Allow_Bicycle|Allow_Horse|Allow_Motorbike|Allow_Motorcar|Allow_PSV|Allow_Goods|Allow_HGV;
             break;
            case Way_Tertiary:
             allow=Allow_Foot|Allow_Bicycle|Allow_Horse|Allow_Motorbike|Allow_Motorcar|Allow_PSV|Allow_Goods|Allow_HGV;
             break;
            case Way_Unclassified:
             allow=Allow_Foot|Allow_Bicycle|Allow_Horse|Allow_Motorbike|Allow_Motorcar|Allow_PSV|Allow_Goods|Allow_HGV;
             break;
            case Way_Residential:
             allow=Allow_Foot|Allow_Bicycle|Allow_Horse|Allow_Motorbike|Allow_Motorcar|Allow_PSV|Allow_Goods|Allow_HGV;
             break;
            case Way_Service:
             allow=Allow_Foot|Allow_Bicycle|Allow_Horse|Allow_Motorbike|Allow_Motorcar|Allow_PSV|Allow_Goods|Allow_HGV;
             break;
            case Way_Track:
             allow=Allow_Foot|Allow_Bicycle|Allow_Horse;
             break;
            case Way_Path:
             allow=Allow_Foot; /* Only allow bicycle and horse if so indicated. */
             break;
            case Way_Bridleway:
             allow=Allow_Foot|Allow_Bicycle|Allow_Horse;
             break;
            case Way_Cycleway:
             allow=Allow_Foot|Allow_Bicycle;
             break;
            case Way_Footway:
             allow=Allow_Foot;
             break;
            default:
             allow=0;
             break;
            }

          if(way_allow_no)      /* Remove the ones explicitly denied (e.g. private) */
             allow&=~way_allow_no;

          if(way_allow_yes)     /* Add the ones explicitly allowed (e.g. footpath along private) */
             allow|=way_allow_yes;

          if(allow&profile->allow && profile->highways[HIGHWAY(type)])
            {
             Way *way;
             char *refname;
             int i;

             if(way_ref && way_name)
               {
                refname=(char*)malloc(strlen(way_ref)+strlen(way_name)+4);
                sprintf(refname,"%s (%s)",way_name,way_ref);
               }
             else if(way_ref && !way_name && way_roundabout)
               {
                refname=(char*)malloc(strlen(way_ref)+14);
                sprintf(refname,"%s (roundabout)",way_ref);
               }
             else if(way_ref && !way_name)
                refname=way_ref;
             else if(!way_ref && way_name)
                refname=way_name;
             else if(way_roundabout)
               {
                refname=(char*)malloc(strlen(way_highway)+14);
                sprintf(refname,"%s (roundabout)",way_highway);
               }
             else /* if(!way_ref && !way_name && !way_roundabout) */
                refname=way_highway;

             way=AppendWay(OSMWays,refname);

             way->speed=way_maxspeed;
             way->weight=way_maxweight;
             way->height=way_maxheight;
             way->width=way_maxwidth;
             way->length=way_maxlength;

             way->type=type;

             way->allow=allow;

             if(way_oneway)
                way->type|=Way_OneWay;

             if(way_roundabout)
                way->type|=Way_Roundabout;

             if(refname!=way_ref && refname!=way_name && refname!=way_highway)
                free(refname);

             for(i=1;i<way_nnodes;i++)
               {
                node_t from=way_nodes[i-1];
                node_t to  =way_nodes[i];
                Segment *segment;

                segment=AppendSegment(OSMSegments,from,to);
                segment->way=OSMWays->number-1;

                if(way_oneway>0)
                   segment->distance=ONEWAY_1TO2;
                else if(way_oneway<0)
                   segment->distance=ONEWAY_2TO1;

                segment=AppendSegment(OSMSegments,to,from);
                segment->way=OSMWays->number-1;

                if(way_oneway>0)
                   segment->distance=ONEWAY_2TO1;
                else if(way_oneway<0)
                   segment->distance=ONEWAY_1TO2;
               }
            }
         }

       if(way_highway) free(way_highway);
       if(way_name)    free(way_name);
       if(way_ref)     free(way_ref);
      }
    else if(!strncmp(l,"<relation",9)) /* The start of a relation */
      {
       nrelations++;

       isnode=0; isway=0; isrelation=1;
      }
    else if(!strncmp(l,"</relation",10)) /* The end of a relation */
      {
       isnode=0; isway=0; isrelation=0;
      }
    else if(isnode) /* The middle of a node */
      {
      }
    else if(isway) /* The middle of a way */
      {
       node_t id;

       if(!strncmp(l,"<nd",3)) /* The start of a node specifier */
         {
          m=strstr(l,"ref="); m+=4; if(*m=='"' || *m=='\'') m++; id=atoll(m);

          if(way_nnodes<=way_nalloc)
             way_nodes=(node_t*)realloc((void*)way_nodes,(way_nalloc+=16)*sizeof(node_t));

          way_nodes[way_nnodes++]=id;
         }

       if(!strncmp(l,"<tag",4)) /* The start of a tag specifier */
         {
          char delimiter,*k="",*v="";

          m=strstr(l,"k="); m+=2; delimiter=*m; m++; k=m;
          while(*m!=delimiter) m++; *m=0; l=m+1;

          m=strstr(l,"v="); m+=2; delimiter=*m; m++; v=m;
          while(*m!=delimiter) m++; *m=0;

          switch(*k)
            {
            case 'a':
             if(!strcmp(k,"access"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1") || !strcmp(v,"permissive") || !strcmp(v,"designated"))
                   ;
                else
                   way_allow_no=~0;
               }
             break;

            case 'b':
             if(!strcmp(k,"bicycle"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1") || !strcmp(v,"permissive") || !strcmp(v,"designated"))
                   way_allow_yes|=Allow_Bicycle;
                else
                   way_allow_no|=Allow_Bicycle;
               }
             break;

            case 'f':
             if(!strcmp(k,"foot"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1") || !strcmp(v,"permissive") || !strcmp(v,"designated"))
                   way_allow_yes|=Allow_Foot;
                else
                   way_allow_no|=Allow_Foot;
               }
             break;

            case 'g':
             if(!strcmp(k,"goods"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1") || !strcmp(v,"permissive") || !strcmp(v,"designated"))
                   way_allow_yes|=Allow_Goods;
                else
                   way_allow_no|=Allow_Goods;
               }
             break;

            case 'h':
             if(!strcmp(k,"highway"))
               {
                if(!strncmp(v,"motorway",8)) way_oneway=1;

                way_highway=strcpy((char*)malloc(strlen(v)+1),v);
               }
             if(!strcmp(k,"horse"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1") || !strcmp(v,"permissive") || !strcmp(v,"designated"))
                   way_allow_yes|=Allow_Horse;
                else
                   way_allow_no|=Allow_Horse;
               }
             if(!strcmp(k,"hgv"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1") || !strcmp(v,"permissive") || !strcmp(v,"designated"))
                   way_allow_yes|=Allow_HGV;
                else
                   way_allow_no|=Allow_HGV;
               }
             break;

            case 'j':
             if(!strcmp(k,"junction"))
                if(!strcmp(v,"roundabout"))
                  {way_oneway=1; way_roundabout=1;}
             break;

            case 'm':
             if(!strcmp(k,"maxspeed"))
               {
                if(strstr(v,"mph"))
                   way_maxspeed=kph_to_speed(1.6*atof(v));
                else
                   way_maxspeed=kph_to_speed(atof(v));
               }
             if(!strcmp(k,"maxweight"))
               {
                if(strstr(v,"kg"))
                   way_maxweight=tonnes_to_weight(atof(v)/1000);
                else
                   way_maxweight=tonnes_to_weight(atof(v));
               }
             if(!strcmp(k,"maxheight"))
               {
                if(strstr(v,"ft") || strstr(v,"feet"))
                   way_maxheight=metres_to_height(atof(v)*0.254);
                else
                   way_maxheight=metres_to_height(atof(v));
               }
             if(!strcmp(k,"maxwidth"))
               {
                if(strstr(v,"ft") || strstr(v,"feet"))
                   way_maxwidth=metres_to_width(atof(v)*0.254);
                else
                   way_maxwidth=metres_to_width(atof(v));
               }
             if(!strcmp(k,"maxlength"))
               {
                if(strstr(v,"ft") || strstr(v,"feet"))
                   way_maxlength=metres_to_length(atof(v)*0.254);
                else
                   way_maxlength=metres_to_length(atof(v));
               }
             if(!strcmp(k,"motorbike"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1") || !strcmp(v,"permissive") || !strcmp(v,"designated"))
                   way_allow_yes|=Allow_Motorbike;
                else
                   way_allow_no|=Allow_Motorbike;
               }
             if(!strcmp(k,"motorcar"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1") || !strcmp(v,"permissive") || !strcmp(v,"designated"))
                   way_allow_yes|=Allow_Motorcar;
                else
                   way_allow_no|=Allow_Motorcar;
               }
             break;

            case 'n':
             if(!strcmp(k,"name"))
                way_name=strcpy((char*)malloc(strlen(v)+1),v);
             break;

            case 'o':
             if(!strcmp(k,"oneway"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1"))
                   way_oneway=1;
                else if(!strcmp(v,"-1"))
                   way_oneway=-1;
               }
             break;

            case 'p':
             if(!strcmp(k,"psv"))
               {
                if(!strcmp(v,"true") || !strcmp(v,"yes") || !strcmp(v,"1") || !strcmp(v,"permissive") || !strcmp(v,"designated"))
                   way_allow_yes|=Allow_PSV;
                else
                   way_allow_no|=Allow_PSV;
               }
             break;

            case 'r':
             if(!strcmp(k,"ref"))
                way_ref=strcpy((char*)malloc(strlen(v)+1),v);
             break;

            default:
             ;
            }
         }
      }
    else if(isrelation) /* The middle of a relation */
      {
      }

    if(!(nlines%10000))
      {
       printf("\rReading: Lines=%ld Nodes=%ld Ways=%ld Relations=%ld",nlines,nnodes,nways,nrelations);
       fflush(stdout);
      }
   }

 printf("\rRead: Lines=%ld Nodes=%ld Ways=%ld Relations=%ld   \n",nlines,nnodes,nways,nrelations);
 fflush(stdout);

 if(line)
    free(line);

 if(way_nalloc)
    free(way_nodes);

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Call fgets and realloc the buffer as needed to get a whole line.

  char *fgets_realloc Returns the modified buffer (NULL at the end of the file).

  char *buffer The current buffer.

  FILE *file The file to read from.
  ++++++++++++++++++++++++++++++++++++++*/

static char *fgets_realloc(char *buffer,FILE *file)
{
 int n=0;
 char *buf;

 if(!buffer)
    buffer=(char*)malloc(BUFFSIZE+1);

 while((buf=fgets(&buffer[n],BUFFSIZE,file)))
   {
    int s=strlen(buf);
    n+=s;

    if(buffer[n-1]=='\n')
       break;
    else
       buffer=(char*)realloc(buffer,n+BUFFSIZE+1);
   }

 if(!buf)
   {free(buffer);buffer=NULL;}

 return(buffer);
}