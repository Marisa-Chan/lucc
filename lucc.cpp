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
 * lucc.cpp - Main lucc source file
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

#include "lucc.h"

DECLARE_UCC_COMMAND( classexport );
DECLARE_UCC_COMMAND( textureexport );
DECLARE_UCC_COMMAND( soundexport );
DECLARE_UCC_COMMAND( musicexport );
DECLARE_UCC_COMMAND( meshexport );
DECLARE_UCC_COMMAND( levelexport );
DECLARE_UCC_COMMAND( missingnativefields );
DECLARE_UCC_COMMAND( fullpkgexport );
DECLARE_UCC_COMMAND( objectexport );
DECLARE_UCC_COMMAND( playmusic );
DECLARE_UCC_COMMAND( levelviewer );

char wd[4096]; // Working directory
char Path[4096] = { 0 };
char* PkgName = NULL;
char* SingleObject = NULL;
char* ExportType = NULL;

void PrintHelpAndExit()
{
  printf("Usage:\n");
  printf("\tlucc [gopts] <command> <parameters>\n\n");

  printf("Commands for \"lucc\":\n");
  printf("\tlucc classexport\n");
  printf("\tlucc soundexport\n");
  printf("\tlucc musicexport\n");
  printf("\tlucc textureexport\n");
  printf("\tlucc meshexport\n");
  printf("\tlucc levelexport\n");
  printf("\tlucc missingnativefields\n");
  printf("\tlucc fullpkgexport\n");
  printf("\tlucc objectexport\n");
  printf("\n");
  printf("Engine level tests:\n");
  printf("\t lucc levelviewer\n");
  printf("\t lucc playmusic\n");
  printf("\n");

  printf("Global options:\n");
  printf("\t-g \"<GameName>\"   - Selects the specified game automatically\n");
  printf("\t-v                  - Sets log level to highest verbosity\n");
  printf("\t-l \"<loglevel>\"   - Specifies log verbosity\n");
  printf("\t   Log Levels:\n");
  printf("\t   \"Dev\"   - Development/Debugging log messages\n");
  printf("\t   \"Info\"  - General runtime information\n");
  printf("\t   \"Warn\"  - Warning messages indicating non-fatal failures\n");
  printf("\t   \"Error\" - Errors that may or may not result in a crash\n");
  printf("\t   \"Crit\"  - Critical failures that will most likely result in a crash\n");
  printf("\n");

  exit( ERR_BAD_ARGS );
}

/*-----------------------------------------------------------------------------
 * GamePromptHandler
 * This provides a menu with which to pick a game (if one was not specified)
-----------------------------------------------------------------------------*/
int GamePromptHandler( TArray<char*>* Names )
{
  int i;
  char InputBuffer[4096] = { 0 };

  // Print game choices out
  printf("Pick a game:\n");
  for ( i = 0; i < Names->Size(); i++ )
    printf( "\t(%i) %s\n", i+1, (*Names)[i] );
  
  printf( "\t(%i) Add game\n", ++i );

tryAgain:
  // Get input
  char* Result = NULL;
  while ( Result == NULL )
    Result = fgets( InputBuffer, 4, stdin );

  int Choice = strtol( InputBuffer, NULL, 10 );

  // Input validation
  if ( Choice == 0 || Choice > i )
  {
    InputBuffer[0] = '\0';
    InputBuffer[1] = '\0';
    InputBuffer[2] = '\0';
    printf( "Invalid choice, pick a game: " );
    goto tryAgain;
  }

  // Input is valid
  if ( Choice == i )
  {
    InputBuffer[0] = '\0';
    InputBuffer[1] = '\0';
    InputBuffer[2] = '\0';

    // Prompt for a new game
    Result = NULL;
    while ( Result == NULL )
    {
      printf( "Choose a name for this game entry (i.e.; Unreal226, Un227i, UT436, etc): " );
      Result = fgets( InputBuffer, sizeof( InputBuffer ), stdin );
    }

    char* RemoveNewline = strchr( InputBuffer, '\n' );
    if ( RemoveNewline )
      *RemoveNewline = '\0';

    // Add it to the config
    GLibunrConfig->WriteString( "Game", "Name", InputBuffer, i-1 );

    // Get exe name
    memset( InputBuffer, 0, sizeof( InputBuffer ) );
    Result = NULL;
    while ( Result == NULL )
    {
      printf( "Enter the name of the Executable (i.e.; Unreal, UnrealTournament, DeusEx, etc): " );
      Result = fgets( InputBuffer, sizeof( InputBuffer ), stdin );
    }

    RemoveNewline = strchr( InputBuffer, '\n' );
    if ( RemoveNewline )
      *RemoveNewline = '\0';

    // Add it to the config
    GLibunrConfig->WriteString( "Game", "Exec", InputBuffer, i-1 );

    // Get path name
    memset( InputBuffer, 0, sizeof( InputBuffer ) );
    Result = NULL;
    while ( Result == NULL )
    {
      printf( "Enter the relative path of the game: " );
      Result = fgets( InputBuffer, sizeof( InputBuffer ), stdin );
    }

    RemoveNewline = strchr( InputBuffer, '\n' );
    if ( RemoveNewline )
      *RemoveNewline = '\0';

    // Add it to the config
    GLibunrConfig->WriteString( "Game", "Path", InputBuffer, i-1 );

    GLibunrConfig->Save();
  }
  
  return Choice - 1;
}

CommandHandler GetCommandFunction( char* CmdName )
{
  TArray<UccCommand*> Commands;

  #define APPEND_COMMAND( name ) \
    Commands.PushBack( &name##Command )

  APPEND_COMMAND( classexport );
  APPEND_COMMAND( textureexport );
  APPEND_COMMAND( meshexport );
  APPEND_COMMAND( soundexport );
  APPEND_COMMAND( musicexport );
  APPEND_COMMAND( levelexport );
  APPEND_COMMAND( missingnativefields );
  APPEND_COMMAND( fullpkgexport );
  APPEND_COMMAND( objectexport );
  APPEND_COMMAND( playmusic );
  APPEND_COMMAND( levelviewer );
  
  for ( int i = 0; i < Commands.Size(); i++ )
    if ( stricmp( Commands[i]->Name, CmdName ) == 0 )
      return Commands[i]->Func;

  return NULL;
}

// kind of sloppy...
int StrToLogLevel( char* LogLevelStr )
{
  if ( stricmp( LogLevelStr, "Dev" ) == 0 )
    return LOG_DEV;
  else if ( stricmp( LogLevelStr, "Info" ) == 0 )
    return LOG_INFO;
  else if ( stricmp( LogLevelStr, "Warn" ) == 0 )
    return LOG_WARN;
  else if ( stricmp( LogLevelStr, "Error" ) == 0 )
    return LOG_ERR;
  else if ( stricmp( LogLevelStr, "Crit" ) == 0 )
    return LOG_CRIT;

  return LOG_INFO;
}

int main( int argc, char** argv )
{
  int ReturnCode = 0;
  int LogLevel = LOG_INFO;
  char* GameName = NULL;
  char* CmdName = NULL;

  printf("======================================\n");
  printf("lucc: libunr UCC\n");
  printf("Written by Adam 'Xaleros' Smith\n");
  printf("======================================\n\n");

  int i = 1;
  while (1)
  {
    if ( i == argc )
      break;

    if ( argv[i][0] == '-' )
    {
      switch (argv[i][1])
      {
        case 'g':
          GameName = argv[++i];
          break;
        case 'l':
          LogLevel = StrToLogLevel( argv[++i] );
          break;
        case 'v':
          LogLevel = LOG_DEV;
          break;
        default:
          PrintHelpAndExit();
      }
    }
    else
    {
      CmdName = argv[i];
      break;
    }

    i++;
  }

  if ( i == argc )
    PrintHelpAndExit();

  // Check command argument
  CommandHandler Cmd = GetCommandFunction( CmdName );

  // Check the command and run it
  if ( Cmd == NULL )
  {
    GLogf( LOG_CRIT, "Unknown command '%s'", CmdName );
    PrintHelpAndExit();
  }
  else
  {
    // Preserve the current working directory
    getcwd( wd, sizeof( wd ) );
    GLogFile = new FLogFile();
    GLogFile->Open( "lucc.log" );

    TIMER_DECLARE(libunr_timer);
    TIMER_START(libunr_timer);

    if ( !LibunrInit( GamePromptHandler, NULL, true, GameName ) )
    {
      GLogf( LOG_CRIT, "libunr init failed; exiting");
      ReturnCode = ERR_LIBUNR_INIT;
    }
    else
    {
      ReturnCode = Cmd( argc - i - 1, &argv[i+1] );
      if ( ReturnCode > 0 )
        GLogf( LOG_CRIT, "Command failed");
      else
        GLogf( LOG_INFO, "Command completed successfully");
    }

    TIMER_END(libunr_timer);
    TIMER_PRINT(libunr_timer);

    GLogFile->Close();
  }

  return ReturnCode;
}

