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


 /* Library functions */

 DLL_PUBLIC void Routino(void);


/* Handle compilation with a C++ compiler */

#ifdef __cplusplus
}
#endif

#endif /* ROUTINO_H */
