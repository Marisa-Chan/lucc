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
 * SoundExport.cpp - Dumps sound files from packages
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#include "lucc.h"

int soundexport( int argc, char** argv )
{
  int i = 0;
  bool bExportToUCCFolder = false;
  bool bUseGroupPath = false;
  bool bDoGroupPathExport = false;

  // Argument parsing
  while ( 1 )
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf( "soundexport usage:\n" );
      printf( "\tlucc [gopts] soundexport [copts] <Package Name>\n\n" );

      printf( "Command options:\n" );
      printf( "\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n" );
      printf( "\t-s \"<ObjectName>\"   - Specifies a (s)ingle object to export\n" );
      printf( "\t-c                    - Let path point to a folder UCC can see\n" );
      printf( "\t-g                    - Exports objects to folders based on Group\n" );
      printf( "\n" );
      return ERR_BAD_ARGS;
    }

    if ( argv[i][0] == '-' )
    {
      switch ( argv[i][1] )
      {
      case 'p':
        // Get the path relative to our original working directory
#ifdef _WIN32
        if ( strchr( argv[i + 1], ':' ) == NULL )
#endif
        {
          strcpy( Path, wd );
          strcat( Path, "/" );
        }
        strcat( Path, argv[++i] );
        break;
      case 's':
        SingleObject = argv[++i];
        break;
      case 'c':
        bExportToUCCFolder = true;
        break;
      case 'g':
        bUseGroupPath = true;
        break;
      default:
        GLogf( LOG_WARN, "Bad option '%s'", argv[i] );
        goto BadOpt;
      }
    }
    else
    {
      PkgName = argv[i];
      break;
    }

    i++;
  }

  if ( Path[0] == '\0' )
  {
    strcat( Path, "../" );
    if ( bExportToUCCFolder )
    {
      strcat( Path, PkgName );
      strcat( Path, "/Sounds" );
    }
    else
    {
      strcat( Path, "Sounds/" );
      strcat( Path, PkgName );
    }
  }

  if ( !USystem::MakeDir( Path ) )
  {
    GLogf( LOG_CRIT, "Failed to create output folder '%s'",
      Path );
    return ERR_BAD_PATH;
  }
  UClass* Class = USound::StaticClass();

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    GLogf( LOG_CRIT, "Failed to open package '%s'; file does not exist" );
    return ERR_MISSING_PKG;
  }

  // Iterate and export all sounds
  TArray<FExport>& ExportTable = Pkg->GetExportTable();
  for ( int i = 0; i < ExportTable.Size(); i++ )
  {
    FExport* Export = &ExportTable[i];
    const char* ObjName = Pkg->ResolveNameFromIdx( Export->ObjectName );

    // Why are there 'None' exports at all???
    if ( strnicmp( ObjName, "None", 4 ) != 0 )
    {
      // Check if object matches the one we want (if needed)
      if ( SingleObject != NULL )
        if ( stricmp( ObjName, SingleObject ) != 0 )
          continue;

      // Check class type
      const char* ClassName = Pkg->ResolveNameFromObjRef( Export->Class );
      if ( strnicmp( ClassName, "Sound", 5 ) == 0 )
      {
        USound* Obj = (USound*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, true );
        if ( Obj == NULL )
        {
          GLogf( LOG_CRIT, "Failed to load object '%s'" );
          return ERR_BAD_OBJECT;
        }

        if ( bUseGroupPath )
        {
          const char* GroupName = Pkg->ResolveNameFromObjRef( Export->Group );
          if ( stricmp( GroupName, "None" ) != 0 )
          {
            bDoGroupPathExport = true;
            strcat( Path, "/" );
            strcat( Path, GroupName );
            USystem::MakeDir( Path );
          }
          else
          {
            bDoGroupPathExport = false;
          }
        }

        USoundExporter::ExportObject( Obj, Path, NULL );

        if ( bDoGroupPathExport )
          *strrchr( Path, DIRECTORY_SEPARATOR ) = '\0';
      }
    }
  }
  return 0;
}
