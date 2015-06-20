/***************************************
 Load the translations from a file and the functions for handling them.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2010-2015 Andrew M. Bishop

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
#include <string.h>
#include <stdlib.h>

#include "files.h"
#include "translations.h"
#include "xmlparse.h"


/* Default English translations - Must not require any UTF-8 encoding */

static Translation default_translation=
{
 .raw_copyright_creator = {"Creator","Routino - http://www.routino.org/"},
 .raw_copyright_source  = {NULL,NULL},
 .raw_copyright_license = {NULL,NULL},

 .xml_copyright_creator = {"Creator","Routino - http://www.routino.org/"},
 .xml_copyright_source  = {NULL,NULL},
 .xml_copyright_license = {NULL,NULL},

 .xml_heading = {"South","South-West","West","North-West","North","North-East","East","South-East","South"},
 .xml_turn    = {"Very sharp left","Sharp left","Left","Slight left","Straight on","Slight right","Right","Sharp right","Very sharp right"},
 .xml_ordinal = {"First","Second","Third","Fourth","Fifth","Sixth","Seventh","Eighth","Ninth","Tenth"},

 .raw_highway = {"","motorway","trunk road","primary road","secondary road","tertiary road","unclassified road","residential road","service road","track","cycleway","path","steps","ferry"},

 .xml_route_shortest = "Shortest",
 .xml_route_quickest = "Quickest",

 .html_waypoint   = "<span class='w'>Waypoint</span>", /* span tag added when reading XML translations file */
 .html_junction   = "Junction",
 .html_roundabout = "Roundabout",

 .html_title   = "%s Route",
 .html_start   = {"Start" ,"At %s, head <span class='b'>%s</span>"}, /* span tag added when reading XML translations file */
 .html_node    = {"At"    ,"%s, go <span class='t'>%s</span> heading <span class='b'>%s</span>"}, /* span tag added when reading XML translations file */
 .html_rbnode  = {"Leave" ,"%s, take the <span class='b'>%s</span> exit heading <span class='b'>%s</span>"}, /* span tag added when reading XML translations file */
 .html_segment = {"Follow","<span class='h'>%s</span> for <span class='d'>%.3f km, %.1f min</span>"}, /* span tag added when reading XML translations file */
 .html_stop    = {"Stop"  ,"At %s"},
 .html_total   = {"Total" ,"%.1f km, %.0f minutes"},

 .gpx_desc  = "%s route between 'start' and 'finish' waypoints",
 .gpx_name  = "%s route",
 .gpx_step  = "%s on '%s' for %.3f km, %.1f min",
 .gpx_final = "Total Journey %.1f km, %.0f minutes",

 .gpx_start  = "START",
 .gpx_inter  = "INTER",
 .gpx_trip   = "TRIP",
 .gpx_finish = "FINISH"
};


/* Global variables */

/*+ The set of parsed translations. +*/
Translation *translation=&default_translation;


/* Local variables (re-initialised for each file) */

/*+ The language that is to be stored. +*/
static const char *store_lang;

/*+ This current language is to be stored. +*/
static int store;

/*+ The chosen language has been stored. +*/
static int stored;


/* The XML tag processing function prototypes */

//static int xmlDeclaration_function(const char *_tag_,int _type_,const char *version,const char *encoding);
//static int RoutinoTranslationsType_function(const char *_tag_,int _type_);
static int LanguageType_function(const char *_tag_,int _type_,const char *lang);
//static int CopyrightType_function(const char *_tag_,int _type_);
static int TurnType_function(const char *_tag_,int _type_,const char *direction,const char *string);
static int HeadingType_function(const char *_tag_,int _type_,const char *direction,const char *string);
static int OrdinalType_function(const char *_tag_,int _type_,const char *number,const char *string);
static int HighwayType_function(const char *_tag_,int _type_,const char *type,const char *string);
static int RouteType_function(const char *_tag_,int _type_,const char *type,const char *string);
//static int HTMLType_function(const char *_tag_,int _type_);
//static int GPXType_function(const char *_tag_,int _type_);
static int CopyrightCreatorType_function(const char *_tag_,int _type_,const char *string,const char *text);
static int CopyrightSourceType_function(const char *_tag_,int _type_,const char *string,const char *text);
static int CopyrightLicenseType_function(const char *_tag_,int _type_,const char *string,const char *text);
static int HTMLWaypointType_function(const char *_tag_,int _type_,const char *type,const char *string);
static int HTMLTitleType_function(const char *_tag_,int _type_,const char *text);
static int HTMLStartType_function(const char *_tag_,int _type_,const char *string,const char *text);
static int HTMLNodeType_function(const char *_tag_,int _type_,const char *string,const char *text);
static int HTMLRBNodeType_function(const char *_tag_,int _type_,const char *string,const char *text);
static int HTMLSegmentType_function(const char *_tag_,int _type_,const char *string,const char *text);
static int HTMLStopType_function(const char *_tag_,int _type_,const char *string,const char *text);
static int HTMLTotalType_function(const char *_tag_,int _type_,const char *string,const char *text);
static int GPXWaypointType_function(const char *_tag_,int _type_,const char *type,const char *string);
static int GPXDescType_function(const char *_tag_,int _type_,const char *text);
static int GPXNameType_function(const char *_tag_,int _type_,const char *text);
static int GPXStepType_function(const char *_tag_,int _type_,const char *text);
static int GPXFinalType_function(const char *_tag_,int _type_,const char *text);


/* The XML tag definitions (forward declarations) */

static const xmltag xmlDeclaration_tag;
static const xmltag RoutinoTranslationsType_tag;
static const xmltag LanguageType_tag;
static const xmltag CopyrightType_tag;
static const xmltag TurnType_tag;
static const xmltag HeadingType_tag;
static const xmltag OrdinalType_tag;
static const xmltag HighwayType_tag;
static const xmltag RouteType_tag;
static const xmltag HTMLType_tag;
static const xmltag GPXType_tag;
static const xmltag CopyrightCreatorType_tag;
static const xmltag CopyrightSourceType_tag;
static const xmltag CopyrightLicenseType_tag;
static const xmltag HTMLWaypointType_tag;
static const xmltag HTMLTitleType_tag;
static const xmltag HTMLStartType_tag;
static const xmltag HTMLNodeType_tag;
static const xmltag HTMLRBNodeType_tag;
static const xmltag HTMLSegmentType_tag;
static const xmltag HTMLStopType_tag;
static const xmltag HTMLTotalType_tag;
static const xmltag GPXWaypointType_tag;
static const xmltag GPXDescType_tag;
static const xmltag GPXNameType_tag;
static const xmltag GPXStepType_tag;
static const xmltag GPXFinalType_tag;


/* The XML tag definition values */

/*+ The complete set of tags at the top level. +*/
static const xmltag * const xml_toplevel_tags[]={&xmlDeclaration_tag,&RoutinoTranslationsType_tag,NULL};

/*+ The xmlDeclaration type tag. +*/
static const xmltag xmlDeclaration_tag=
              {"xml",
               2, {"version","encoding"},
               NULL,
               {NULL}};

/*+ The RoutinoTranslationsType type tag. +*/
static const xmltag RoutinoTranslationsType_tag=
              {"routino-translations",
               0, {NULL},
               NULL,
               {&LanguageType_tag,NULL}};

/*+ The LanguageType type tag. +*/
static const xmltag LanguageType_tag=
              {"language",
               1, {"lang"},
               LanguageType_function,
               {&CopyrightType_tag,&TurnType_tag,&HeadingType_tag,&OrdinalType_tag,&HighwayType_tag,&RouteType_tag,&HTMLType_tag,&GPXType_tag,NULL}};

/*+ The CopyrightType type tag. +*/
static const xmltag CopyrightType_tag=
              {"copyright",
               0, {NULL},
               NULL,
               {&CopyrightCreatorType_tag,&CopyrightSourceType_tag,&CopyrightLicenseType_tag,NULL}};

/*+ The TurnType type tag. +*/
static const xmltag TurnType_tag=
              {"turn",
               2, {"direction","string"},
               TurnType_function,
               {NULL}};

/*+ The HeadingType type tag. +*/
static const xmltag HeadingType_tag=
              {"heading",
               2, {"direction","string"},
               HeadingType_function,
               {NULL}};

/*+ The OrdinalType type tag. +*/
static const xmltag OrdinalType_tag=
              {"ordinal",
               2, {"number","string"},
               OrdinalType_function,
               {NULL}};

/*+ The HighwayType type tag. +*/
static const xmltag HighwayType_tag=
              {"highway",
               2, {"type","string"},
               HighwayType_function,
               {NULL}};

/*+ The RouteType type tag. +*/
static const xmltag RouteType_tag=
              {"route",
               2, {"type","string"},
               RouteType_function,
               {NULL}};

/*+ The HTMLType type tag. +*/
static const xmltag HTMLType_tag=
              {"output-html",
               0, {NULL},
               NULL,
               {&HTMLWaypointType_tag,&HTMLTitleType_tag,&HTMLStartType_tag,&HTMLNodeType_tag,&HTMLRBNodeType_tag,&HTMLSegmentType_tag,&HTMLStopType_tag,&HTMLTotalType_tag,NULL}};

/*+ The GPXType type tag. +*/
static const xmltag GPXType_tag=
              {"output-gpx",
               0, {NULL},
               NULL,
               {&GPXWaypointType_tag,&GPXDescType_tag,&GPXNameType_tag,&GPXStepType_tag,&GPXFinalType_tag,NULL}};

/*+ The CopyrightCreatorType type tag. +*/
static const xmltag CopyrightCreatorType_tag=
              {"creator",
               2, {"string","text"},
               CopyrightCreatorType_function,
               {NULL}};

/*+ The CopyrightSourceType type tag. +*/
static const xmltag CopyrightSourceType_tag=
              {"source",
               2, {"string","text"},
               CopyrightSourceType_function,
               {NULL}};

/*+ The CopyrightLicenseType type tag. +*/
static const xmltag CopyrightLicenseType_tag=
              {"license",
               2, {"string","text"},
               CopyrightLicenseType_function,
               {NULL}};

/*+ The HTMLWaypointType type tag. +*/
static const xmltag HTMLWaypointType_tag=
              {"waypoint",
               2, {"type","string"},
               HTMLWaypointType_function,
               {NULL}};

/*+ The HTMLTitleType type tag. +*/
static const xmltag HTMLTitleType_tag=
              {"title",
               1, {"text"},
               HTMLTitleType_function,
               {NULL}};

/*+ The HTMLStartType type tag. +*/
static const xmltag HTMLStartType_tag=
              {"start",
               2, {"string","text"},
               HTMLStartType_function,
               {NULL}};

/*+ The HTMLNodeType type tag. +*/
static const xmltag HTMLNodeType_tag=
              {"node",
               2, {"string","text"},
               HTMLNodeType_function,
               {NULL}};

/*+ The HTMLRBNodeType type tag. +*/
static const xmltag HTMLRBNodeType_tag=
              {"rbnode",
               2, {"string","text"},
               HTMLRBNodeType_function,
               {NULL}};

/*+ The HTMLSegmentType type tag. +*/
static const xmltag HTMLSegmentType_tag=
              {"segment",
               2, {"string","text"},
               HTMLSegmentType_function,
               {NULL}};

/*+ The HTMLStopType type tag. +*/
static const xmltag HTMLStopType_tag=
              {"stop",
               2, {"string","text"},
               HTMLStopType_function,
               {NULL}};

/*+ The HTMLTotalType type tag. +*/
static const xmltag HTMLTotalType_tag=
              {"total",
               2, {"string","text"},
               HTMLTotalType_function,
               {NULL}};

/*+ The GPXWaypointType type tag. +*/
static const xmltag GPXWaypointType_tag=
              {"waypoint",
               2, {"type","string"},
               GPXWaypointType_function,
               {NULL}};

/*+ The GPXDescType type tag. +*/
static const xmltag GPXDescType_tag=
              {"desc",
               1, {"text"},
               GPXDescType_function,
               {NULL}};

/*+ The GPXNameType type tag. +*/
static const xmltag GPXNameType_tag=
              {"name",
               1, {"text"},
               GPXNameType_function,
               {NULL}};

/*+ The GPXStepType type tag. +*/
static const xmltag GPXStepType_tag=
              {"step",
               1, {"text"},
               GPXStepType_function,
               {NULL}};

/*+ The GPXFinalType type tag. +*/
static const xmltag GPXFinalType_tag=
              {"final",
               1, {"text"},
               GPXFinalType_function,
               {NULL}};


/* The XML tag processing functions */


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the XML declaration is seen

  int xmlDeclaration_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *version The contents of the 'version' attribute (or NULL if not defined).

  const char *encoding The contents of the 'encoding' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

//static int xmlDeclaration_function(const char *_tag_,int _type_,const char *version,const char *encoding)
//{
// return(0);
//}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the RoutinoTranslationsType XSD type is seen

  int RoutinoTranslationsType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.
  ++++++++++++++++++++++++++++++++++++++*/

//static int RoutinoTranslationsType_function(const char *_tag_,int _type_)
//{
// return(0);
//}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the LanguageType XSD type is seen

  int LanguageType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *lang The contents of the 'lang' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int LanguageType_function(const char *_tag_,int _type_,const char *lang)
{
 if(_type_&XMLPARSE_TAG_START)
   {
    XMLPARSE_ASSERT_STRING(_tag_,lang);

    if(!store_lang && !stored)
       store=1;
    else if(store_lang && !strcmp(store_lang,lang))
       store=1;
    else
       store=0;
   }

 if(_type_&XMLPARSE_TAG_END && store)
   {
    store=0;
    stored=1;
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the CopyrightType XSD type is seen

  int CopyrightType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.
  ++++++++++++++++++++++++++++++++++++++*/

//static int CopyrightType_function(const char *_tag_,int _type_)
//{
// return(0);
//}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the TurnType XSD type is seen

  int TurnType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *direction The contents of the 'direction' attribute (or NULL if not defined).

  const char *string The contents of the 'string' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int TurnType_function(const char *_tag_,int _type_,const char *direction,const char *string)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring;
    int d;

    XMLPARSE_ASSERT_INTEGER(_tag_,direction); d=atoi(direction);
    XMLPARSE_ASSERT_STRING(_tag_,string);

    d+=4;

    if(d<0 || d>8)
       XMLPARSE_INVALID(_tag_,direction);

    xmlstring=ParseXML_Encode_Safe_XML(string);

    translation->xml_turn[d]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HeadingType XSD type is seen

  int HeadingType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *direction The contents of the 'direction' attribute (or NULL if not defined).

  const char *string The contents of the 'string' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HeadingType_function(const char *_tag_,int _type_,const char *direction,const char *string)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring;
    int d;

    XMLPARSE_ASSERT_INTEGER(_tag_,direction); d=atoi(direction);
    XMLPARSE_ASSERT_STRING(_tag_,string);

    d+=4;

    if(d<0 || d>8)
       XMLPARSE_INVALID(_tag_,direction);

    xmlstring=ParseXML_Encode_Safe_XML(string);

    translation->xml_heading[d]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the OrdinalType XSD type is seen

  int OrdinalType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *number The contents of the 'number' attribute (or NULL if not defined).

  const char *string The contents of the 'string' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int OrdinalType_function(const char *_tag_,int _type_,const char *number,const char *string)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring;
    int n;

    XMLPARSE_ASSERT_INTEGER(_tag_,number); n=atoi(number);
    XMLPARSE_ASSERT_STRING(_tag_,string);

    if(n<1 || n>10)
       XMLPARSE_INVALID(_tag_,number);

    xmlstring=ParseXML_Encode_Safe_XML(string);

    translation->xml_ordinal[n-1]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HighwayType XSD type is seen

  int HighwayType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *type The contents of the 'type' attribute (or NULL if not defined).

  const char *string The contents of the 'string' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HighwayType_function(const char *_tag_,int _type_,const char *type,const char *string)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    Highway highway;

    XMLPARSE_ASSERT_STRING(_tag_,type);
    XMLPARSE_ASSERT_STRING(_tag_,string);

    highway=HighwayType(type);

    if(highway==Highway_None)
       XMLPARSE_INVALID(_tag_,type);

    translation->raw_highway[highway]=strcpy(malloc(strlen(string)+1),string);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the RouteType XSD type is seen

  int RouteType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *type The contents of the 'type' attribute (or NULL if not defined).

  const char *string The contents of the 'string' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int RouteType_function(const char *_tag_,int _type_,const char *type,const char *string)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring;

    XMLPARSE_ASSERT_STRING(_tag_,type);
    XMLPARSE_ASSERT_STRING(_tag_,string);

    xmlstring=ParseXML_Encode_Safe_XML(string);

    if(!strcmp(type,"shortest"))
       translation->xml_route_shortest=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    else if(!strcmp(type,"quickest"))
       translation->xml_route_quickest=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    else
       XMLPARSE_INVALID(_tag_,type);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HTMLType XSD type is seen

  int HTMLType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.
  ++++++++++++++++++++++++++++++++++++++*/

//static int HTMLType_function(const char *_tag_,int _type_)
//{
// return(0);
//}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the GPXType XSD type is seen

  int GPXType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.
  ++++++++++++++++++++++++++++++++++++++*/

//static int GPXType_function(const char *_tag_,int _type_)
//{
// return(0);
//}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the CopyrightCreatorType XSD type is seen

  int CopyrightCreatorType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *string The contents of the 'string' attribute (or NULL if not defined).

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int CopyrightCreatorType_function(const char *_tag_,int _type_,const char *string,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring,*xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,string);
    XMLPARSE_ASSERT_STRING(_tag_,text);

    translation->raw_copyright_creator[0]=strcpy(malloc(strlen(string)+1),string);
    translation->raw_copyright_creator[1]=strcpy(malloc(strlen(text)+1)  ,text);

    xmlstring=ParseXML_Encode_Safe_XML(string);
    xmltext  =ParseXML_Encode_Safe_XML(text);

    translation->xml_copyright_creator[0]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    translation->xml_copyright_creator[1]=strcpy(malloc(strlen(xmltext)+1)  ,xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the CopyrightSourceType XSD type is seen

  int CopyrightSourceType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *string The contents of the 'string' attribute (or NULL if not defined).

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int CopyrightSourceType_function(const char *_tag_,int _type_,const char *string,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring,*xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,string);
    XMLPARSE_ASSERT_STRING(_tag_,text);

    translation->raw_copyright_source[0]=strcpy(malloc(strlen(string)+1),string);
    translation->raw_copyright_source[1]=strcpy(malloc(strlen(text)+1)  ,text);

    xmlstring=ParseXML_Encode_Safe_XML(string);
    xmltext  =ParseXML_Encode_Safe_XML(text);

    translation->xml_copyright_source[0]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    translation->xml_copyright_source[1]=strcpy(malloc(strlen(xmltext)+1)  ,xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the CopyrightLicenseType XSD type is seen

  int CopyrightLicenseType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *string The contents of the 'string' attribute (or NULL if not defined).

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int CopyrightLicenseType_function(const char *_tag_,int _type_,const char *string,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring,*xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,string);
    XMLPARSE_ASSERT_STRING(_tag_,text);

    translation->raw_copyright_license[0]=strcpy(malloc(strlen(string)+1),string);
    translation->raw_copyright_license[1]=strcpy(malloc(strlen(text)+1)  ,text);

    xmlstring=ParseXML_Encode_Safe_XML(string);
    xmltext  =ParseXML_Encode_Safe_XML(text);

    translation->xml_copyright_license[0]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    translation->xml_copyright_license[1]=strcpy(malloc(strlen(xmltext)+1)  ,xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HTMLWaypointType XSD type is seen

  int HTMLWaypointType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *type The contents of the 'type' attribute (or NULL if not defined).

  const char *string The contents of the 'string' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HTMLWaypointType_function(const char *_tag_,int _type_,const char *type,const char *string)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring;

    XMLPARSE_ASSERT_STRING(_tag_,type);
    XMLPARSE_ASSERT_STRING(_tag_,string);

    xmlstring=ParseXML_Encode_Safe_XML(string);

    if(!strcmp(type,"waypoint"))
      {
       translation->html_waypoint=malloc(strlen(xmlstring)+1+sizeof("<span class='w'>")+sizeof("</span>"));
       sprintf(translation->html_waypoint,"<span class='w'>%s</span>",xmlstring);
      }
    else if(!strcmp(type,"junction"))
       translation->html_junction=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    else if(!strcmp(type,"roundabout"))
       translation->html_roundabout=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    else
       XMLPARSE_INVALID(_tag_,type);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HTMLTitleType XSD type is seen

  int HTMLTitleType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HTMLTitleType_function(const char *_tag_,int _type_,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmltext=ParseXML_Encode_Safe_XML(text);

    translation->html_title=strcpy(malloc(strlen(xmltext)+1),xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HTMLStartType XSD type is seen

  int HTMLStartType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *string The contents of the 'string' attribute (or NULL if not defined).

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HTMLStartType_function(const char *_tag_,int _type_,const char *string,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring,*xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,string);
    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmlstring=ParseXML_Encode_Safe_XML(string);
    xmltext  =ParseXML_Encode_Safe_XML(text);

    translation->html_start[0]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    translation->html_start[1]=malloc(strlen(xmltext)+1+sizeof("<span class='b'>")+sizeof("</span>"));
    sprintf(translation->html_start[1],xmltext,"%s","<span class='b'>%s</span>");
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HTMLNodeType XSD type is seen

  int HTMLNodeType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *string The contents of the 'string' attribute (or NULL if not defined).

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HTMLNodeType_function(const char *_tag_,int _type_,const char *string,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring,*xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,string);
    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmlstring=ParseXML_Encode_Safe_XML(string);
    xmltext  =ParseXML_Encode_Safe_XML(text);

    translation->html_node[0]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    translation->html_node[1]=malloc(strlen(xmltext)+1+2*sizeof("<span class='b'>")+2*sizeof("</span>"));
    sprintf(translation->html_node[1],xmltext,"%s","<span class='t'>%s</span>","<span class='b'>%s</span>");
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HTMLRBNodeType XSD type is seen

  int HTMLRBNodeType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *string The contents of the 'string' attribute (or NULL if not defined).

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HTMLRBNodeType_function(const char *_tag_,int _type_,const char *string,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring,*xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,string);
    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmlstring=ParseXML_Encode_Safe_XML(string);
    xmltext  =ParseXML_Encode_Safe_XML(text);

    translation->html_rbnode[0]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    translation->html_rbnode[1]=malloc(strlen(xmltext)+1+2*sizeof("<span class='b'>")+2*sizeof("</span>"));
    sprintf(translation->html_rbnode[1],xmltext,"%s","<span class='t'>%s</span>","<span class='b'>%s</span>");
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HTMLSegmentType XSD type is seen

  int HTMLSegmentType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *string The contents of the 'string' attribute (or NULL if not defined).

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HTMLSegmentType_function(const char *_tag_,int _type_,const char *string,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring,*xmltext;
    const char *p;
    char *q;

    XMLPARSE_ASSERT_STRING(_tag_,string);
    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmlstring=ParseXML_Encode_Safe_XML(string);
    xmltext  =ParseXML_Encode_Safe_XML(text);

    translation->html_segment[0]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    translation->html_segment[1]=malloc(strlen(xmltext)+1+2*sizeof("<span class='b'>")+2*sizeof("</span>"));

    p=xmltext;
    q=translation->html_segment[1];

    while(*p!='%' && *(p+1)!='s')
       *q++=*p++;

    p+=2;
    strcpy(q,"<span class='h'>%s</span>"); q+=sizeof("<span class='h'>%s</span>")-1;

    while(*p!='%')
       *q++=*p++;

    strcpy(q,"<span class='d'>"); q+=sizeof("<span class='d'>")-1;

    strcpy(q,p);
    strcat(q,"</span>");
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HTMLStopType XSD type is seen

  int HTMLStopType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *string The contents of the 'string' attribute (or NULL if not defined).

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HTMLStopType_function(const char *_tag_,int _type_,const char *string,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring,*xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,string);
    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmlstring=ParseXML_Encode_Safe_XML(string);
    xmltext  =ParseXML_Encode_Safe_XML(text);

    translation->html_stop[0]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    translation->html_stop[1]=strcpy(malloc(strlen(xmltext)+1)  ,xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the HTMLTotalType XSD type is seen

  int HTMLTotalType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *string The contents of the 'string' attribute (or NULL if not defined).

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int HTMLTotalType_function(const char *_tag_,int _type_,const char *string,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring,*xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,string);
    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmlstring=ParseXML_Encode_Safe_XML(string);
    xmltext  =ParseXML_Encode_Safe_XML(text);

    translation->html_total[0]=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    translation->html_total[1]=strcpy(malloc(strlen(xmltext)+1)  ,xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the GPXWaypointType XSD type is seen

  int GPXWaypointType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *type The contents of the 'type' attribute (or NULL if not defined).

  const char *string The contents of the 'string' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int GPXWaypointType_function(const char *_tag_,int _type_,const char *type,const char *string)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmlstring;

    XMLPARSE_ASSERT_STRING(_tag_,type);
    XMLPARSE_ASSERT_STRING(_tag_,string);

    xmlstring=ParseXML_Encode_Safe_XML(string);

    if(!strcmp(type,"start"))
       translation->gpx_start=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    else if(!strcmp(type,"inter"))
       translation->gpx_inter=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    else if(!strcmp(type,"trip"))
       translation->gpx_trip=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    else if(!strcmp(type,"finish"))
       translation->gpx_finish=strcpy(malloc(strlen(xmlstring)+1),xmlstring);
    else
       XMLPARSE_INVALID(_tag_,type);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the GPXDescType XSD type is seen

  int GPXDescType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int GPXDescType_function(const char *_tag_,int _type_,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmltext=ParseXML_Encode_Safe_XML(text);

    translation->gpx_desc=strcpy(malloc(strlen(xmltext)+1),xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the GPXNameType XSD type is seen

  int GPXNameType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int GPXNameType_function(const char *_tag_,int _type_,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmltext=ParseXML_Encode_Safe_XML(text);

    translation->gpx_name=strcpy(malloc(strlen(xmltext)+1),xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the GPXStepType XSD type is seen

  int GPXStepType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int GPXStepType_function(const char *_tag_,int _type_,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmltext=ParseXML_Encode_Safe_XML(text);

    translation->gpx_step=strcpy(malloc(strlen(xmltext)+1),xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The function that is called when the GPXFinalType XSD type is seen

  int GPXFinalType_function Returns 0 if no error occured or something else otherwise.

  const char *_tag_ Set to the name of the element tag that triggered this function call.

  int _type_ Set to XMLPARSE_TAG_START at the start of a tag and/or XMLPARSE_TAG_END at the end of a tag.

  const char *text The contents of the 'text' attribute (or NULL if not defined).
  ++++++++++++++++++++++++++++++++++++++*/

static int GPXFinalType_function(const char *_tag_,int _type_,const char *text)
{
 if(_type_&XMLPARSE_TAG_START && store)
   {
    char *xmltext;

    XMLPARSE_ASSERT_STRING(_tag_,text);

    xmltext=ParseXML_Encode_Safe_XML(text);

    translation->gpx_final=strcpy(malloc(strlen(xmltext)+1),xmltext);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  The XML translation parser.

  int ParseXMLTranslations Returns 0 if OK or something else in case of an error.

  const char *filename The name of the file to read.

  const char *language The language to search for (NULL means first in file).
  ++++++++++++++++++++++++++++++++++++++*/

int ParseXMLTranslations(const char *filename,const char *language)
{
 int fd;
 int retval;

 if(!ExistsFile(filename))
   {
#ifndef LIBROUTINO
    fprintf(stderr,"Error: Specified translations file '%s' does not exist.\n",filename);
#endif
    return(1);
   }

 fd=OpenFile(filename);

 /* Start with a copy of the default translations */

 if(translation!=&default_translation)
    FreeXMLTranslations();

 translation=calloc(sizeof(Translation),1);

 *translation=default_translation;

 /* Initialise variables used for parsing */

 store_lang=language;

 store=0;
 stored=0;

 /* Parse the file */

 retval=ParseXML(fd,xml_toplevel_tags,XMLPARSE_UNKNOWN_ATTR_ERRNONAME|XMLPARSE_RETURN_ATTR_ENCODED);

 CloseFile(fd);

 if(retval)
    return(1);

 if(language && !stored)
#ifdef LIBROUTINO
    return(1);
#else
    fprintf(stderr,"Warning: Cannot find translations for language '%s' using English instead.\n",language);
#endif

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Free the memory that has been allocated for the translations.
  ++++++++++++++++++++++++++++++++++++++*/

void FreeXMLTranslations()
{
 int i;

 if(translation==&default_translation)
    return;

 for(i=0;i<2;i++)
   {
    if(translation->raw_copyright_creator[i] != default_translation.raw_copyright_creator[i]) free(translation->raw_copyright_creator[i]);
    if(translation->raw_copyright_source[i]  != default_translation.raw_copyright_source[i])  free(translation->raw_copyright_source[i]);
    if(translation->raw_copyright_license[i] != default_translation.raw_copyright_license[i]) free(translation->raw_copyright_license[i]);

    if(translation->xml_copyright_creator[i] != default_translation.xml_copyright_creator[i]) free(translation->xml_copyright_creator[i]);
    if(translation->xml_copyright_source[i]  != default_translation.xml_copyright_source[i])  free(translation->xml_copyright_source[i]);
    if(translation->xml_copyright_license[i] != default_translation.xml_copyright_license[i]) free(translation->xml_copyright_license[i]);
   }

 for(i=0;i<9;i++)
   {
    if(translation->xml_heading[i] != default_translation.xml_heading[i]) free(translation->xml_heading[i]);
    if(translation->xml_turn[i]    != default_translation.xml_turn[i])    free(translation->xml_turn[i]);
   }

 for(i=0;i<10;i++)
    if(translation->xml_ordinal[i] != default_translation.xml_ordinal[i]) free(translation->xml_ordinal[i]);

 for(i=0;i<Highway_Count;i++)
    if(translation->raw_highway[i] != default_translation.raw_highway[i]) free(translation->raw_highway[i]);

 if(translation->xml_route_shortest != default_translation.xml_route_shortest) free(translation->xml_route_shortest);
 if(translation->xml_route_quickest != default_translation.xml_route_quickest) free(translation->xml_route_quickest);

 if(translation->html_waypoint   != default_translation.html_waypoint)   free(translation->html_waypoint);
 if(translation->html_junction   != default_translation.html_junction)   free(translation->html_junction);
 if(translation->html_roundabout != default_translation.html_roundabout) free(translation->html_roundabout);

 if(translation->html_title != default_translation.html_title) free(translation->html_title);

 for(i=0;i<2;i++)
   {
    if(translation->html_start[i]   != default_translation.html_start[i])   free(translation->html_start[i]);
    if(translation->html_segment[i] != default_translation.html_segment[i]) free(translation->html_segment[i]);
    if(translation->html_node[i]    != default_translation.html_node[i])    free(translation->html_node[i]);
    if(translation->html_rbnode[i]  != default_translation.html_rbnode[i])  free(translation->html_rbnode[i]);
    if(translation->html_stop[i]    != default_translation.html_stop[i])    free(translation->html_stop[i]);
    if(translation->html_total[i]   != default_translation.html_total[i])   free(translation->html_total[i]);
   }

 if(translation->gpx_desc  != default_translation.gpx_desc)  free(translation->gpx_desc);
 if(translation->gpx_name  != default_translation.gpx_name)  free(translation->gpx_name);
 if(translation->gpx_step  != default_translation.gpx_step)  free(translation->gpx_step);
 if(translation->gpx_final != default_translation.gpx_final) free(translation->gpx_final);

 if(translation->gpx_start  != default_translation.gpx_start)  free(translation->gpx_start);
 if(translation->gpx_inter  != default_translation.gpx_inter)  free(translation->gpx_inter);
 if(translation->gpx_trip   != default_translation.gpx_trip)   free(translation->gpx_trip);
 if(translation->gpx_finish != default_translation.gpx_finish) free(translation->gpx_finish);

 free(translation);

 translation=&default_translation;
}
