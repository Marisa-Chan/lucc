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

// Working directory
char wd[4096];

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
  printf("\tlucc levelexport\n");

  exit( ERR_BAD_ARGS );
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
  char  Path[4096];

  // Get the path relative to our original working directory
  strcpy( Path, wd );
  strcat( Path, "/");
  strcat( Path, argv[3] );

  if ( !USystem::MakeDir( Path ) )
  {
    Logf( LOG_CRIT, "Failed to create output folder '%s'",
          Path );
    return ERR_BAD_PATH;
  }
  UClass* Class = UClass::StaticClass();

  // Load package
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist\n" );
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
      // Check class type
      const char* ClassName = Pkg->ResolveNameFromObjRef( Export->Class );
      if ( strnicmp( ClassName, "None", 4 ) == 0 )
      {
        printf( "Exporting %s.uc\n", ObjName );
 
        UClass* Obj = (UClass*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, true );
        if ( Obj == NULL )
        {
          Logf( LOG_CRIT, "Failed to load object '%s'\n");
          return ERR_BAD_OBJECT;
        }
        
        Obj->ExportToFile( Path, NULL );
      }
    }
  }

  return 0;
}

int batchtextureexport( int argc, char** argv )
{
  if ( argc < 4 )
  {
    printf("batchtextureexport usage:\n");
    printf("\tlucc batchtextureexport <Package Name> <Export Path>\n\n");
    return ERR_BAD_ARGS;
  }

  char* PkgName = argv[2];
  char  Path[4096];

  // Get the path relative to our original working directory
  strcpy( Path, wd );
  strcat( Path, "/");
  strcat( Path, argv[3] );

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
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist\n" );
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
      // Check class type
      const char* ClassName = Pkg->ResolveNameFromObjRef( Export->Class );
      if ( strnicmp( ClassName, "Texture", 7 ) == 0 )
      {
        printf( "Exporting %s.bmp\n", ObjName );
 
        UTexture* Obj = (UTexture*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, true );
        if ( Obj == NULL )
        {
          Logf( LOG_CRIT, "Failed to load object '%s'\n");
          return ERR_BAD_OBJECT;
        }
        
        Obj->ExportToFile( Path, "bmp" );
      }
    }
  }

  return 0;
}

int batchsoundexport( int argc, char** argv )
{
  if ( argc < 4 )
  {
    printf("batchsoundexport usage:\n");
    printf("\tlucc batchsoundexport <Package Name> <Export Path>\n\n");
    return ERR_BAD_ARGS;
  }

  char* PkgName = argv[2];
  char  Path[4096];

  // Get the path relative to our original working directory
  strcpy( Path, wd );
  strcat( Path, "/");
  strcat( Path, argv[3] );

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
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist\n" );
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
      // Check class type
      const char* ClassName = Pkg->ResolveNameFromObjRef( Export->Class );
      if ( strnicmp( ClassName, "Sound", 5 ) == 0 )
      {
        printf( "Exporting %s.wav\n", ObjName );
 
        USound* Obj = (USound*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, true );
        if ( Obj == NULL )
        {
          Logf( LOG_CRIT, "Failed to load object '%s'\n");
          return ERR_BAD_OBJECT;
        }
        
        Obj->ExportToFile( Path, NULL );
      }
    }
  }
  return 0;
}

int batchmusicexport( int argc, char** argv )
{
  if ( argc < 4 )
  {
    printf("batchmusicexport usage:\n");
    printf("\tlucc batchmusicexport <Package Name> <Export Path>\n\n");
    return ERR_BAD_ARGS;
  }

  char* PkgName = argv[2];
  char  Path[4096];

  // Get the path relative to our original working directory
  strcpy( Path, wd );
  strcat( Path, "/");
  strcat( Path, argv[3] );

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
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist\n" );
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
        printf( "Exporting %s\n", ObjName );
 
        UMusic* Obj = (UMusic*)UObject::StaticLoadObject( Pkg, Export, Class, NULL, true );
        if ( Obj == NULL )
        {
          Logf( LOG_CRIT, "Failed to load object '%s'\n");
          return ERR_BAD_OBJECT;
        }
        
        Obj->ExportToFile( Path, NULL );
      }
    }
  }
  return 0;
}

int levelexport( int argc, char** argv )
{
  if ( argc < 4 )
  {
    printf("levelexport usage:\n");
    printf("\tlucc levelexport <Package Name> <Export Path>\n\n");
    return ERR_BAD_ARGS;
  }

  char* PkgName = argv[2];
  char  Path[4096];

  // Get the path relative to our original working directory
  strcpy( Path, wd );
  strcat( Path, "/");
  strcat( Path, argv[3] );

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
    Logf( LOG_CRIT, "Failed to open package '%s'; file does not exist\n" );
    return ERR_MISSING_PKG;
  }

  // Load level object and export
  ULevel* Level = (ULevel*)UObject::StaticLoadObject( Pkg, "MyLevel", ULevel::StaticClass(), NULL );
  Level->ExportToFile( Path, NULL );

  return 0;
}

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

int missingnativefields( int argc, char** argv )
{
  if ( argc < 3 )
  {
    printf("levelexport usage:\n");
    printf("\tlucc missingnativefields <Package Name>\n\n");
    return ERR_BAD_ARGS;
  }
  
  char* PkgName = argv[2];

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
    Result = fgets( InputBuffer, 3, stdin );

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

DECLARE_UCC_COMMAND( batchclassexport );
DECLARE_UCC_COMMAND( batchtextureexport );
DECLARE_UCC_COMMAND( batchsoundexport );
DECLARE_UCC_COMMAND( batchmusicexport );
DECLARE_UCC_COMMAND( levelexport );
DECLARE_UCC_COMMAND( missingnativefields );

CommandHandler GetCommandFunction( char* CmdName )
{
  Array<UccCommand*> Commands;

  #define APPEND_COMMAND( name ) \
    Commands.PushBack( &name##Command )

  APPEND_COMMAND( batchclassexport );
  APPEND_COMMAND( batchtextureexport );
  APPEND_COMMAND( batchsoundexport );
  APPEND_COMMAND( batchmusicexport );
  APPEND_COMMAND( levelexport );
  APPEND_COMMAND( missingnativefields );
  
  for ( int i = 0; i < Commands.Size(); i++ )
    if ( stricmp( Commands[i]->Name, CmdName ) == 0 )
      return Commands[i]->Func;

  return NULL;
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
    printf("Unknown command '%s'\n", argv[1] );
    ReturnCode = ERR_UNKNOWN_CMD;
  }
  else
  {
    // Preserve the current working directory
    getcwd( wd, sizeof( wd ) );

    if ( !LibunrInit( GamePromptHandler, NULL, true ) )
    {
      printf("libunr init failed; exiting\n");
      ReturnCode = ERR_LIBUNR_INIT;
    }
    else
    {
      ReturnCode = Cmd( argc, argv );
      if ( ReturnCode > 0 )
        printf("Command failed\n");
      else
        printf("Command completed successfully\n");
    }
  }

  return ReturnCode;
}

