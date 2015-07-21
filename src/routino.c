/***************************************
 Routino library functions file.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2015 Andrew M. Bishop

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

#include "routino.h"

#include "types.h"
#include "nodes.h"
#include "segments.h"
#include "ways.h"
#include "relations.h"

#include "fakes.h"
#include "results.h"
#include "functions.h"
#include "profiles.h"
#include "translations.h"


/* Global variables */

/*+ Contains the error number of the most recent Routino error. +*/
int Routino_errno=ROUTINO_ERROR_NONE;

/*+ The options to select the format of the output. +*/
int option_html=1,option_gpx_track=1,option_gpx_route=1,option_text=1,option_text_all=1,option_stdout=0;

/*+ The option to calculate the quickest route insted of the shortest. +*/
int option_quickest=0;


/* Static variables */

static distance_t distmax=km_to_distance(1);


/* Local types */

struct _Routino_Database
{
 Nodes      *nodes;
 Segments   *segments;
 Ways       *ways;
 Relations  *relations;
};

struct _Routino_Waypoint
{
 index_t segment;
 index_t node1,node2;
 distance_t dist1,dist2;
};


/*++++++++++++++++++++++++++++++++++++++
  Load a database of files for Routino to use for routing.

  Routino_Database *Routino_LoadDatabase Returns a pointer to the database.

  const char *dirname The pathname of the directory containing the database files.

  const char *prefix The prefix of the database files.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC Routino_Database *Routino_LoadDatabase(const char *dirname,const char *prefix)
{
 char *nodes_filename;
 char *segments_filename;
 char *ways_filename;
 char *relations_filename;
 Routino_Database *database=NULL;

 nodes_filename    =FileName(dirname,prefix,"nodes.mem");
 segments_filename =FileName(dirname,prefix,"segments.mem");
 ways_filename     =FileName(dirname,prefix,"ways.mem");
 relations_filename=FileName(dirname,prefix,"relations.mem");

 if(!ExistsFile(nodes_filename) || !ExistsFile(nodes_filename) || !ExistsFile(nodes_filename) || !ExistsFile(nodes_filename))
    Routino_errno=ROUTINO_ERROR_NO_DATABASE_FILES;
 else
   {
    database=calloc(sizeof(Routino_Database),1);

    database->nodes    =LoadNodeList    (nodes_filename);
    database->segments =LoadSegmentList (segments_filename);
    database->ways     =LoadWayList     (ways_filename);
    database->relations=LoadRelationList(relations_filename);
   }

 free(nodes_filename);
 free(segments_filename);
 free(ways_filename);
 free(relations_filename);

 if(!database->nodes || !database->segments || !database->ways || !database->relations)
   {
    Routino_UnloadDatabase(database);
    database=NULL;

    Routino_errno=ROUTINO_ERROR_BAD_DATABASE_FILES;
   }

 if(database)
   {
    Routino_errno=ROUTINO_ERROR_NONE;
    return(database);
   }
 else
    return(NULL);
}


/*++++++++++++++++++++++++++++++++++++++
  Close the database files that were opened by a call to Routino_LoadDatabase().

  Routino_Database *database The database to close.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC void Routino_UnloadDatabase(Routino_Database *database)
{
 if(!database)
    Routino_errno=ROUTINO_ERROR_NO_DATABASE;
 else
   {
    if(database->nodes)     DestroyNodeList    (database->nodes);
    if(database->segments)  DestroySegmentList (database->segments);
    if(database->ways)      DestroyWayList     (database->ways);
    if(database->relations) DestroyRelationList(database->relations);

    free(database);

    Routino_errno=ROUTINO_ERROR_NONE;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Parse a Routino XML file containing profiles, must be called before selecting a profile.

  int Routino_ParseXMLProfiles Returns non-zero in case of an error or zero if there was no error.

  const char *filename The full pathname of the file to read.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC int Routino_ParseXMLProfiles(const char *filename)
{
 int retval;

 retval=ParseXMLProfiles(filename,NULL,1);

 if(retval==1)
    retval=ROUTINO_ERROR_NO_PROFILES_XML;
 else if(retval==2)
    retval=ROUTINO_ERROR_BAD_PROFILES_XML;

 Routino_errno=retval;
 return(retval);
}


/*++++++++++++++++++++++++++++++++++++++
  Return a list of the profile names that have been loaded from the XML file.

  char **Routino_GetProfileNames Returns a NULL terminated list of strings - all allocated.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC char **Routino_GetProfileNames(void)
{
 Routino_errno=ROUTINO_ERROR_NONE;

 return(GetProfileNames());
}


/*++++++++++++++++++++++++++++++++++++++
  Select a specific routing profile from the set of Routino profiles that have been loaded from the XML file or NULL in case of an error.

  Routino_Profile *Routino_GetProfile Returns a pointer to an internal data structure - do not free.

  const char *name The name of the profile to select.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC Routino_Profile *Routino_GetProfile(const char *name)
{
 Profile *profile=GetProfile(name);

 if(profile)
    Routino_errno=ROUTINO_ERROR_NONE;
 else
    Routino_errno=ROUTINO_ERROR_NO_SUCH_PROFILE;

 return(profile);
}


/*++++++++++++++++++++++++++++++++++++++
  Free the internal memory that was allocated for the Routino profiles loaded from the XML file.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC void Routino_FreeXMLProfiles(void)
{
 Routino_errno=ROUTINO_ERROR_NONE;

 FreeXMLProfiles();
}


/*++++++++++++++++++++++++++++++++++++++
  Parse a Routino XML file containing translations, must be called before selecting a translation.

  int Routino_ParseXMLTranslations Returns non-zero in case of an error or zero if there was no error.

  const char *filename The full pathname of the file to read.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC int Routino_ParseXMLTranslations(const char *filename)
{
 int retval;

 retval=ParseXMLTranslations(filename,NULL,1);

 if(retval==1)
    retval=ROUTINO_ERROR_NO_TRANSLATIONS_XML;
 else if(retval==2)
    retval=ROUTINO_ERROR_BAD_TRANSLATIONS_XML;

 Routino_errno=retval;
 return(retval);
}


/*++++++++++++++++++++++++++++++++++++++
  Return a list of the translation languages that have been loaded from the XML file.

  char **Routino_GetTranslationLanguages Returns a NULL terminated list of strings - all allocated.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC char **Routino_GetTranslationLanguages(void)
{
 Routino_errno=ROUTINO_ERROR_NONE;

 return(GetTranslationLanguages());
}


/*++++++++++++++++++++++++++++++++++++++
  Select a specific translation from the set of Routino translations that have been loaded from the XML file or NULL in case of an error.

  Routino_Translation *Routino_GetTranslation Returns a pointer to an internal data structure - do not free.

  const char *language The language to select (as a country code, e.g. 'en', 'de') or an empty string for the first in the file or NULL for the built-in English version.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC Routino_Translation *Routino_GetTranslation(const char *language)
{
 Translation *translation=GetTranslation(language);

 if(translation)
    Routino_errno=ROUTINO_ERROR_NONE;
 else
    Routino_errno=ROUTINO_ERROR_NO_SUCH_TRANSLATION;

 return(translation);
}


/*++++++++++++++++++++++++++++++++++++++
  Free the internal memory that was allocated for the Routino translations loaded from the XML file.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC void Routino_FreeXMLTranslations(void)
{
 Routino_errno=ROUTINO_ERROR_NONE;

 FreeXMLTranslations();
}


/*++++++++++++++++++++++++++++++++++++++
  Create a fully formed Routino Profile from a Routino User Profile.

  Routino_Profile *Routino_CreateProfileFromUserProfile Returns an allocated Routino Profile.

  Routino_UserProfile *profile The user specified profile to convert (not modified by this).
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC Routino_Profile *Routino_CreateProfileFromUserProfile(Routino_UserProfile *profile)
{
 Routino_Profile *rprofile=calloc(1,sizeof(Routino_Profile));
 int i;

 Routino_errno=ROUTINO_ERROR_NONE;

 if(profile->transport<=0 || profile->transport>=Transport_Count)
    Routino_errno=ROUTINO_ERROR_BAD_USER_PROFILE;
 else
    rprofile->transport=profile->transport;

 for(i=1;i<Highway_Count;i++)
   {
    if(profile->highway[i]<0 || profile->highway[i]>1)
       Routino_errno=ROUTINO_ERROR_BAD_USER_PROFILE;
    else
       rprofile->highway[i]=profile->highway[i];

    if(profile->speed[i]<=0)
       Routino_errno=ROUTINO_ERROR_BAD_USER_PROFILE;
    else
       rprofile->speed[i]=kph_to_speed(profile->speed[i]);
   }

 for(i=1;i<Property_Count;i++)
   {
    if(profile->props[i]<0 || profile->props[i]>1)
       Routino_errno=ROUTINO_ERROR_BAD_USER_PROFILE;
    else
       rprofile->props[i]=profile->props[i];
   }

 if(profile->weight<=0)
    Routino_errno=ROUTINO_ERROR_BAD_USER_PROFILE;
 else
    rprofile->weight=tonnes_to_weight(profile->weight);

 if(profile->height<=0)
    Routino_errno=ROUTINO_ERROR_BAD_USER_PROFILE;
 else
    rprofile->height=metres_to_height(profile->height);

 if(profile->width<=0)
    Routino_errno=ROUTINO_ERROR_BAD_USER_PROFILE;
 else
    rprofile->width=metres_to_width(profile->width);

 if(profile->length<=0)
    Routino_errno=ROUTINO_ERROR_BAD_USER_PROFILE;
 else
    rprofile->length=metres_to_length(profile->length);

 if(Routino_errno==ROUTINO_ERROR_NONE)
    return(rprofile);

 free(rprofile);
 return(NULL);
}


/*++++++++++++++++++++++++++++++++++++++
  Create a Routino User Profile from a Routino Profile loaded from an XML file.

  Routino_UserProfile *Routino_CreateUserProfileFromProfile Returns an allocated Routino User Profile.

  Routino_Profile *profile The Routino Profile to convert (not modified by this).
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC Routino_UserProfile *Routino_CreateUserProfileFromProfile(Routino_Profile *profile)
{
 Routino_UserProfile *uprofile=calloc(1,sizeof(Routino_UserProfile));
 int i;

 Routino_errno=ROUTINO_ERROR_NONE;

 uprofile->transport=profile->transport;

 for(i=1;i<Highway_Count;i++)
   {
    uprofile->highway[i]=profile->highway[i];

    uprofile->speed[i]=speed_to_kph(profile->speed[i]);
   }

 for(i=1;i<Property_Count;i++)
    uprofile->props[i]=profile->props[i];

 uprofile->weight=weight_to_tonnes(profile->weight);

 uprofile->height=height_to_metres(profile->height);

 uprofile->width=width_to_metres(profile->width);

 uprofile->length=length_to_metres(profile->length);

 return(uprofile);
}


/*++++++++++++++++++++++++++++++++++++++
  Validates that a selected routing profile is valid for use with the selected routing database.

  int Routino_ValidateProfile Returns zero if OK or something else in case of an error.

  Routino_Database *database The Routino database to use.

  Routino_Profile *profile The Routino profile to validate.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC int Routino_ValidateProfile(Routino_Database *database,Routino_Profile *profile)
{
 Routino_errno=ROUTINO_ERROR_NONE;

 if(UpdateProfile(profile,database->ways))
    Routino_errno=ROUTINO_ERROR_PROFILE_DATABASE_ERR;

 return(Routino_errno);
}


/*++++++++++++++++++++++++++++++++++++++
  Finds the nearest point in the database to the specified latitude and longitude.

  Routino_Waypoint *Routino_FindWaypoint Returns a pointer to a newly allocated Routino waypoint or NULL if none could be found.

  Routino_Database *database The Routino database to use.

  Routino_Profile *profile The Routino profile to use.

  double latitude The latitude in degrees of the point.

  double longitude The longitude in degrees of the point.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC Routino_Waypoint *Routino_FindWaypoint(Routino_Database *database,Routino_Profile *profile,double latitude,double longitude)
{
 distance_t dist;
 Routino_Waypoint *waypoint;

 if(!database)
   {
    Routino_errno=ROUTINO_ERROR_NO_DATABASE;
    return(NULL);
   }

 if(!profile)
   {
    Routino_errno=ROUTINO_ERROR_NO_PROFILE;
    return(NULL);
   }

 if(!profile->allow)
   {
    Routino_errno=ROUTINO_ERROR_NOTVALID_PROFILE;
    return(NULL);
   }

 waypoint=calloc(sizeof(Routino_Waypoint),1);

 waypoint->segment=FindClosestSegment(database->nodes,database->segments,database->ways,
                                      degrees_to_radians(latitude),degrees_to_radians(longitude),distmax,profile,
                                      &dist,&waypoint->node1,&waypoint->node2,&waypoint->dist1,&waypoint->dist2);

 if(waypoint->segment==NO_SEGMENT)
   {
    free(waypoint);

    Routino_errno=ROUTINO_ERROR_NO_NEARBY_HIGHWAY;
    return(NULL);
   }

 Routino_errno=ROUTINO_ERROR_NONE;
 return(waypoint);
}


/*++++++++++++++++++++++++++++++++++++++
  Calculate a route using a loaded database, chosen profile, chosen translation and set of waypoints.

  int Routino_CalculateRoute Return zero on success or some other value in case of an error.

  Routino_Database *database The loaded database to use.

  Routino_Profile *profile The chosen routing profile to use.

  Routino_Translation *translation The chosen translation information to use.

  Routino_Waypoint **waypoints The set of waypoints.

  int nwaypoints The number of waypoints.

  int options The set of routing options ORed together.
  ++++++++++++++++++++++++++++++++++++++*/

DLL_PUBLIC int Routino_CalculateRoute(Routino_Database *database,Routino_Profile *profile,Routino_Translation *translation,
                                      Routino_Waypoint **waypoints,int nwaypoints,int options)
{
 int waypoint,retval=ROUTINO_ERROR_NONE;
 index_t start_node,finish_node=NO_NODE;
 index_t join_segment=NO_SEGMENT;
 Results **results;

 /* Check the input data */

 if(!database)
   {
    Routino_errno=ROUTINO_ERROR_NO_DATABASE;
    return(Routino_errno);
   }

 if(!profile)
   {
    Routino_errno=ROUTINO_ERROR_NO_PROFILE;
    return(Routino_errno);
   }

 if(!profile->allow)
   {
    Routino_errno=ROUTINO_ERROR_NOTVALID_PROFILE;
    return(Routino_errno);
   }

 if(!translation)
   {
    Routino_errno=ROUTINO_ERROR_NO_TRANSLATION;
    return(Routino_errno);
   }

 /* Extract the options */

 if(options&ROUTINO_ROUTE_QUICKEST)       option_quickest=1;  else option_quickest=0;

 if(options&ROUTINO_ROUTE_FILE_HTML)      option_html=1;      else option_html=0;
 if(options&ROUTINO_ROUTE_FILE_GPX_TRACK) option_gpx_track=1; else option_gpx_track=0;
 if(options&ROUTINO_ROUTE_FILE_GPX_ROUTE) option_gpx_route=1; else option_gpx_route=0;
 if(options&ROUTINO_ROUTE_FILE_TEXT)      option_text=1;      else option_text=0;
 if(options&ROUTINO_ROUTE_FILE_TEXT_ALL)  option_text_all=1;  else option_text_all=0;

 if(options&ROUTINO_ROUTE_FILE_STDOUT)    option_stdout=1;    else option_stdout=0;

 if(option_stdout && (option_html+option_gpx_track+option_gpx_route+option_text+option_text_all)!=1)
   {
    Routino_errno=ROUTINO_ERROR_BAD_OPTIONS;
    return(Routino_errno);
   }

 /* Loop through all pairs of waypoints */

 results=calloc(sizeof(Results*),nwaypoints);

 for(waypoint=0;waypoint<nwaypoints;waypoint++)
   {
    start_node=finish_node;

    finish_node=CreateFakes(database->nodes,database->segments,waypoint+1,
                            LookupSegment(database->segments,waypoints[waypoint]->segment,1),
                            waypoints[waypoint]->node1,waypoints[waypoint]->node2,waypoints[waypoint]->dist1,waypoints[waypoint]->dist2);

    if(waypoint==0)
       continue;

    results[waypoint-1]=CalculateRoute(database->nodes,database->segments,database->ways,database->relations,
                                       profile,start_node,join_segment,finish_node,waypoint,waypoint+1);

    if(!results[waypoint-1])
      {
       retval=ROUTINO_ERROR_NO_ROUTE_1+waypoint-1;
       goto tidy_and_exit;
      }

    join_segment=results[waypoint-1]->last_segment;
   }

 /* Print the route */

 PrintRoute(results,nwaypoints-1,database->nodes,database->segments,database->ways,profile,translation);

 /* Tidy up and exit */

 tidy_and_exit:

 DeleteFakeNodes();

 for(waypoint=0;waypoint<nwaypoints;waypoint++)
    if(results[waypoint])
       FreeResultsList(results[waypoint]);

 free(results);

 return(retval);
}