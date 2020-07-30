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
 * MusicExport.cpp - Dumps music files from packages
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#include "lucc.h"

int musicexport( int argc, char** argv )
{
  int i = 0;
  bool bExportToUCCFolder = false;

  // Argument parsing
  while ( 1 )
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf( "musicexport usage:\n" );
      printf( "\tlucc [gopts] musicexport [copts] <Package Name>\n\n" );

      printf( "Command options:\n" );
      printf( "\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n" );
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
    strcat( Path, "../Music/" );

  if ( !USystem::MakeDir( Path ) )
  {
    GLogf( LOG_CRIT, "Failed to create output folder '%s'",
      Path );
    return ERR_BAD_PATH;
  }
  UClass* Class = UMusic::StaticClass();

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    GLogf( LOG_CRIT, "Failed to open package '%s'; file does not exist" );
    return ERR_MISSING_PKG;
  }

  // Iterate and export all music files
  TArray<FExport>& ExportTable = Pkg->GetExportTable();
  for ( int i = 0; i < ExportTable.Size(); i++ )
  {
    FExport* Export = &ExportTable[i];
    const char* ObjName = Pkg->ResolveNameFromIdx( Export->ObjectName );

    // Why are there 'None' exports at all???
    if ( strnicmp( ObjName, "None", 4 ) != 0 )
    {
      // Check class type
      const char* ClassName = Pkg->ResolveNameFromObjRef( Export->Class );
      if ( strnicmp( ClassName, "Music", 5 ) == 0 )
      {
        UMusic* Obj = (UMusic*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, LOAD_Immediate );
        if ( Obj == NULL )
        {
          GLogf( LOG_CRIT, "Failed to load object '%s'", ObjName );
          return ERR_BAD_OBJECT;
        }

        UMusicExporter::ExportObject( Obj, Path, NULL );
      }
    }
  }
  return 0;
}
