#include "UTPPressurePlate.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "../Animals/Turtle/UTPTurtleCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "../../TPToggleableInterface.h"
#include "UTPWeightProviderInterface.h"

namespace
{
bool IsInsidePlateBounds(const UBoxComponent* TriggerVolume, const AActor* Actor)
{
	if (!TriggerVolume || !IsValid(Actor))
	{
		return false;
	}

	const FVector LocalActorLocation = TriggerVolume->GetComponentTransform()
		.InverseTransformPosition(Actor->GetActorLocation());
	// InverseTransformPosition 결과는 컴포넌트의 로컬(비율 적용 전) 좌표이므로
	// 스케일이 적용되지 않은 Box extent와 비교해야 합니다.
	FVector AcceptedExtent = TriggerVolume->GetUnscaledBoxExtent();
	// 거북이의 Actor 원점은 캡슐 중심에 있고, 현재 맵의 플레이트는 Actor
	// 스케일 0.5를 사용합니다. 판 위의 실제 중심 높이(로컬 약 374uu)까지
	// 포함하도록 충분한 상단 여유를 둡니다.
	AcceptedExtent.Z += 260.0f;

	// Capsule 가장자리만 플레이트에 닿는 경우는 눌림으로 인정하지 않습니다.
	// 현재 맵의 0.5 스케일에서는 120uu가 월드 기준 60uu(거북이 Capsule 반지름)입니다.
	constexpr float FootprintInset = 120.0f;
	AcceptedExtent.X = FMath::Max(0.0f, AcceptedExtent.X - FootprintInset);
	AcceptedExtent.Y = FMath::Max(0.0f, AcceptedExtent.Y - FootprintInset);

	// Trigger의 상단 여유를 제외한 윗부분만 '판 위'로 판정합니다.
	const float MinimumStandingHeight = TriggerVolume->GetUnscaledBoxExtent().Z - 80.0f;

	return FMath::Abs(LocalActorLocation.X) <= AcceptedExtent.X
		&& FMath::Abs(LocalActorLocation.Y) <= AcceptedExtent.Y
		&& LocalActorLocation.Z >= MinimumStandingHeight
		&& LocalActorLocation.Z <= AcceptedExtent.Z;
}
}

AUTPPressurePlate::AUTPPressurePlate()
{
	PrimaryActorTick.bCanEverTick = true;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(100.0f, 100.0f, 25.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionObjectType(ECC_WorldStatic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);

	BlockingVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingVolume"));
	BlockingVolume->SetupAttachment(TriggerVolume);
	BlockingVolume->SetBoxExtent(FVector(100.0f, 100.0f, 25.0f));
	BlockingVolume->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingVolume->SetCollisionObjectType(ECC_WorldStatic);
	BlockingVolume->SetCollisionResponseToAllChannels(ECR_Block);
	BlockingVolume->SetGenerateOverlapEvents(false);

	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(TriggerVolume);
	PlateMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PlateMesh->SetGenerateOverlapEvents(false);
}

void AUTPPressurePlate::BeginPlay()
{
	Super::BeginPlay();
	PlateMeshRestingLocation = PlateMesh ? PlateMesh->GetRelativeLocation() : FVector::ZeroVector;
	BlockingVolumeRestingLocation = BlockingVolume ? BlockingVolume->GetRelativeLocation() : FVector::ZeroVector;

	// 추가 대상이 지정된 발판은 단일 대상 자동 검색을 하지 않습니다.
	if (!TargetDoor && TargetWindZones.IsEmpty())
	{
		TArray<AActor*> ToggleableActors;
		UGameplayStatics::GetAllActorsWithInterface(this, UTPToggleableInterface::StaticClass(), ToggleableActors);
		if (ToggleableActors.Num() == 1)
		{
			TargetDoor = ToggleableActors[0];
		}

		UE_LOG(LogTemp, Warning, TEXT("[PressurePlate] %s: Target=%s, ToggleableCount=%d"),
			*GetName(), *GetNameSafe(TargetDoor), ToggleableActors.Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PressurePlate] %s: assigned TargetDoor=%s"), *GetName(), *GetNameSafe(TargetDoor));
	}

	TArray<AActor*> TurtleActors;
	UGameplayStatics::GetAllActorsOfClass(this, AUTPTurtleCharacter::StaticClass(), TurtleActors);
	UE_LOG(LogTemp, Warning, TEXT("[PressurePlate] %s: detected TurtleActors=%d at BeginPlay"), *GetName(), TurtleActors.Num());

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AUTPPressurePlate::OnTriggerBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AUTPPressurePlate::OnTriggerEndOverlap);
	// Blueprint 자식의 Tick 설정과 관계없이 게임 중 위치 변화가 반드시 반영되도록 합니다.
	GetWorldTimerManager().SetTimer(RefreshTimerHandle, this, &AUTPPressurePlate::RefreshPressedState,
		0.1f, true);
	RefreshPressedState();
	ApplyTargetState(bIsPressed);
}

void AUTPPressurePlate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateTriggerVolumeFromPlateMesh();
}

void AUTPPressurePlate::Tick(float DeltaSeconds)
{
	RefreshPressedState();
	UpdatePlateDepression(DeltaSeconds);
}

void AUTPPressurePlate::UpdateTriggerVolumeFromPlateMesh()
{
	if (!TriggerVolume || !PlateMesh)
	{
		return;
	}

	const UStaticMesh* StaticMesh = PlateMesh->GetStaticMesh();
	if (!StaticMesh)
	{
		return;
	}

	const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
	const FVector MeshScale = PlateMesh->GetRelativeScale3D().GetAbs();
	const FVector MeshExtent = MeshBounds.BoxExtent * MeshScale;
	if (MeshExtent.IsNearlyZero())
	{
		return;
	}

	// 캐릭터의 Actor 원점은 Capsule 중심에 있으므로, 판의 윗면보다 충분히 높은
	// 위치까지 감지 범위를 확장합니다. 현재 맵에서 거북이 중심은 플레이트보다
	// 로컬 374uu 높이에 있으므로, 그 높이를 포함하도록 확장합니다.
	FVector TriggerExtent = MeshExtent;
	TriggerExtent.Z += 260.0f;
	TriggerVolume->SetBoxExtent(TriggerExtent);

	if (BlockingVolume)
	{
		BlockingVolume->SetBoxExtent(MeshExtent);
	}
}

void AUTPPressurePlate::UpdatePlateDepression(float DeltaSeconds)
{
	if (!PlateMesh || !BlockingVolume)
	{
		return;
	}

	const float TargetOffsetZ = bIsPressed ? -PressDepth : 0.0f;
	const float InterpSpeed = bIsPressed ? PressSpeed : ReleaseSpeed;
	const FVector TargetPlateLocation = PlateMeshRestingLocation + FVector(0.0f, 0.0f, TargetOffsetZ);
	const FVector TargetBlockingLocation = BlockingVolumeRestingLocation + FVector(0.0f, 0.0f, TargetOffsetZ);

	PlateMesh->SetRelativeLocation(FMath::VInterpTo(
		PlateMesh->GetRelativeLocation(), TargetPlateLocation, DeltaSeconds, InterpSpeed));
	BlockingVolume->SetRelativeLocation(FMath::VInterpTo(
		BlockingVolume->GetRelativeLocation(), TargetBlockingLocation, DeltaSeconds, InterpSpeed));
}

void AUTPPressurePlate::ApplyTargetState(bool bPressed)
{
	if (IsValid(TargetDoor) && TargetDoor->GetClass()->ImplementsInterface(UTPToggleableInterface::StaticClass()))
	{
		ITPToggleableInterface::Execute_SetToggleableEnabled(TargetDoor, bPressed);
	}

	for (AActor* ToggleableActor : TargetWindZones)
	{
		if (IsValid(ToggleableActor) && ToggleableActor->GetClass()->ImplementsInterface(UTPToggleableInterface::StaticClass()))
		{
			ITPToggleableInterface::Execute_SetToggleableEnabled(ToggleableActor, bPressed);
		}
	}
}

void AUTPPressurePlate::RefreshPressedState()
{
	if (!TriggerVolume)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	TriggerVolume->GetOverlappingActors(OverlappingActors);

	TArray<AActor*> WeightProviders;
	UGameplayStatics::GetAllActorsWithInterface(this, UTPWeightProviderInterface::StaticClass(), WeightProviders);

	// BP_SnappingTurtle은 네이티브 거북이에서 상속되지만, Hot Reload 직후에는
	// BlueprintGeneratedClass가 인터페이스 목록을 갱신하지 못하는 경우가 있습니다.
	// 이때도 실제 거북이는 반드시 압력판의 무게 제공자로 포함시킵니다.
	TArray<AActor*> TurtleActors;
	UGameplayStatics::GetAllActorsOfClass(this, AUTPTurtleCharacter::StaticClass(), TurtleActors);
	for (AActor* TurtleActor : TurtleActors)
	{
		WeightProviders.AddUnique(TurtleActor);
	}

	if (UWorld* World = GetWorld(); World && World->GetTimeSeconds() >= NextDebugLogTime)
	{
		NextDebugLogTime = World->GetTimeSeconds() + 1.0f;
		for (AActor* TurtleActor : TurtleActors)
		{
			if (!IsValid(TurtleActor))
			{
				continue;
			}

			const FVector LocalLocation = TriggerVolume->GetComponentTransform()
				.InverseTransformPosition(TurtleActor->GetActorLocation());
			const FVector AcceptedExtent = TriggerVolume->GetUnscaledBoxExtent();
			UE_LOG(LogTemp, Warning, TEXT("[PressurePlate] %s: Turtle=%s World=%s Local=%s Extent=%s NativeOverlap=%s InBounds=%s"),
				*GetName(), *GetNameSafe(TurtleActor), *TurtleActor->GetActorLocation().ToCompactString(),
				*LocalLocation.ToCompactString(), *AcceptedExtent.ToCompactString(),
				TriggerVolume->IsOverlappingActor(TurtleActor) ? TEXT("true") : TEXT("false"),
				IsInsidePlateBounds(TriggerVolume, TurtleActor) ? TEXT("true") : TEXT("false"));
		}
	}

	float TotalWeight = 0.0f;
	int32 GeometryOnlyProviderCount = 0;
	for (AActor* Actor : WeightProviders)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		const bool bNativeOverlap = TriggerVolume->IsOverlappingActor(Actor);
		const bool bInsideGeometryBounds = IsInsidePlateBounds(TriggerVolume, Actor);
		// Overlap만으로는 Capsule이 옆면에 살짝 닿아도 true가 됩니다.
		// 실제 눌림은 거북이 중심이 판의 상단 풋프린트 안에 있을 때만 허용합니다.
		if (!bInsideGeometryBounds)
		{
			continue;
		}

		if (!bNativeOverlap && bInsideGeometryBounds)
		{
			++GeometryOnlyProviderCount;
		}

		const AUTPTurtleCharacter* Turtle = Cast<AUTPTurtleCharacter>(Actor);
		const bool bCanActivate = Turtle
			? Turtle->CanActivateWeightPlate_Implementation()
			: ITPWeightProviderInterface::Execute_CanActivateWeightPlate(Actor);
		if (!bCanActivate)
		{
			continue;
		}

		TotalWeight += Turtle
			? Turtle->GetWeight_Implementation()
			: ITPWeightProviderInterface::Execute_GetWeight(Actor);
	}

	const bool bNewPressedState = TotalWeight >= RequiredWeight;
	if (bIsPressed == bNewPressedState)
	{
		return;
	}

	bIsPressed = bNewPressedState;
	UE_LOG(LogTemp, Warning, TEXT("[PressurePlate] %s: Pressed=%s, Weight=%.1f / Required=%.1f, Overlaps=%d, GeometryOnly=%d, Door=%s"),
		*GetName(), bIsPressed ? TEXT("true") : TEXT("false"), TotalWeight, RequiredWeight,
		OverlappingActors.Num(), GeometryOnlyProviderCount, *GetNameSafe(TargetDoor));
	ApplyTargetState(bIsPressed);
	OnPressedChanged.Broadcast(bIsPressed);
}

void AUTPPressurePlate::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	const bool bProvidesWeight = IsValid(OtherActor)
		&& OtherActor->GetClass()->ImplementsInterface(UTPWeightProviderInterface::StaticClass());
	const bool bCanActivate = bProvidesWeight
		&& ITPWeightProviderInterface::Execute_CanActivateWeightPlate(OtherActor);
	const float Weight = bCanActivate ? ITPWeightProviderInterface::Execute_GetWeight(OtherActor) : 0.0f;
	UE_LOG(LogTemp, Warning, TEXT("[PressurePlate] %s: BeginOverlap Actor=%s, Class=%s, WeightProvider=%s, CanActivate=%s, Weight=%.1f"),
		*GetName(), *GetNameSafe(OtherActor), *GetNameSafe(OtherActor ? OtherActor->GetClass() : nullptr),
		bProvidesWeight ? TEXT("true") : TEXT("false"), bCanActivate ? TEXT("true") : TEXT("false"), Weight);
	RefreshPressedState();
}

void AUTPPressurePlate::OnTriggerEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	UE_LOG(LogTemp, Warning, TEXT("[PressurePlate] %s: EndOverlap Actor=%s"), *GetName(), *GetNameSafe(OtherActor));
	RefreshPressedState();
}
