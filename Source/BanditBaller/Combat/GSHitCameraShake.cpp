// BanditBaller

#include "GSHitCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UGSHitCameraShake::UGSHitCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;

	if (UPerlinNoiseCameraShakePattern* Pattern = Cast<UPerlinNoiseCameraShakePattern>(GetRootShakePattern()))
	{
		Pattern->Duration = 0.25f;
		Pattern->BlendInTime = 0.02f;
		Pattern->BlendOutTime = 0.15f;

		Pattern->LocationAmplitudeMultiplier = 0.f;

		Pattern->RotationAmplitudeMultiplier = 1.f;
		Pattern->Pitch.Amplitude = 1.5f;
		Pattern->Pitch.Frequency = 20.f;
		Pattern->Yaw.Amplitude = 1.f;
		Pattern->Yaw.Frequency = 20.f;
		Pattern->Roll.Amplitude = 0.6f;
		Pattern->Roll.Frequency = 15.f;
		Pattern->FOV.Amplitude = 0.f;
	}
}
