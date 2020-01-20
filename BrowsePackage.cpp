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
 * BrowsePackage.cpp - Opens a viewport to browse and preview objects
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#include <libunr.h>

int browsepackage( int argc, char** argv )
{
  // Initialize engine
  GEngine = (UEngine*)UEngine::StaticClass()->CreateObject();
  if ( !GEngine->Init() )
  {
    GLogf( LOG_CRIT, "Engine init failed" );
    return false;
  }

  // Create a viewport
  UViewport* Viewport = GEngine->Client->OpenViewport();
  Viewport->Show();

  // Start ticking
  double LastTime = USystem::GetSeconds();
  double CurrentTime = 0;
  while ( 1 )
  {
    CurrentTime = USystem::GetSeconds();
    double DeltaTime = CurrentTime - LastTime;

    // Load the default texture
    UPackage* Engine = UPackage::StaticLoadPackage( "Engine" );
    UTexture* Default = (UTexture*)UObject::StaticLoadObject( Engine, "DefaultTexture", UTexture::StaticClass(), NULL );
    UTexture* Trigger = (UTexture*)UObject::StaticLoadObject( Engine, "S_Trigger", UTexture::StaticClass(), NULL );

    // Test render
    int StartX = (GEngine->Client->CurrentViewport->Width / 2) - (Default->USize / 2);
    int StartY = (GEngine->Client->CurrentViewport->Height / 2) - (Default->VSize / 2);
    FBoxInt2D Dim( StartX, StartY, StartX + Default->USize, StartY + Default->VSize );
    FRotator Rot;
    GEngine->Render->DrawTile( Default, Dim, Rot, 0.0f, 0.0f, 1.0f, 1.0f );

    StartX = (GEngine->Client->CurrentViewport->Width / 2) - (Trigger->USize / 2);
    StartY = (GEngine->Client->CurrentViewport->Height / 2) - (Trigger->VSize / 2);
    Dim = FBoxInt2D( StartX, StartY, StartX + Trigger->USize, StartY + Trigger->VSize );
    GEngine->Render->DrawTile( Trigger, Dim, Rot, 0.0f, 0.0f, 1.0f, 1.0f );

    GEngine->Tick( DeltaTime );

    LastTime = CurrentTime;
  }

  return 0;
}
