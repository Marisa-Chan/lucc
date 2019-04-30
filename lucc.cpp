/*===========================================================================*\
|*  lucc - An open source UCC implementation that makes use of libunr        *|
|*  Copyright (C) 2018-2019  Adam W.E. Smith                                 *|
|*                                                                           *|
|*  This program is free software: you can redistribute it and/or modify     *|
|*  it under the terms of the GNU Affero General Public License as           *|
|*  published by the Free Software Foundation, either version 3 of the       *|
|*  License, or (at your option) any later version.                          *|
|*                                                                           *|
|*  This program is distributed in the hope that it will be useful,          *|
|*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *|
|*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *|
|*  GNU Affero General Public License for more details.                      *|
|*                                                                           *|
|*  You should have received a copy of the GNU Affero General Public License *|
|*  along with this program.  If not, see <https://www.gnu.org/licenses/>.   *|
\*===========================================================================*/

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
#define ERR_BAD_PATH      7

char wd[4096]; // Working directory
char Path[4096] = { 0 };
char* PkgName;
char* SingleObject = NULL;

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
  printf("\tlucc [gopts] <command> <parameters>\n\n");

  printf("Commands for \"lucc\":\n");
  printf("\tlucc classexport\n");
  printf("\tlucc soundexport\n");
  printf("\tlucc musicexport\n");
  printf("\tlucc textureexport\n");
  printf("\tlucc levelexport\n");
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
 * classexport
 * This exports script text to .uc files for any package
-----------------------------------------------------------------------------*/
int classexport( int argc, char** argv )
{
  int i = 0;

  // Argument parsing
  while (1)
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf("classexport usage:\n");
      printf("\tlucc [gopts] classexport [copts] <Package Name>\n\n");

      printf("Command options:\n");
      printf("\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n");
      printf("\t-s \"<ObjectName>\"   - Specifies a {s}ingle object to export\n");
      printf("\n");
      return ERR_BAD_ARGS;
    }

    if ( argv[i][0] == '-' )
    {
      switch (argv[i][1])
      {
        case 'p':
          // Get the path relative to our original working directory
          strcpy( Path, wd );
          strcat( Path, "/" );
          strcat( Path, argv[++i] );
          break;
        case 's':
          SingleObject = argv[++i];
          break;
        default:
          Logf( LOG_WARN, "Bad option '%s'", argv[i] );
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
    strcat( Path, "/Classes" );
  }

  if ( !USystem::MakeDir( Path ) )
  {
    Logf( LOG_CRIT, "Failed to create output folder '%s'", Path );
    return ERR_BAD_PATH;
  }
  UClass* Class = UClass::StaticClass();

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist" );
    return ERR_MISSING_PKG;
  }

  // Iterate and export all class scripts
  Array<FExport>* ExportTable = Pkg->GetExportTable();
  for ( int i = 0; i < ExportTable->Size(); i++ )
  {
    FExport* Export = &(*ExportTable)[i];
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
      if ( strnicmp( ClassName, "None", 4 ) == 0 )
      {
        UClass* Obj = (UClass*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, true );
        if ( Obj == NULL )
        {
          Logf( LOG_CRIT, "Failed to load object '%s'");
          return ERR_BAD_OBJECT;
        }
        
        Obj->ExportToFile( Path, NULL );
      }
    }
  }

  return 0;
}

/*-----------------------------------------------------------------------------
 * textureexport
 * This exports textures to image files for any package
-----------------------------------------------------------------------------*/
int textureexport( int argc, char** argv )
{
  int i = 0;
  bool bExportToUCCFolder = false;

  // Argument parsing
  while (1)
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf("textureexport usage:\n");
      printf("\tlucc [gopts] textureexport [copts] <Package Name>\n\n");

      printf("Command options:\n");
      printf("\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n");
      printf("\t-s \"<ObjectName>\"   - Specifies a (s)ingle object to export\n");
      printf("\t-c                    - Let path point to a folder UCC can see\n");
      printf("\n");
      return ERR_BAD_ARGS;
    }

    if ( argv[i][0] == '-' )
    {
      switch (argv[i][1])
      {
        case 'p':
          // Get the path relative to our original working directory
          strcpy( Path, wd );
          strcat( Path, "/" );
          strcat( Path, argv[++i] );
          break;
        case 's':
          SingleObject = argv[++i];
          break;
        case 'c':
          bExportToUCCFolder = true;
          break;
        default:
          Logf( LOG_WARN, "Bad option '%s'", argv[i] );
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
      strcat( Path, "/Textures" );
    }
    else
    {
      strcat( Path, "Textures/" );
      strcat( Path, PkgName );
    }
  }

  if ( !USystem::MakeDir( Path ) )
  {
    Logf( LOG_CRIT, "Failed to create output folder '%s'",
          Path );
    return ERR_BAD_PATH;
  }
  UClass* Class = UTexture::StaticClass();

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist" );
    return ERR_MISSING_PKG;
  }

  // Iterate and export all textures
  Array<FExport>* ExportTable = Pkg->GetExportTable();
  for ( int i = 0; i < ExportTable->Size(); i++ )
  {
    FExport* Export = &(*ExportTable)[i];
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
      if ( strnicmp( ClassName, "Texture", 7 ) == 0 )
      {
        UTexture* Obj = (UTexture*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, true );
        if ( Obj == NULL )
        {
          Logf( LOG_CRIT, "Failed to load object '%s'");
          return ERR_BAD_OBJECT;
        }
        
        Obj->ExportToFile( Path, "bmp" );
      }
    }
  }

  return 0;
}

/*-----------------------------------------------------------------------------
 * soundexport
 * This exports sounds to audio files for any package
-----------------------------------------------------------------------------*/
int soundexport( int argc, char** argv )
{
  int i = 0;
  bool bExportToUCCFolder = false;

  // Argument parsing
  while (1)
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf("soundexport usage:\n");
      printf("\tlucc [gopts] soundexport [copts] <Package Name>\n\n");

      printf("Command options:\n");
      printf("\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n");
      printf("\t-s \"<ObjectName>\"   - Specifies a (s)ingle object to export\n");
      printf("\t-c                    - Let path point to a folder UCC can see\n");
      printf("\n");
      return ERR_BAD_ARGS;
    }

    if ( argv[i][0] == '-' )
    {
      switch (argv[i][1])
      {
        case 'p':
          // Get the path relative to our original working directory
          strcpy( Path, wd );
          strcat( Path, "/" );
          strcat( Path, argv[++i] );
          break;
        case 's':
          SingleObject = argv[++i];
          break;
        case 'c':
          bExportToUCCFolder = true;
          break;
        default:
          Logf( LOG_WARN, "Bad option '%s'", argv[i] );
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
    Logf( LOG_CRIT, "Failed to create output folder '%s'",
          Path );
    return ERR_BAD_PATH;
  }
  UClass* Class = USound::StaticClass();

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist" );
    return ERR_MISSING_PKG;
  }

  // Iterate and export all sounds
  Array<FExport>* ExportTable = Pkg->GetExportTable();
  for ( int i = 0; i < ExportTable->Size(); i++ )
  {
    FExport* Export = &(*ExportTable)[i];
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
          Logf( LOG_CRIT, "Failed to load object '%s'");
          return ERR_BAD_OBJECT;
        }
        
        Obj->ExportToFile( Path, NULL );
      }
    }
  }
  return 0;
}

/*-----------------------------------------------------------------------------
 * musicexport
 * This exports music to their respective file formats for any package
-----------------------------------------------------------------------------*/
int musicexport( int argc, char** argv )
{
  int i = 0;
  bool bExportToUCCFolder = false;

  // Argument parsing
  while (1)
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf("musicexport usage:\n");
      printf("\tlucc [gopts] musicexport [copts] <Package Name>\n\n");

      printf("Command options:\n");
      printf("\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n");
      printf("\n");
      return ERR_BAD_ARGS;
    }

    if ( argv[i][0] == '-' )
    {
      switch (argv[i][1])
      {
        case 'p':
          // Get the path relative to our original working directory
          strcpy( Path, wd );
          strcat( Path, "/" );
          strcat( Path, argv[++i] );
          break;
        default:
          Logf( LOG_WARN, "Bad option '%s'", argv[i] );
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
    Logf( LOG_CRIT, "Failed to create output folder '%s'",
          Path );
    return ERR_BAD_PATH;
  }
  UClass* Class = UMusic::StaticClass();

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist" );
    return ERR_MISSING_PKG;
  }

  // Iterate and export all music files
  Array<FExport>* ExportTable = Pkg->GetExportTable();
  for ( int i = 0; i < ExportTable->Size(); i++ )
  {
    FExport* Export = &(*ExportTable)[i];
    const char* ObjName = Pkg->ResolveNameFromIdx( Export->ObjectName );

    // Why are there 'None' exports at all???
    if ( strnicmp( ObjName, "None", 4 ) != 0 )
    {
      // Check class type
      const char* ClassName = Pkg->ResolveNameFromObjRef( Export->Class );
      if ( strnicmp( ClassName, "Music", 5 ) == 0 )
      {
        UMusic* Obj = (UMusic*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, true );
        if ( Obj == NULL )
        {
          Logf( LOG_CRIT, "Failed to load object '%s'");
          return ERR_BAD_OBJECT;
        }
        
        Obj->ExportToFile( Path, NULL );
      }
    }
  }
  return 0;
}

/*-----------------------------------------------------------------------------
 * fullpkgexport helpers
-----------------------------------------------------------------------------*/
struct FAssetPath
{
  FHash TypeHash;
  const char* Path;
};

static const FAssetPath AssetPaths[] = 
{
  {FnvHashString("None"),    "Classes"},
  {FnvHashString("Texture"), "Textures"},
  {FnvHashString("Sound"),   "Sounds"},
  {FnvHashString("Music"),   "Music"},
  {FnvHashString("LodMesh"), "Meshes"},
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
    Logf( LOG_ERR, "Could not create path '%s' for full export", Path );
    return NULL;
  }
  
  return Path;
}

int DoFullPkgExport( UPackage* Pkg, char* Path )
{
  char* CurrentPath;
  const char* ClassName;
  const char* ObjName;
  FHash ClassHash;
  Array<FExport>* Exports = Pkg->GetExportTable();

  for ( int i = 0; i < Exports->Size(); i++ )
  {
    FExport* Export = &(*Exports)[i];
    ObjName = Pkg->ResolveNameFromIdx( Export->ObjectName );
    if ( stricmp( ObjName, "None" ) == 0 )
      continue;

    CurrentPath = NULL;
    ClassName = Pkg->ResolveNameFromObjRef( Export->Class );
    ClassHash = FnvHashString( ClassName );
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

    UObject* Obj = UObject::StaticLoadObject( Pkg, Export, NULL, NULL, true );
    Obj->ExportToFile( CurrentPath, NULL );
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
 * - Meshes
-----------------------------------------------------------------------------*/
int fullpkgexport( int argc, char** argv )
{
  int i = 0;

  // Argument parsing
  while (1)
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf("classexport usage:\n");
      printf("\tlucc [gopts] classexport [copts] <Package Name>\n\n");

      printf("Command options:\n");
      printf("\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n");
      printf("\n");
      return ERR_BAD_ARGS;
    }

    if ( argv[i][0] == '-' )
    {
      switch (argv[i][1])
      {
        case 'p':
          // Get the path relative to our original working directory
          strcpy( Path, wd );
          strcat( Path, "/" );
          strcat( Path, argv[++i] );
          break;
        default:
          Logf( LOG_WARN, "Bad option '%s'", argv[i] );
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
    Logf( LOG_CRIT, "Failed to create output folder '%s'", Path );
    return ERR_BAD_PATH;
  }

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist" );
    return ERR_MISSING_PKG;
  }

  return DoFullPkgExport( Pkg, Path );
}

/*-----------------------------------------------------------------------------
 * levelexport
 * This exports levels to .T3D files for any package with map data
-----------------------------------------------------------------------------*/
int levelexport( int argc, char** argv )
{
  int i = 0;
  bool bExportMyLevelAssets = false;

  // Argument parsing
  while (1)
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf("levelexport usage:\n");
      printf("\tlucc [gopts] musicexport [copts] <Package Name>\n\n");

      printf("Command options:\n");
      printf("\t-p \"<ExportPath>\"   - Specifies a folder (p)ath to export to\n");
      printf("\t-m                    - Exports all (m)yLevel assets as well as a t3d file\n");
      printf("\n");
      return ERR_BAD_ARGS;
    }

    if ( argv[i][0] == '-' )
    {
      switch (argv[i][1])
      {
        case 'p':
          // Get the path relative to our original working directory
          strcpy( Path, wd );
          strcat( Path, "/" );
          strcat( Path, argv[++i] );
          break;
        case 'm':
          bExportMyLevelAssets = true;
          break;
        default:
          Logf( LOG_WARN, "Bad option '%s'", argv[i] );
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
    strcat( Path, "../Maps/" );
    if ( bExportMyLevelAssets )
      strcat( Path, PkgName );
  }

  if ( !USystem::MakeDir( Path ) )
  {
    Logf( LOG_CRIT, "Failed to create output folder '%s'",
          Path );
    return ERR_BAD_PATH;
  }

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist", PkgName );
    return ERR_MISSING_PKG;
  }

  Logf( LOG_INFO, "Running levelexport on package '%s' to '%s'", PkgName, Path );

  // Load level object and export
  ULevel* Level = (ULevel*)UObject::StaticLoadObject( Pkg, "MyLevel", ULevel::StaticClass(), NULL );
  Level->ExportToFile( Path, NULL );

  if ( bExportMyLevelAssets )
    DoFullPkgExport( Pkg, Path );

  return 0;
}

/*-----------------------------------------------------------------------------
 * missingnativefields helpers
-----------------------------------------------------------------------------*/
const char* const CppPropNames[] =
{
  "None",
  "u8",
  "int",
  "bool",
  "float",
  "UObject*",
  "FName",
  "FString*",
  "UClass*",
  "Array",
  "UStruct*",
  "FVector",
  "FRotator",
  "FString*",
  "Map",
  "???",
  "u64"
};

char* GetCppClassName( UClass* Class )
{
  // Not thread-safe
  static char ClassName[128] = { 0 };
  xstl::Set( ClassName, 0, sizeof(ClassName) );
  
  if ( Class->ClassIsA( AActor::StaticClass() ) )
    ClassName[0] = 'A';
  else
    ClassName[0] = 'U';
  
  strcat( ClassName, Class->Name.Data() );
  return ClassName;
}

char* GetCppClassNameProp( UProperty* Prop )
{
  UObjectProperty* ObjProp = (UObjectProperty*)Prop;
  char* ClassName = GetCppClassName( ObjProp->ObjectType );
  strcat( ClassName, "*" );
  return ClassName;
}

char* GetCppArrayType( UProperty* Prop )
{
  // Not thread-safe
  static char ArrayName[128] = { 0 };
  xstl::Set( ArrayName, 0, sizeof(ArrayName) );

  UArrayProperty* ArrayProp = (UArrayProperty*)Prop;
  if ( ArrayProp->Inner->PropertyType == PROP_Object )
    return GetCppClassNameProp( ArrayProp->Inner );
  else if ( ArrayProp->Inner->PropertyType == PROP_Struct )
  {
    ArrayName[0] = 'F';
    strcat( ArrayName, ((UStructProperty*)ArrayProp->Inner)->Struct->Name.Data() );
    return ArrayName;
  }
  
  return (char*)CppPropNames[ArrayProp->Inner->PropertyType];
}

/*-----------------------------------------------------------------------------
 * missingnativefields
 * This reports any missing properties and prints them in a way that can
 * be pasted to C++ code
-----------------------------------------------------------------------------*/
int missingnativefields( int argc, char** argv )
{
  if ( argc < 1 )
  {
    printf("levelexport usage:\n");
    printf("\tlucc missingnativefields <Package Name>\n\n");
    return ERR_BAD_ARGS;
  }
  
  char* PkgName = argv[0];

  // Load package
  USystem::LogLevel = LOG_CRIT;
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist\n" );
    return ERR_MISSING_PKG;
  }
  
  // Iterate and load all classes
  Array<FExport>* ExportTable = Pkg->GetExportTable();
  for ( int i = 0; i < ExportTable->Size(); i++ )
  {
    FExport* Export = &(*ExportTable)[i];
    const char* ObjName = Pkg->ResolveNameFromIdx( Export->ObjectName );

    // Why are there 'None' exports at all???
    if ( strnicmp( ObjName, "None", 4 ) != 0 )
    {
      // Check class type
      const char* ClassName = Pkg->ResolveNameFromObjRef( Export->Class );
      if ( strnicmp( ClassName, "None", 4 ) == 0 )
      {
        UClass* Class = (UClass*)UObject::StaticLoadObject( Pkg, Export, UClass::StaticClass(), NULL, true );
        if ( !Class )
        {
          Logf( LOG_CRIT, "Failed to load object '%s'\n", ClassName );
          return ERR_BAD_OBJECT;
        }

        // Iterate through all class properties for dumping cpp class text
        int NumMissing = 0;
        for ( UField* It = Class->Children; It != NULL; It = It->Next )
        {
          UProperty* Prop = SafeCast<UProperty>( It );
          if ( Prop && Prop->Offset == MAX_UINT32 && Prop->Outer == Class )
          {
            if ( NumMissing == 0 )
            {
              printf("//====================================================================\n");
              printf("// Missing native fields for class '%s'\n", Class->Name.Data());
              printf("//====================================================================\n");
            }

            if ( Prop->PropertyType == PROP_Struct )
              printf("  F%s %s", ((UStructProperty*)Prop)->Struct->Name.Data(), Prop->Name.Data() );
            else if ( Prop->PropertyType == PROP_Object )
              printf("  %s %s", GetCppClassNameProp( Prop ), Prop->Name.Data() );
            else if ( Prop->PropertyType == PROP_Array )
              printf("  Array<%s>* %s", GetCppArrayType( Prop ), Prop->Name.Data() );
            else
              printf("  %s %s", CppPropNames[Prop->PropertyType], Prop->Name.Data() );

            if ( Prop->ArrayDim > 1 )
              printf("[%i]", Prop->ArrayDim );
            printf(";\n");

            NumMissing++;
          }
        }

        if ( NumMissing == 0 )
          continue;

        // Iterate again, but this time spit out LINK_NATIVE_PROPERTY stuff
        printf("BEGIN_PROPERTY_LINK( %s, %i )\n", GetCppClassName( Class ), NumMissing);
        for ( UField* It = Class->Children; It != NULL; It = It->Next )
        {
          UProperty* Prop = SafeCast<UProperty>( It );
          if ( Prop && Prop->Offset == MAX_UINT32 && Prop->Outer == Class )
            printf("  LINK_NATIVE_PROPERTY( %s );\n", Prop->Name.Data() );
        }
        printf("END_PROPERTY_LINK()\n");
      }
    }
  }

  return 0;  
}

/*-----------------------------------------------------------------------------
 * GamePromptHandler
 * This provides a menu with which to pick a game (if one was not specified)
-----------------------------------------------------------------------------*/
int GamePromptHandler( Array<char*>* Names )
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
    xstl::Set( InputBuffer, 0, sizeof( InputBuffer ) );
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
    xstl::Set( InputBuffer, 0, sizeof( InputBuffer ) );
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

DECLARE_UCC_COMMAND( classexport );
DECLARE_UCC_COMMAND( textureexport );
DECLARE_UCC_COMMAND( soundexport );
DECLARE_UCC_COMMAND( musicexport );
DECLARE_UCC_COMMAND( levelexport );
DECLARE_UCC_COMMAND( missingnativefields );
DECLARE_UCC_COMMAND( fullpkgexport );

CommandHandler GetCommandFunction( char* CmdName )
{
  Array<UccCommand*> Commands;

  #define APPEND_COMMAND( name ) \
    Commands.PushBack( &name##Command )

  APPEND_COMMAND( classexport );
  APPEND_COMMAND( textureexport );
  APPEND_COMMAND( soundexport );
  APPEND_COMMAND( musicexport );
  APPEND_COMMAND( levelexport );
  APPEND_COMMAND( missingnativefields );
  APPEND_COMMAND( fullpkgexport );
  
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
    Logf( LOG_CRIT, "Unknown command '%s'", CmdName );
    PrintHelpAndExit();
  }
  else
  {
    // Preserve the current working directory
    getcwd( wd, sizeof( wd ) );
    CreateLogFile( "lucc.log" );

    if ( !LibunrInit( GamePromptHandler, NULL, true, GameName ) )
    {
      Logf( LOG_CRIT, "libunr init failed; exiting");
      ReturnCode = ERR_LIBUNR_INIT;
    }
    else
    {
      ReturnCode = Cmd( argc - i, &argv[i+1] );
      if ( ReturnCode > 0 )
        Logf( LOG_CRIT, "Command failed");
      else
        Logf( LOG_INFO, "Command completed successfully");
    }

    CloseLogFile();
  }

  return ReturnCode;
}

