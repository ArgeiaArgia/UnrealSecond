#include "UTPWindZone.h"

#include "Components/BoxComponent.h"
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

	WindVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WindVolume"));
	SetRootComponent(WindVolume);
	WindVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WindVolume->SetCollisionObjectType(ECC_WorldDynamic);
	WindVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	WindVolume->SetGenerateOverlapEvents(true);

	WindOutletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WindOutletMesh"));
	WindOutletMesh->SetupAttachment(WindVolume);
	WindOutletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WindOutletMesh->SetGenerateOverlapEvents(false);

	WindVisual = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WindVisual"));
	WindVisual->SetupAttachment(WindVolume);
	WindVisual->SetAutoActivate(true);
	WindVisual->SetCastShadow(false);
	// WindDirection is a world-space gameplay vector, so the visual must not
	// inherit a parent transform that could make it appear to track the camera.
	WindVisual->SetUsingAbsoluteRotation(true);

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
	RefreshWindVisual();
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

void AUTPWindZone::RefreshWindVisual()
{
	if (!WindVisual || !WindVolume)
	{
		return;
	}

	const FVector BoxExtent = WindVolume->GetUnscaledBoxExtent();
	const FVector LocalWindDirectionRaw = GetActorTransform()
		.InverseTransformVectorNoScale(WindDirection.GetSafeNormal());
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
	WindVisual->SetWorldRotation(FRotationMatrix::MakeFromZ(WindDirection.GetSafeNormal()).Rotator());
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

	const FVector Direction = WindDirection.GetSafeNormal();
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
