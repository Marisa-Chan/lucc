/*========================================================================*\
|*  This file is part of lucc.                                            *|
|*                                                                        *|
|*  lucc is free software: you can redistribute it and/or modify          *|
|*  it under the terms of the GNU General Public License as published by  *|
|*  the Free Software Foundation, either version 3 of the License, or     *|
|*  (at your option) any later version.                                   *|
|*                                                                        *|
|*  lucc is distributed in the hope that it will be useful,               *|
|*  but WITHOUT ANY WARRANTY; without even the implied warranty of        *|
|*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *|
|*  GNU General Public License for more details.                          *|
|*                                                                        *|
|*  You should have received a copy of the GNU General Public License     *|
|*  along with lucc.  If not, see <http://www.gnu.org/licenses/>.         *|
|*                                                                        *|
\*========================================================================*/

/*========================================================================
 * lucc.cpp - libunr implemented UCC
 * 
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

// TODO: Implement commandlets
//
// Commandlets are means of performing tasks in UnrealScript without
// having any game state loaded. Commandlets don't have to be implemented
// in UnrealScript however (and the majority of them aren't)
//

#include "libunr.h"

// Error codes
#define ERR_BAD_ARGS      1
#define ERR_MISSING_PKG   2
#define ERR_MISSING_CLASS 3
#define ERR_BAD_OBJECT    4
#define ERR_UNKNOWN_CMD   5
#define ERR_LIBUNR_INIT   6

// Command handler function
typedef int(*CommandHandler)(int, char**);

struct UccCommand
{
  const char* Name;
  CommandHandler Func;
};

#define DECLARE_UCC_COMMAND( name ) \
  UccCommand name##Command = { TEXT(name), name };

void PrintHelpAndExit()
{
  printf("Usage:\n");
  printf("\tlucc <command> <parameters>\n\n");

  printf("Commands for \"lucc\":\n");
  printf("\tlucc batchclassexport\n");
  printf("\tlucc batchsoundexport\n");
  printf("\tlucc batchmusicexport\n");
  printf("\tlucc batchtextureexport\n");
}

int batchclassexport( int argc, char** argv )
{
  if ( argc < 4 )
  {
    printf("batchclassexport usage:\n");
    printf("\tlucc batchclassexport <Package Name> <Export Path>\n\n");
    return ERR_BAD_ARGS;
  }

  char* PkgName   = argv[2];
  char Path[4096] = { 0 };

  realpath( argv[3], Path );
  UClass* Class = UClass::StaticClass();

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPkg( PkgName );
  if ( Pkg == NULL )
  {
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist\n" );
    return ERR_MISSING_PKG;
  }

  // Iterate and export all objects and export class scripts
  Array<FExport>* ExportTable = Pkg->GetExportTable();
  for ( int i = 0; i < ExportTable->Size(); i++ )
  {
    const char* ObjName = Pkg->ResolveNameFromIdx( Export->Name );
    UClass* Obj = (UClass*)UPackage::StaticLoadObject( Pkg, ObjName, Class );
    if ( Obj == NULL )
    {
      Logf( LOG_CRIT, "Failed to load object '%s'\n");
      return ERR_BAD_OBJECT;
    }
    
    Obj->Export( Path, NULL ); 
  }

  return 0;
}

int StripSource( int argc, char** argv )
{
  printf("********************************************************************\n");
  printf("Don't hinder creativity by hiding your source. You aren't \"securing\"\n");
  printf("anything, and you aren't preventing anyone who wants to \"hack\" your mod\n");
  printf("from doing so anyway. Write secure code if you want a secure mod.\n");
  printf("\n");
  printf("Obscurity is NOT security.\n");
  printf("********************************************************************\n");
  return -1;
}

void GamePromptHandler( char* PathOut, char* NameOut )
{
  Array<char*> GameNames;
  int i = 1;
  char* GameName = GLibunrConfig->ReadString( "Game", "Name", 0 );
  char* GamePath = NULL;
  char InputBuffer[4096] = { 0 };

  // Print game choices out
  printf("Pick a game:\n");
  while ( GameName != NULL )
  {
    GameNames.Add( GameName );
    printf( "\t(%i) %s\n", i, GameName );
    GameName = GLibunrConfig->ReadString( "Game", "Name", i );
    i++;
  }
  printf( "\t(%i) Add game\n", i );

  // Get input
  char* Result = NULL;
  while ( Result == NULL )
    Result = fgets( InputBuffer, 2, stdin );

  int Choice = atoi( InputBuffer );
  if ( Choice == i )
  {
    // Prompt for a new game
    InputBuffer[0] = '\0';
    InputBuffer[1] = '\0';

    Result = NULL;
    while ( Result == NULL )
    {
      printf( "Enter the name of the game executable (i.e.; Unreal or DeusEx): " );
      Result = fgets( NameOut, GPC_NAME_BUF_LEN, stdin );
    }

    char* RemoveNewline = strchr( NameOut, '\n' );
    if ( RemoveNewline )
      *RemoveNewline = '\0';

    // Get path name
    Result = NULL;
    while ( Result == NULL )
    {
      printf( "Enter the absolute path of the game: " );
      Result = fgets( PathOut, GPC_PATH_BUF_LEN, stdin );
    }

    RemoveNewline = strchr( PathOut, '\n' );
    if ( RemoveNewline )
      *RemoveNewline = '\0';
  }
  else
  {
    // Get game name and path
    GameName = GameNames[Choice-1];
    GamePath = GLibunrConfig->ReadString( "Game", "Name", Choice-1 );

    // Copy them
    strncpy( GamePath, PathOut, GPC_PATH_BUF_LEN );
    strncpy( GameName, NameOut, GPC_NAME_BUF_LEN );

    // Free everything up
    for ( int i = 0; i < GameNames.Size(); i++ )
      xstl::Free( GameNames[i] );
    
    xstl::Free( GamePath );
  }
}

DECLARE_UCC_COMMAND( batchclassexport );
DECLARE_UCC_COMMAND( StripSource );

CommandHandler GetCommandFunction( char* CmdName )
{
  Array<UccCommand*> Commands;

  #define APPEND_COMMAND( name ) \
    Commands.Add( name##Command )

  APPEND_COMMAND( batchclassexport );
  
  for ( int i = 0; i < Commands.Size(); i++ )
    if ( strnicmp( Commands[i], CmdName ) == 0 )
      return Commands[i].Func;
}

int main( int argc, char** argv )
{
  int ReturnCode = 0;

  printf("======================================\n");
  printf("lucc: libunr UCC\n");
  printf("Written by Adam 'Xaleros' Smith\n");
  printf("======================================\n\n");

  if ( argc < 2 )
    PrintHelpAndExit();

  // Check command argument
  CommandHandler Cmd = GetCommandFunction( argv[1] );

  // Check the command and run it
  if ( Cmd == NULL )
  {
    printf("Unknown command '%s'\n", Cmd );
    ReturnCode = ERR_UNKNOWN_CMD;
  }
  else
  {
    if ( !LibunrInit( GamePromptHandler, NULL ) )
    {
      printf("libunr init failed; exiting\n");
      ReturnCode = ERR_LIBUNR_INIT;
    }
   
    ReturnCode = Cmd( argc, argv );
  }
}

