/*===========================================================================*\
|*  lucc - An open source UCC implementation that makes use of libunr        *|
|*  Copyright (C) 2018-2019  Adam W.E. Smith                                 *|
|*                                                                           *|
|*  This program is free software: you can redistribute it and/or modify     *|
|*  it under the terms of the GNU General Public License as published by     *|
|*  the Free Software Foundation, either version 3 of the License, or        *|
|*  (at your option) any later version.                                      *|
|*                                                                           *|
|*  This program is distributed in the hope that it will be useful,          *|
|*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *|
|*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *|
|*  GNU General Public License for more details.                             *|
|*                                                                           *|
|*  You should have received a copy of the GNU General Public License        *|
|*  along with this program. If not, see <https://www.gnu.org/licenses/>.    *|
\*===========================================================================*/

/*========================================================================
 * lucc.h - Main lucc header file
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#pragma once

#ifdef _WIN32
  #include <direct.h>
  #include <Windows.h>
  #undef DrawText
#endif

#include <libunr.h>

// Error codes
#define ERR_BAD_ARGS      1
#define ERR_MISSING_PKG   2
#define ERR_MISSING_CLASS 3
#define ERR_BAD_OBJECT    4
#define ERR_UNKNOWN_CMD   5
#define ERR_LIBUNR_INIT   6
#define ERR_BAD_PATH      7

extern char wd[4096]; // Working directory
extern char Path[4096];
extern char* PkgName;
extern char* SingleObject;

// Command handler function
typedef int(*CommandHandler)(int, char**);

struct UccCommand
{
  const char* Name;
  CommandHandler Func;
};

#define DECLARE_UCC_COMMAND( name ) \
  int name ( int argc, char** argv ); \
  UccCommand name##Command = { TXT(name), name };
