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
 * PlayMusic.cpp - Plays music from any package
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#include "lucc.h"

int playmusic( int argc, char** argv )
{
  // Initialize engine
  GEngine = (UEngine*)UEngine::StaticClass()->CreateObject();
  if ( !GEngine->Init() )
  {
    GLogf( LOG_CRIT, "Engine init failed" );
    return false;
  }

  // Load music package
  char* PkgName = argv[0];
  UPackage* Pkg = UPackage::StaticLoadPackage( PkgName );
  if ( Pkg == NULL )
  {
    GLogf( LOG_CRIT, "Failed to open package '%s'; file does not exist\n" );
    return ERR_MISSING_PKG;
  }

  // Get music
  UMusic* Music = (UMusic*)UObject::StaticLoadObject( Pkg, Pkg->GetExport( 0 ), UMusic::StaticClass(), NULL );

  // Play music
  GEngine->Audio->PlayMusic( Music, 0, MTRAN_Instant );

  // Start ticking
  double LastTime = USystem::GetSeconds();
  double CurrentTime = 0;
  while ( 1 )
  {
    CurrentTime = USystem::GetSeconds();

    double DeltaTime = CurrentTime - LastTime;
    GEngine->Tick( DeltaTime );

    LastTime = CurrentTime;
  }
}
