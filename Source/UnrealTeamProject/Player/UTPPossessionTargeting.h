#pragma once

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

namespace UTPPossessionTargeting
{
	inline FBox GetBodyBounds(const AActor& Actor)
	{
		if (const ACharacter* Character = Cast<ACharacter>(&Actor))
		{
			const USkeletalMeshComponent* Mesh = Character->GetMesh();
			if (Mesh && Mesh->IsRegistered() && Mesh->GetSkeletalMeshAsset())
			{
				// Only the visible body defines the focus point. Trigger volumes,
				// effects and other auxiliary components must not move it.
				return Mesh->Bounds.GetBox();
			}
		}

		if (const UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Actor.GetRootComponent()))
		{
			if (Root->IsRegistered())
			{
				return Root->Bounds.GetBox();
			}
		}

		return FBox(Actor.GetActorLocation(), Actor.GetActorLocation());
	}

	inline FVector GetFocusLocation(const AActor& Actor)
	{
		return GetBodyBounds(Actor).GetCenter();
	}
}
