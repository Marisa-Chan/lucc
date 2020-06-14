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
 * LevelViewer.cpp - Opens a viewport to explore a level
 *
 * written by Adam 'Xaleros' Smith
 *========================================================================
*/

#include "lucc.h"

bool bLeftMouseHeld;
bool bRightMouseHeld;

int MouseSpeed = 100;

void CameraMove( EInputKey Key, float DeltaTime, bool bKeyDown )
{
  switch ( Key )
  {
  case IK_LeftMouse:
    bLeftMouseHeld = bKeyDown;
    return;
  case IK_RightMouse:
    bRightMouseHeld = bKeyDown;
    return;
  }
}

void CameraMouseMove( float DeltaTime, int DeltaX, int DeltaY )
{
  // Convert rotation axes
  APlayerPawn* Player = GEngine->Client->CurrentViewport->Actor;
  FVector& ViewLoc = Player->Location;
  FRotator& ViewRot = Player->Rotation;
  FVector X, Y, Z;

  ViewRot.GetAxes( X, Y, Z );
  X.Z = 0;
  Y.Z = 0;

  if ( bLeftMouseHeld )
  {
    if ( bRightMouseHeld )
    {
      // Both buttons held: DeltaX = Camera relative Y Axis movement, DeltaY = Absolute Z Axis movement
      Y *= (float)DeltaX;
      Y *= (float)DeltaTime;
      Y *= (float)MouseSpeed;
      ViewLoc += Y;

      ViewLoc.Z += (DeltaY * DeltaTime * MouseSpeed);
    }
    else
    {
      // Only left mouse held: DeltaX = Yaw camera rotation, DeltaY = X Axis movement
      ViewRot.Yaw -= DeltaX * 10;
      X *= (float)DeltaY;
      X *= (float)DeltaTime;
      X *= (float)MouseSpeed;
      ViewLoc += X;
    }
  }
  else if ( bRightMouseHeld )
  {
    // Only right mouse held: DeltaX = Yaw camera rotation, DeltaY = Pitch camera rotation
    ViewRot.Yaw -= DeltaX * 10;
    ViewRot.Pitch += DeltaY * 10;
  }
}

int levelviewer( int argc, char** argv )
{
  // Argument parsing
  int i = 0;
  while ( 1 )
  {
    if ( argc == 0 || i > argc )
    {
    BadOpt:
      printf( "level usage:\n" );
      printf( "\tlucc [gopts] levelviewer <Package Name>\n\n" );
      return ERR_BAD_ARGS;
    }

    if ( argv[i][0] == '-' )
    {
      GLogf( LOG_WARN, "Bad option '%s'", argv[i] );
      goto BadOpt;
    }
    else
    {
      PkgName = argv[i];
      break;
    }

    i++;
  }

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

  // Create a camera
  ACamera* Camera = (ACamera*)ACamera::StaticClass()->CreateObject();
  Camera->Acceleration = FVector( 100, 100, 0 );
  Viewport->Possess( Camera );

  // Bind inputs to CameraMove
  GEngine->Client->BindKeyInput( IK_LeftMouse, CameraMove );
  GEngine->Client->BindKeyInput( IK_RightMouse, CameraMove );
  GEngine->Client->BindMouseInput( CameraMouseMove );

  // Load packages
  UPackage* Engine = UPackage::StaticLoadPackage( "Engine" );

  // Load medium font
  UFont* MedFont = (UFont*)UObject::StaticLoadObject( Engine, "MedFont", UFont::StaticClass(), NULL );

  // Load level
  UPackage* Entry = UPackage::StaticLoadPackage( PkgName );
  ULevel* MyLevel = (ULevel*)UObject::StaticLoadObject( Entry, "MyLevel", ULevel::StaticClass(), NULL );
  GEngine->Level = MyLevel;

  FBoxInt2D TextPosX( 8, 8, 256, 256 );
  FBoxInt2D TextPosY( 8, 20, 256, 256 );
  FBoxInt2D TextPosZ( 8, 32, 256, 256 );
  FBoxInt2D TextPosPitch( 8, 44, 256, 256 );
  FBoxInt2D TextPosYaw( 8, 56, 256, 256 );
  FBoxInt2D TextPosRoll( 8, 68, 256, 256 );

  // Cube properties
  FVector Loc( 0, 0, 0 );
  FRotator Rot( 0, 0, 0 );
  FVector Scale( 1.0f, 1.0f, 1.0f );

  GEngine->Client->CurrentViewport->Actor->Location = FVector( 0, 0, 0 );
  GEngine->Client->CurrentViewport->Actor->Rotation = FRotator( 0, 0, 0 );
  FVector& CameraLoc = GEngine->Client->CurrentViewport->Actor->Location;
  FRotator& CameraRot = GEngine->Client->CurrentViewport->Actor->Rotation;

  // Start ticking
  double LastTime = USystem::GetSeconds();
  double CurrentTime = 0;
  double FrameNum = 0.0;
  while ( 1 )
  {
    CurrentTime = USystem::GetSeconds();
    double DeltaTime = CurrentTime - LastTime;

    if ( DeltaTime <= FLT_MIN )
      DeltaTime = FLT_MIN;

    FrameNum += ((1.0/30.0) * DeltaTime);
    if ( FrameNum > 1.0 )
      FrameNum = 0.0;

    // Camera debug
    FString CameraX = "Camera.X: " + FString( CameraLoc.X );
    FString CameraY = "Camera.Y: " + FString( CameraLoc.Y );
    FString CameraZ = "Camera.Z: " + FString( CameraLoc.Z );
    FString CameraPitch = "Camera.Pitch: " + FString( CameraRot.Pitch );
    FString CameraYaw   = "Camera.Yaw:   " + FString( CameraRot.Yaw );
    FString CameraRoll  = "Camera.Roll:  " + FString( CameraRot.Roll );
    GEngine->Render->DrawText( MedFont, TextPosX, CameraX );
    GEngine->Render->DrawText( MedFont, TextPosY, CameraY );
    GEngine->Render->DrawText( MedFont, TextPosZ, CameraZ );
    GEngine->Render->DrawText( MedFont, TextPosPitch, CameraPitch );
    GEngine->Render->DrawText( MedFont, TextPosYaw, CameraYaw );
    GEngine->Render->DrawText( MedFont, TextPosRoll, CameraRoll );

    // Tick tock
    GEngine->Tick( (float)DeltaTime );

    LastTime = CurrentTime;
  }

  return 0;
}
