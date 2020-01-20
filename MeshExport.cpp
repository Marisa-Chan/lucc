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
 * MeshExport.cpp - Exports meshes to a range of file formats
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#include "lucc.h"

int meshexport( int argc, char** argv )
{
  int i = 0;
  bool bExportToUCCFolder = false;
  bool bUseGroupPath = false;
  bool bDoGroupPathExport = false;
  char* MeshType = NULL;
  int FrameNum = -1;

  // Argument parsing
  while ( 1 )
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf( "meshexport usage:\n" );
      printf( "\tlucc [gopts] meshexport [copts] <Package Name>\n\n" );

      printf( "Command options:\n" );
      printf( "\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n" );
      printf( "\t-s \"<ObjectName>\"   - Specifies a (s)ingle object to export\n" );
      printf( "\t-c                    - Let path point to a folder UCC can see\n" );
      printf( "\t-g                    - Exports objects to folders based on (g)roup\n" );
      printf( "\t-f \"<FrameNum>\"     - Specifies a (f)rame number to export (for .obj)\n" );
      printf( "\t-t \"<MeshFormat>\"   - Specifies a mesh (t)ype to export to (default to u3d)\n" );
      printf( "\t   Mesh Formats:\n" );
      printf( "\t   \"u3d\"  - Unreal Vertex Mesh Format (_a.3d/_d.3d)\n" );
      printf( "\t   \"obj\"  - Waveform Obj Format\n" );
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
      case 'f':
        FrameNum = strtol( argv[++i], NULL, 0 );
        break;
      case 't':
        MeshType = argv[++i];
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
      // Make a folder inside of the game folder (like original UCC)
      strcat( Path, PkgName );
      strcat( Path, "/Models" );
    }
    else
    {
      strcat( Path, "Models/" );
      strcat( Path, PkgName );
    }
  }

  if ( !USystem::MakeDir( Path ) )
  {
    GLogf( LOG_CRIT, "Failed to create output folder '%s'",
      Path );
    return ERR_BAD_PATH;
  }
  UClass* Class = UMesh::StaticClass();

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    GLogf( LOG_CRIT, "Failed to open package '%s'; file does not exist" );
    return ERR_MISSING_PKG;
  }

  // Iterate and export all textures
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
      if ( strstr( ClassName, "Mesh" ) != 0 )
      {
        UMesh* Obj = (UMesh*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, true );
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

        Obj->ExportToFile( Path, MeshType, FrameNum );

        if ( bDoGroupPathExport )
          *strrchr( Path, DIRECTORY_SEPARATOR ) = '\0';
      }
    }
  }

  return 0;
}
