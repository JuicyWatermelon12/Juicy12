// BanditBaller

#pragma once

#include "Engine/EngineTypes.h"

/** Project collision channels (defined in Config/DefaultEngine.ini, [/Script/Engine.CollisionProfile]). */
namespace GSCollision
{
	/**
	 * Object type of AGSMapBoundary. Every profile blocks it by default, but the boundary only blocks the Pawn
	 * channel, so it stops pawns and nothing else. Weapon traces (Visibility), the camera (Camera), enemy shots
	 * (WorldStatic / WorldDynamic / Pawn object query) and pickup traces (WorldStatic) never see it.
	 */
	constexpr ECollisionChannel MapBoundary = ECC_GameTraceChannel1;
}
