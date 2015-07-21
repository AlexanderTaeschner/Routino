/***************************************
 Routino library header file.

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


#ifndef ROUTINO_H
#define ROUTINO_H    /*+ To stop multiple inclusions. +*/

/* Limit the exported symbols in the library */

#if defined(_MSC_VER)
  #ifdef LIBROUTINO
    #define DLL_PUBLIC __declspec(dllexport)
  #else
    #define DLL_PUBLIC __declspec(dllimport)
  #endif
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
  #if defined(__MINGW32__) || defined(__CYGWIN__)
    #ifdef LIBROUTINO
      #define DLL_PUBLIC __attribute__ ((dllexport))
    #else
      #define DLL_PUBLIC __attribute__ ((dllimport))
    #endif
  #else
    #ifdef LIBROUTINO
      #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #endif
  #endif
#endif

#ifndef DLL_PUBLIC
  #define DLL_PUBLIC
#endif


/* Handle compilation with a C++ compiler */

#ifdef __cplusplus
extern "C"
{
#endif


 /* Routino error constants */

#define ROUTINO_ERROR_NONE                  0 /*+ No error. +*/

#define ROUTINO_ERROR_NO_DATABASE           1 /*+ A function was called without the database variable set. +*/
#define ROUTINO_ERROR_NO_PROFILE            2 /*+ A function was called without the profile variable set. +*/
#define ROUTINO_ERROR_NO_TRANSLATION        3 /*+ A function was called without the translation variable set. +*/

#define ROUTINO_ERROR_NO_DATABASE_FILES    11 /*+ The specified database to load did not exist. +*/
#define ROUTINO_ERROR_BAD_DATABASE_FILES   12 /*+ The specified database could not be loaded. +*/
#define ROUTINO_ERROR_NO_PROFILES_XML      13 /*+ The specified profiles XML file did not exist. +*/
#define ROUTINO_ERROR_BAD_PROFILES_XML     14 /*+ The specified profiles XML file could not be loaded. +*/
#define ROUTINO_ERROR_NO_TRANSLATIONS_XML  11 /*+ The specified translations XML file did not exist. +*/
#define ROUTINO_ERROR_BAD_TRANSLATIONS_XML 12 /*+ The specified translations XML file could not be loaded. +*/

#define ROUTINO_ERROR_NO_SUCH_PROFILE      21 /*+ The requested profile name does not exist in the loaded XML file. +*/
#define ROUTINO_ERROR_NO_SUCH_TRANSLATION  22 /*+ The requested translation language does not exist in the loaded XML file. +*/

#define ROUTINO_ERROR_NO_NEARBY_HIGHWAY    31 /*+ There is no highway near the coordinates to place a waypoint. +*/

#define ROUTINO_ERROR_PROFILE_DATABASE_ERR 41 /*+ The profile and database do not work together. +*/
#define ROUTINO_ERROR_NOTVALID_PROFILE     42 /*+ The profile being used has not been validated. +*/
#define ROUTINO_ERROR_BAD_USER_PROFILE     43 /*+ The user specified profile contained invalid data. +*/

#define ROUTINO_ERROR_BAD_OPTIONS          51 /*+ The routing options specified are not consistent with each other. +*/

#define ROUTINO_ERROR_NO_ROUTE_1         1001 /*+ A route could not be found to waypoint 1. +*/
#define ROUTINO_ERROR_NO_ROUTE_2         1002 /*+ A route could not be found to waypoint 2. +*/
#define ROUTINO_ERROR_NO_ROUTE_3         1003 /*+ A route could not be found to waypoint 3. +*/
/*  Higher values of the error number refer to later waypoints. */


 /* Routino routing option constants */

#define ROUTINO_ROUTE_SHORTEST              0 /*+ Calculate the shortest route. +*/
#define ROUTINO_ROUTE_QUICKEST              1 /*+ Calculate the quickest route. +*/

#define ROUTINO_ROUTE_FILE_HTML             2 /*+ Output an HTML route file. +*/
#define ROUTINO_ROUTE_FILE_GPX_TRACK        4 /*+ Output a GPX track file. +*/
#define ROUTINO_ROUTE_FILE_GPX_ROUTE        8 /*+ Output a GPX route file. +*/
#define ROUTINO_ROUTE_FILE_TEXT            16 /*+ Output a text file with important junctions. +*/
#define ROUTINO_ROUTE_FILE_TEXT_ALL        32 /*+ Output a text file with all nodes and segments. +*/

#define ROUTINO_ROUTE_FILE_STDOUT          64 /*+ Output a single file type to stdout. +*/


 /* Routino user profile array indexes */

#define ROUTINO_HIGHWAY_MOTORWAY            1 /*+ A Motorway highway. +*/
#define ROUTINO_HIGHWAY_TRUNK               2 /*+ A Trunk highway. +*/
#define ROUTINO_HIGHWAY_PRIMARY             3 /*+ A Primary highway. +*/
#define ROUTINO_HIGHWAY_SECONDARY           4 /*+ A Secondary highway. +*/
#define ROUTINO_HIGHWAY_TERTIARY            5 /*+ A Tertiary highway. +*/
#define ROUTINO_HIGHWAY_UNCLASSIFIED        6 /*+ A Unclassified highway. +*/
#define ROUTINO_HIGHWAY_RESIDENTIAL         7 /*+ A Residential highway. +*/
#define ROUTINO_HIGHWAY_SERVICE             8 /*+ A Service highway. +*/
#define ROUTINO_HIGHWAY_TRACK               9 /*+ A Track highway. +*/
#define ROUTINO_HIGHWAY_CYCLEWAY           10 /*+ A Cycleway highway. +*/
#define ROUTINO_HIGHWAY_PATH               11 /*+ A Path highway. +*/
#define ROUTINO_HIGHWAY_STEPS              12 /*+ A Steps highway. +*/
#define ROUTINO_HIGHWAY_FERRY              13 /*+ A Ferry highway. +*/

#define ROUTINO_PROPERTY_PAVED              1 /*+ A Paved highway. +*/
#define ROUTINO_PROPERTY_MULTILANE          2 /*+ A Multilane highway. +*/
#define ROUTINO_PROPERTY_BRIDGE             3 /*+ A Bridge highway. +*/
#define ROUTINO_PROPERTY_TUNNEL             4 /*+ A Tunnel highway. +*/
#define ROUTINO_PROPERTY_FOOTROUTE          5 /*+ A Footroute highway. +*/
#define ROUTINO_PROPERTY_BICYCLEROUTE       6 /*+ A Bicycleroute highway. +*/


 /* Routino error number variable */

 /*+ Contains the error number of the most recent Routino error. +*/
 extern int Routino_errno;


 /* Routino types */

 typedef struct _Routino_Database Routino_Database;
 typedef struct _Routino_Waypoint Routino_Waypoint;

#ifdef LIBROUTINO
 typedef struct _Profile             Routino_Profile;
 typedef struct _Translation         Routino_Translation;
#else
 typedef struct _Routino_Profile     Routino_Profile;
 typedef struct _Routino_Translation Routino_Translation;
#endif

/*+ A data structure to hold a transport type profile. +*/
typedef struct _Routino_UserProfile
{
 int    transport;              /*+ The type of transport. +*/

 float  highway[14];            /*+ A floating point preference for travel on the highway (range 0 to 1). +*/

 float  speed[14];              /*+ The maximum speed on each type of highway (km/hour). +*/

 float  props[7];               /*+ A floating point preference for ways with this attribute (range 0 to 1). +*/

 int    oneway;                 /*+ A flag to indicate if one-way restrictions apply. +*/
 int    turns;                  /*+ A flag to indicate if turn restrictions apply. +*/

 float  weight;                 /*+ The weight of the vehicle (in tonnes). +*/

 float  height;                 /*+ The height of the vehicle (in metres). +*/
 float  width;                  /*+ The width of vehicle (in metres). +*/
 float  length;                 /*+ The length of vehicle (in metres). +*/
}
 Routino_UserProfile;


 /* Routino library functions */

 DLL_PUBLIC Routino_Database *Routino_LoadDatabase(const char *dirname,const char *prefix);
 DLL_PUBLIC void Routino_UnloadDatabase(Routino_Database *database);

 DLL_PUBLIC int Routino_ParseXMLProfiles(const char *filename);
 DLL_PUBLIC char **Routino_GetProfileNames(void);
 DLL_PUBLIC Routino_Profile *Routino_GetProfile(const char *name);
 DLL_PUBLIC void Routino_FreeXMLProfiles(void);

 DLL_PUBLIC int Routino_ParseXMLTranslations(const char *filename);
 DLL_PUBLIC char **Routino_GetTranslationLanguages(void);
 DLL_PUBLIC Routino_Translation *Routino_GetTranslation(const char *language);
 DLL_PUBLIC void Routino_FreeXMLTranslations(void);

 DLL_PUBLIC Routino_Profile *Routino_CreateProfileFromUserProfile(Routino_UserProfile *profile);
 DLL_PUBLIC Routino_UserProfile *Routino_CreateUserProfileFromProfile(Routino_Profile *profile);

 DLL_PUBLIC int Routino_ValidateProfile(Routino_Database *database,Routino_Profile *profile);

 DLL_PUBLIC Routino_Waypoint *Routino_FindWaypoint(Routino_Database *database,Routino_Profile *profile,double latitude,double longitude);

 DLL_PUBLIC int Routino_CalculateRoute(Routino_Database *database,Routino_Profile *profile,Routino_Translation *translation,
                                       Routino_Waypoint **waypoints,int nwaypoints,int options);


/* Handle compilation with a C++ compiler */

#ifdef __cplusplus
}
#endif

#endif /* ROUTINO_H */
