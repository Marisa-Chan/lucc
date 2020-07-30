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
 * FullPkgExport.cpp - Fully exports a package
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#include "lucc.h"

/*-----------------------------------------------------------------------------
 * fullpkgexport helpers
-----------------------------------------------------------------------------*/
struct FAssetPath
{
  u32 TypeHash;
  const char* Path;
};

static const FAssetPath AssetPaths[] =
{
  {SuperFastHashString( "None" ),    "Classes"},
  {SuperFastHashString( "Texture" ), "Textures"},
  {SuperFastHashString( "Sound" ),   "Sounds"},
  {SuperFastHashString( "Music" ),   "Music"},
  {SuperFastHashString( "LodMesh" ), "Models"},
};
#define NUM_ASSET_TYPES (sizeof(AssetPaths)/sizeof(FAssetPath))

inline char* CreateAssetPath( const FAssetPath& AssetPath, char* BasePath )
{
  static char Path[4096];
  Path[0] = '\0';

  strcat( Path, BasePath );
  strcat( Path, "/" );
  strcat( Path, AssetPath.Path );
  strcat( Path, "/" );

  if ( USystem::FileExists( Path ) )
    return Path;

  if ( !USystem::MakeDir( Path ) )
  {
    GLogf( LOG_ERR, "Could not create path '%s' for full export", Path );
    return NULL;
  }

  return Path;
}

int DoFullPkgExport( UPackage* Pkg, char* Path, bool bUseGroupPath )
{
  bool bDoGroupPathExport = false;
  char* CurrentPath;
  const char* ClassName;
  const char* ObjName;
  u32 ClassHash;
  TArray<FExport>& Exports = Pkg->GetExportTable();

  for ( int i = 0; i < Exports.Size(); i++ )
  {
    FExport* Export = &Exports[i];
    ObjName = Pkg->ResolveNameFromIdx( Export->ObjectName );
    if ( stricmp( ObjName, "None" ) == 0 )
      continue;

    CurrentPath = NULL;
    ClassName = Pkg->ResolveNameFromObjRef( Export->Class );
    ClassHash = SuperFastHashString( ClassName );
    for ( int j = 0; j < NUM_ASSET_TYPES; j++ )
    {
      if ( ClassHash == AssetPaths[j].TypeHash )
      {
        CurrentPath = CreateAssetPath( AssetPaths[j], Path );
        break;
      }
    }

    if ( CurrentPath == NULL )
      continue;

    UObject* Obj = UObject::StaticLoadObject( Pkg, Export, NULL, NULL, LOAD_Immediate );

    if ( bUseGroupPath )
    {
      const char* GroupName = Pkg->ResolveNameFromObjRef( Export->Group );
      if ( stricmp( GroupName, "None" ) != 0 )
      {
        bDoGroupPathExport = true;
        strcat( CurrentPath, "/" );
        strcat( CurrentPath, GroupName );
        USystem::MakeDir( CurrentPath );
      }
      else
      {
        bDoGroupPathExport = false;
      }
    }

    UExporter::ExportObject( Obj, CurrentPath, NULL );

    if ( bDoGroupPathExport )
      *strrchr( CurrentPath, DIRECTORY_SEPARATOR ) = '\0';
  }

  return 0;
}

/*-----------------------------------------------------------------------------
 * fullpkgexport
 * This exports an entire package.
 * Assets go into the following folders based on the export directory
 * - Classes
 * - Textures
 * - Sounds
 * - Music
 * - Models
-----------------------------------------------------------------------------*/
int fullpkgexport( int argc, char** argv )
{
  int i = 0;
  bool bUseGroupPath = false;

  // Argument parsing
  while ( 1 )
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf( "fullpkgexport usage:\n" );
      printf( "\tlucc [gopts] classexport [copts] <Package Name>\n\n" );

      printf( "Command options:\n" );
      printf( "\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n" );
      printf( "\t-g                    - Exports objects to folders based on Group\n" );
      printf( "\n" );
      return ERR_BAD_ARGS;
    }

    if ( argv[i][0] == '-' )
    {
      switch ( argv[i][1] )
      {
      case 'p':
#ifdef _WIN32
        if ( strchr( argv[i + 1], ':' ) == NULL )
#endif
        {
          strcpy( Path, wd );
          strcat( Path, "/" );
        }
        strcat( Path, argv[++i] );
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
    GLogf( LOG_CRIT, "Failed to open package '%s'; file does not exist", PkgName );
    return ERR_MISSING_PKG;
  }

  return DoFullPkgExport( Pkg, Path, bUseGroupPath );
}
