// BanditBaller

#include "GSMeshAttachment.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "ReferenceSkeleton.h"

FTransform GSMeshAttachments::GetRefPoseComponentTransform(const USkeletalMesh* Mesh, FName Bone)
{
	if (!Mesh)
	{
		return FTransform::Identity;
	}
	const FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();
	int32 BoneIndex = RefSkeleton.FindBoneIndex(Bone);
	if (BoneIndex == INDEX_NONE)
	{
		return FTransform::Identity;
	}
	const TArray<FTransform>& RefPose = RefSkeleton.GetRefBonePose();
	FTransform Result = RefPose[BoneIndex];
	for (int32 Parent = RefSkeleton.GetParentIndex(BoneIndex); Parent != INDEX_NONE; Parent = RefSkeleton.GetParentIndex(Parent))
	{
		Result = Result * RefPose[Parent];
	}
	return Result;
}

void GSMeshAttachments::Apply(AActor* Owner, USkeletalMeshComponent* Target, const TArray<FGSMeshAttachment>& Attachments,
	TArray<TObjectPtr<UStaticMeshComponent>>& InOutComponents)
{
	for (UStaticMeshComponent* Old : InOutComponents)
	{
		if (IsValid(Old))
		{
			Old->DestroyComponent();
		}
	}
	InOutComponents.Reset();
	if (!Owner || !Target)
	{
		return;
	}

	const USkeletalMesh* SkeletalMesh = Target->GetSkeletalMeshAsset();
	for (const FGSMeshAttachment& Attachment : Attachments)
	{
		if (!Attachment.Mesh || Attachment.Bone.IsNone() || !Target->DoesSocketExist(Attachment.Bone))
		{
			continue;
		}

		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		Comp->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Comp->SetStaticMesh(Attachment.Mesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetCanEverAffectNavigation(false);
		for (int32 Slot = 0; Slot < Attachment.Materials.Num(); ++Slot)
		{
			if (Attachment.Materials[Slot])
			{
				Comp->SetMaterial(Slot, Attachment.Materials[Slot]);
			}
		}
		Comp->SetupAttachment(Target, Attachment.Bone);

		FTransform Relative = Attachment.Offset;
		if (Attachment.bModeledInMeshSpace)
		{
			// The prop was modelled in the skeletal mesh's space with its origin on the joint: place it there in the
			// reference pose, then express that relative to the bone so it moves rigidly with it.
			const FTransform BoneCS = GetRefPoseComponentTransform(SkeletalMesh, Attachment.Bone);
			FTransform PropCS = Attachment.Offset;
			PropCS.AddToTranslation(BoneCS.GetLocation());
			Relative = PropCS.GetRelativeTransform(BoneCS);
		}
		Comp->SetRelativeTransform(Relative);
		Comp->RegisterComponent();
		Owner->AddInstanceComponent(Comp);
		InOutComponents.Add(Comp);
	}
}
