#include "UTPWindZone.h"

#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Math/RotationMatrix.h"
#include "NiagaraComponent.h"
#include "NiagaraDecalRendererProperties.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "UTPWindReceiverInterface.h"

AUTPWindZone::AUTPWindZone()
{
	PrimaryActorTick.bCanEverTick = true;

	WindOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("WindOrigin"));
	SetRootComponent(WindOrigin);

	WindVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WindVolume"));
	WindVolume->SetupAttachment(WindOrigin);
	WindVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WindVolume->SetCollisionObjectType(ECC_WorldDynamic);
	WindVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	WindVolume->SetGenerateOverlapEvents(true);

#if WITH_EDITORONLY_DATA
	WindDirectionArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("WindDirectionArrow"));
	if (WindDirectionArrow)
	{
		WindDirectionArrow->SetupAttachment(WindOrigin);
		WindDirectionArrow->SetAbsolute(false, true, true);
		WindDirectionArrow->SetArrowFColor(FColor::Cyan);
		WindDirectionArrow->SetArrowSize(2.0f);
		WindDirectionArrow->SetTreatAsASprite(false);
		WindDirectionArrow->SetUseInEditorScaling(false);
		WindDirectionArrow->SetIsVisualizationComponent(true);
		WindDirectionArrow->SetHiddenInGame(true);
		WindDirectionArrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
#endif

	WindOutletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WindOutletMesh"));
	WindOutletMesh->SetupAttachment(WindOrigin);
	WindOutletMesh->SetUsingAbsoluteScale(true);
	WindOutletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WindOutletMesh->SetGenerateOverlapEvents(false);

	WindVisual = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WindVisual"));
	WindVisual->SetupAttachment(WindVolume);
	WindVisual->SetAutoActivate(true);
	WindVisual->SetCastShadow(false);
	// The wind direction and its guide rotate with the zone.
	WindVisual->SetUsingAbsoluteRotation(false);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BoundaryCylinderSystem(
		TEXT("/Game/_Art/NiagaraExamples/FX_Misc/NS_Boundary_Cylinder.NS_Boundary_Cylinder"));
	if (BoundaryCylinderSystem.Succeeded())
	{
		WindVisual->SetAsset(BoundaryCylinderSystem.Object);
	}
}

void AUTPWindZone::BeginPlay()
{
	Super::BeginPlay();

	CreateDirectionalWindVisualSystem();
	WindVolume->OnComponentBeginOverlap.AddDynamic(this, &AUTPWindZone::OnWindVolumeBeginOverlap);
	WindVolume->OnComponentEndOverlap.AddDynamic(this, &AUTPWindZone::OnWindVolumeEndOverlap);
	RefreshWindVolume();
	RefreshWindVisual();
	SetWindVisualEnabled(bEnabled);
}

void AUTPWindZone::CreateDirectionalWindVisualSystem()
{
	if (!WindVisual || DirectionalWindVisualSystem)
	{
		return;
	}

	UNiagaraSystem* SourceSystem = WindVisual->GetAsset();
	if (!SourceSystem)
	{
		return;
	}

	DirectionalWindVisualSystem = DuplicateObject<UNiagaraSystem>(SourceSystem, this);
	for (FNiagaraEmitterHandle& EmitterHandle : DirectionalWindVisualSystem->GetEmitterHandles())
	{
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			continue;
		}

		for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
		{
			if (Renderer && Renderer->IsA<UNiagaraDecalRendererProperties>())
			{
				Renderer->SetIsEnabled(false);
			}
		}
	}

	WindVisual->SetAsset(DirectionalWindVisualSystem);
	WindVisual->ReinitializeSystem();
}

void AUTPWindZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshWindVolume();
	RefreshWindVisual();
#if WITH_EDITORONLY_DATA
	RefreshWindDirectionArrow();
#endif
}

void AUTPWindZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bEnabled)
	{
		const TArray<TWeakObjectPtr<AActor>> ReceiversToRemove = ActiveReceivers.Array();
		for (const TWeakObjectPtr<AActor>& Receiver : ReceiversToRemove)
		{
			RemoveWindFromActor(Receiver.Get());
		}

		ActiveReceivers.Empty();
		return;
	}

	if (!bRefreshOverlapsWhileEnabled)
	{
		return;
	}

	// This also handles a zone being enabled while a receiver is already inside it.
	TArray<AActor*> OverlappingActors;
	WindVolume->GetOverlappingActors(OverlappingActors);
	for (AActor* OtherActor : OverlappingActors)
	{
		ApplyWindToActor(OtherActor);
	}
}

void AUTPWindZone::SetWindEnabled(bool bInEnabled)
{
	if (bEnabled == bInEnabled)
	{
		return;
	}

	bEnabled = bInEnabled;
	SetWindVisualEnabled(bEnabled);
	if (!bEnabled)
	{
		const TArray<TWeakObjectPtr<AActor>> ReceiversToRemove = ActiveReceivers.Array();
		for (const TWeakObjectPtr<AActor>& Receiver : ReceiversToRemove)
		{
			RemoveWindFromActor(Receiver.Get());
		}

		ActiveReceivers.Empty();
	}
}

float AUTPWindZone::GetWindVolumeHalfLength() const
{
	if (!WindVolume || WindDirection.IsNearlyZero())
	{
		return 0.0f;
	}

	const FVector WorldDirection = GetActorTransform().TransformVectorNoScale(WindDirection).GetSafeNormal();
	const FVector Direction = WindVolume->GetComponentTransform().InverseTransformVectorNoScale(WorldDirection);

	// Intersect the center line with the box faces. Projecting the corners onto
	// the direction would overestimate the reach for a diagonal wind corridor.
	const FVector BoxExtent = WindVolume->GetScaledBoxExtent().GetAbs();
	float HalfLength = TNumericLimits<float>::Max();
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const float AxisDirection = FMath::Abs(Direction[Axis]);
		if (AxisDirection > KINDA_SMALL_NUMBER)
		{
			HalfLength = FMath::Min(HalfLength, static_cast<float>(BoxExtent[Axis] / AxisDirection));
		}
	}
	return HalfLength;
}

void AUTPWindZone::RefreshWindVolume()
{
	if (!WindVolume)
	{
		return;
	}

	// Placed actors can retain the old serialized parent even after the native
	// hierarchy changes. Migrate before moving the box, preserving the outlet's
	// authored world transform instead of carrying it along with the volume.
	if (WindOrigin && WindOutletMesh && WindOutletMesh->GetAttachParent() != WindOrigin)
	{
		const FTransform OutletWorldTransform = WindOutletMesh->GetComponentTransform();
		WindOutletMesh->SetUsingAbsoluteScale(true);
		WindOutletMesh->AttachToComponent(WindOrigin, FAttachmentTransformRules::KeepWorldTransform);
		WindOutletMesh->SetWorldTransform(OutletWorldTransform);
	}

	// The box remains centered on its own component, while the actor pivot is
	// its inlet. Increasing the extent moves only the center and downstream end.
	const FVector WorldDirection = GetActorTransform().TransformVectorNoScale(WindDirection).GetSafeNormal();
	WindVolume->SetWorldLocation(GetActorLocation() + WorldDirection * GetWindVolumeHalfLength());
}

#if WITH_EDITORONLY_DATA
void AUTPWindZone::RefreshWindDirectionArrow()
{
	if (!WindDirectionArrow || !WindVolume)
	{
		return;
	}

	const FVector Direction = WindDirection.GetSafeNormal();
	const float HalfLength = GetWindVolumeHalfLength();
	WindGuideLength = HalfLength * 2.0f;
	if (WindGuideLength <= KINDA_SMALL_NUMBER)
	{
		WindDirectionArrow->SetVisibility(false);
		return;
	}

	const FVector WorldDirection = GetActorTransform().TransformVectorNoScale(Direction);
	WindDirectionArrow->SetWorldLocation(GetActorLocation());
	// Absolute rotation avoids mirrored/nonuniform parent scales skewing the
	// guide. OnConstruction updates it whenever the zone is edited or rotated.
	WindDirectionArrow->SetUsingAbsoluteRotation(true);
	WindDirectionArrow->SetWorldRotation(WorldDirection.Rotation());
	WindDirectionArrow->SetWorldScale3D(FVector::OneVector);
	// UArrowComponent multiplies ArrowLength by ArrowSize when rendering.
	const float ArrowSize = FMath::Clamp(WindGuideLength / 100.0f, 0.01f, 2.0f);
	WindDirectionArrow->SetArrowSize(ArrowSize);
	WindDirectionArrow->SetArrowLength(WindGuideLength / ArrowSize);
	WindDirectionArrow->SetVisibility(WindGuideLength > KINDA_SMALL_NUMBER);
}
#endif

void AUTPWindZone::RefreshWindVisual()
{
	// Reapply for existing Blueprint instances that saved the old inherited-scale
	// setting. Keep the mesh's own scale while resizing the wind volume.
	if (WindOutletMesh)
	{
		WindOutletMesh->SetUsingAbsoluteScale(true);
	}

	if (!WindVisual || !WindVolume)
	{
		return;
	}

	const FVector BoxExtent = WindVolume->GetUnscaledBoxExtent();
	const FVector LocalWindDirectionRaw = WindDirection.GetSafeNormal();
	const FVector LocalWindDirection = LocalWindDirectionRaw.IsNearlyZero()
		? FVector::UpVector
		: LocalWindDirectionRaw.GetSafeNormal();

	// The Niagara system's cylinder points along its local Z axis.  Size it to
	// cover the collision volume and orient it along the gameplay wind vector.
	const float HalfLength = FMath::Abs(LocalWindDirection.X) * BoxExtent.X
		+ FMath::Abs(LocalWindDirection.Y) * BoxExtent.Y
		+ FMath::Abs(LocalWindDirection.Z) * BoxExtent.Z;
	float Radius = 0.0f;
	for (int32 XSign = -1; XSign <= 1; XSign += 2)
	{
		for (int32 YSign = -1; YSign <= 1; YSign += 2)
		{
			for (int32 ZSign = -1; ZSign <= 1; ZSign += 2)
			{
				const FVector Corner(
					XSign * BoxExtent.X,
					YSign * BoxExtent.Y,
					ZSign * BoxExtent.Z);
				const FVector RadialOffset = Corner
					- FVector::DotProduct(Corner, LocalWindDirection) * LocalWindDirection;
				Radius = FMath::Max(Radius, RadialOffset.Size());
			}
		}
	}

	WindVisual->SetRelativeLocation(WindVisualOffset);
	WindVisual->SetUsingAbsoluteRotation(false);
	WindVisual->SetRelativeRotation(FRotationMatrix::MakeFromZ(LocalWindDirection).Rotator());
	WindVisual->SetVariableFloat(TEXT("User.Radius"), Radius * WindVisualRadiusScale);
	WindVisual->SetVariableFloat(TEXT("User.Height"), HalfLength * 2.0f * WindVisualHeightScale);
	WindVisual->SetVariableLinearColor(TEXT("User.Color"), WindVisualColor);
	SetWindVisualEnabled(bEnabled);
}

void AUTPWindZone::SetWindVisualEnabled(bool bInEnabled)
{
	if (!WindVisual)
	{
		return;
	}

	const bool bShouldShowVisual = bInEnabled && bShowWindVisual;
	WindVisual->SetVisibility(bShouldShowVisual, true);
	if (bShouldShowVisual)
	{
		WindVisual->Activate(true);
	}
	else
	{
		WindVisual->DeactivateImmediate();
	}
}

void AUTPWindZone::SetToggleableEnabled_Implementation(bool bInEnabled)
{
	SetWindEnabled(bInEnabled);
}

bool AUTPWindZone::IsToggleableEnabled_Implementation() const
{
	return IsWindEnabled();
}

void AUTPWindZone::OnWindVolumeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ApplyWindToActor(OtherActor);
}

void AUTPWindZone::OnWindVolumeEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	RemoveWindFromActor(OtherActor);
}

void AUTPWindZone::ApplyWindToActor(AActor* OtherActor)
{
	if (!bEnabled || !IsValid(OtherActor) ||
		!OtherActor->GetClass()->ImplementsInterface(UTPWindReceiverInterface::StaticClass()))
	{
		return;
	}

	const FVector Direction = GetActorTransform().TransformVectorNoScale(WindDirection).GetSafeNormal();
	ITPWindReceiverInterface::Execute_EnterWindZone(
		OtherActor,
		this,
		Direction,
		WindSpeed,
		LiftStrength);
	ActiveReceivers.Add(OtherActor);
}

void AUTPWindZone::RemoveWindFromActor(AActor* OtherActor)
{
	if (!IsValid(OtherActor) ||
		!OtherActor->GetClass()->ImplementsInterface(UTPWindReceiverInterface::StaticClass()))
	{
		return;
	}

	ITPWindReceiverInterface::Execute_ExitWindZone(OtherActor, this);
	ActiveReceivers.Remove(OtherActor);
}
