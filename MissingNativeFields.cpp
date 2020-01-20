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
 * MissingNativeFields.cpp - Checks classes for missing native variables
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#include "lucc.h"

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
  memset( ClassName, 0, sizeof( ClassName ) );

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
  memset( ArrayName, 0, sizeof( ArrayName ) );

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
    printf( "levelexport usage:\n" );
    printf( "\tlucc missingnativefields <Package Name>\n\n" );
    return ERR_BAD_ARGS;
  }

  char* PkgName = argv[0];

  // Load package
  USystem::LogLevel = LOG_CRIT;
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    GLogf( LOG_CRIT, "Failed to open package '%s'; file does not exist\n" );
    return ERR_MISSING_PKG;
  }

  // Iterate and load all classes
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
      if ( strnicmp( ClassName, "None", 4 ) == 0 )
      {
        UClass* Class = (UClass*)UObject::StaticLoadObject( Pkg, Export, UClass::StaticClass(), NULL, true );
        if ( !Class )
        {
          GLogf( LOG_CRIT, "Failed to load object '%s'\n", ClassName );
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
              printf( "//====================================================================\n" );
              printf( "// Missing native fields for class '%s'\n", Class->Name.Data() );
              printf( "//====================================================================\n" );
            }

            if ( Prop->PropertyType == PROP_Struct )
              printf( "  F%s %s", ((UStructProperty*)Prop)->Struct->Name.Data(), Prop->Name.Data() );
            else if ( Prop->PropertyType == PROP_Object )
              printf( "  %s %s", GetCppClassNameProp( Prop ), Prop->Name.Data() );
            else if ( Prop->PropertyType == PROP_Array )
              printf( "  TArray<%s>* %s", GetCppArrayType( Prop ), Prop->Name.Data() );
            else
              printf( "  %s %s", CppPropNames[Prop->PropertyType], Prop->Name.Data() );

            if ( Prop->ArrayDim > 1 )
              printf( "[%i]", Prop->ArrayDim );
            printf( ";\n" );

            NumMissing++;
          }
        }

        if ( NumMissing == 0 )
          continue;

        // Iterate again, but this time spit out LINK_NATIVE_PROPERTY stuff
        printf( "BEGIN_PROPERTY_LINK( %s, %i )\n", GetCppClassName( Class ), NumMissing );
        for ( UField* It = Class->Children; It != NULL; It = It->Next )
        {
          UProperty* Prop = SafeCast<UProperty>( It );
          if ( Prop && Prop->Offset == MAX_UINT32 && Prop->Outer == Class )
            printf( "  LINK_NATIVE_PROPERTY( %s );\n", Prop->Name.Data() );
        }
        printf( "END_PROPERTY_LINK()\n" );
      }
    }
  }

  return 0;
}

