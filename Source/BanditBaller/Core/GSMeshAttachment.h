// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GSMeshAttachment.generated.h"

class AActor;
class UMaterialInterface;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A cosmetic static mesh worn on a character's skeletal mesh (a ski mask, a loot bag, a cap). Placeholder art from
 * Blender (Prompt 56); no collision, no gameplay.
 *
 * bModeledInMeshSpace (the default) is for props modelled in Blender around the low-poly body: the prop keeps the
 * skeletal mesh's own axes and sits on the bone's joint, as it was modelled in the reference pose, then moves rigidly
 * with that bone. Its origin is the bone's joint. Otherwise Offset is relative to the bone (or socket) itself.
 */
USTRUCT(BlueprintType)
struct BANDITBALLER_API FGSMeshAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachment")
	TObjectPtr<UStaticMesh> Mesh;

	/** Bone (or socket) of the skeletal mesh it follows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachment")
	FName Bone;

	/** Modelled around the body in the reference pose, origin on the bone's joint (see the struct comment). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachment")
	bool bModeledInMeshSpace = true;

	/** Extra transform: in mesh space from the joint when bModeledInMeshSpace, else relative to the bone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachment")
	FTransform Offset;

	/** Material per slot (empty entries keep the mesh's own). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachment")
	TArray<TObjectPtr<UMaterialInterface>> Materials;
};

namespace GSMeshAttachments
{
	/** The bone's reference-pose transform in the skeletal mesh's component space (identity if missing). */
	BANDITBALLER_API FTransform GetRefPoseComponentTransform(const USkeletalMesh* Mesh, FName Bone);

	/**
	 * Destroys the components in InOutComponents, then creates one static mesh component per attachment on Owner,
	 * attached to Target (no collision, no navigation effect). Skips entries without a mesh or with an unknown bone.
	 */
	BANDITBALLER_API void Apply(AActor* Owner, USkeletalMeshComponent* Target, const TArray<FGSMeshAttachment>& Attachments,
		TArray<TObjectPtr<UStaticMeshComponent>>& InOutComponents);
}
