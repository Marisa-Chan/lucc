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
 * ClassExport.cpp - Exports all class files in a package to .uc files
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#include "lucc.h"

int objectexport( int argc, char** argv )
{
  int i = 0;
  char* ClassType = NULL;

  // Argument parsing
  while ( 1 )
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf( "objectexport usage:\n" );
      printf( "\tlucc [gopts] objectexport [copts] <Package Name>\n\n" );

      printf( "Command options:\n" );
      printf( "\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n" );
      printf( "\t-s \"<ObjectName>\"   - Specifies a {s}ingle object to export\n" );
      printf( "\t-t \"<ExportType>\"   - Specifies the extension type of the exported object\n" );
      printf( "\t-c \"<ClassType>\"    - Specifies the class type of the object to export\n");
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
      case 't':
        ExportType = argv[++i];
        break;
      case 'c':
        ClassType = argv[++i];
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

  if ( SingleObject == NULL )
  {
    GLogf( LOG_CRIT, "Need to specify an object with -s" );
    goto BadOpt;
  }

  if ( Path[0] == '\0' )
  {
    // Make a folder inside of the game folder (like original UCC)
    strcat( Path, "../" );
    strcat( Path, PkgName );
  }

  if ( !USystem::MakeDir( Path ) )
  {
    GLogf( LOG_CRIT, "Failed to create output folder '%s'", Path );
    return ERR_BAD_PATH;
  }

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    GLogf( LOG_CRIT, "Failed to open package '%s'; file does not exist" );
    return ERR_MISSING_PKG;
  }

  // Iterate and export all class scripts
  TArray<FExport>& ExportTable = Pkg->GetExportTable();
  for ( int i = 0; i < ExportTable.Size(); i++ )
  {
    FExport* Export = &ExportTable[i];
    const char* ObjName = Pkg->ResolveNameFromIdx( Export->ObjectName );

    // Why are there 'None' exports at all???
    if ( strnicmp( ObjName, "None", 4 ) != 0 )
    {
      // Check if object matches the one we want (if needed)
      if ( stricmp( ObjName, SingleObject ) != 0 )
        continue;

      // Check class type
      UClass* Class = NULL;
      if ( ClassType != NULL && Export->Class != 0 )
      {
        Class = (UClass*)UObject::StaticLoadObject( Pkg, Export->Class, UClass::StaticClass(), NULL, LOAD_Immediate );
        if ( stricmp( Class->Name.Data(), ClassType ) != 0 )
          continue;
      }

      // Load object
      UObject* Obj = UObject::StaticLoadObject( Pkg, Export, NULL , NULL, LOAD_Immediate );
      if ( !Obj )
      {
        GLogf( LOG_CRIT, "Failed to load object '%s.%s'\n", PkgName, ObjName );
        return ERR_BAD_OBJECT;
      }

      // Export
      if ( !UExporter::ExportObject( Obj, Path, ExportType ) )
      {
        GLogf( LOG_CRIT, "Could not export object '%s.%s'\n", PkgName, ObjName );
        return ERR_EXPORT_FAILED;
      }
    }
  }

  return 0;
}