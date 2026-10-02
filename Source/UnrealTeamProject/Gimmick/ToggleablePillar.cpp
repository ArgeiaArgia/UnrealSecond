#include "ToggleablePillar.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AToggleablePillar::AToggleablePillar()
{
    PrimaryActorTick.bCanEverTick = true;

    PillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarMesh"));
    SetRootComponent(PillarMesh);
    PillarMesh->SetMobility(EComponentMobility::Movable);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        PillarAsset = CubeMesh.Object;
        PillarMesh->SetStaticMesh(PillarAsset);
    }

    // The Engine cube is 100 cm on each side: 120 x 120 x 400 cm by default.
    PillarMesh->SetRelativeScale3D(PillarScale);
    PillarMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AToggleablePillar::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (IsValid(PillarAsset))
    {
        PillarMesh->SetStaticMesh(PillarAsset);
    }

    PillarMesh->SetRelativeScale3D(PillarScale);
}

void AToggleablePillar::BeginPlay()
{
    Super::BeginPlay();

    OffWorldLocation = GetActorLocation();
    OnWorldLocation = OffWorldLocation + GetActorTransform().TransformVectorNoScale(OnLocationOffset);
    bIsOn = bStartsOn;

    // Keep a placed pillar at its configured initial state from the first frame.
    const bool bMoveToOnLocation = bInvertOnOff ? !bIsOn : bIsOn;
    SetActorLocation(bMoveToOnLocation ? OnWorldLocation : OffWorldLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AToggleablePillar::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const bool bMoveToOnLocation = bInvertOnOff ? !bIsOn : bIsOn;
    const FVector Destination = bMoveToOnLocation ? OnWorldLocation : OffWorldLocation;
    const FVector CurrentLocation = GetActorLocation();

    if (!CurrentLocation.Equals(Destination, 0.1f))
    {
        const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, Destination, DeltaSeconds, MoveSpeed);
        SetActorLocation(NewLocation, true);
    }
}

void AToggleablePillar::SetPillarOn(bool bNewIsOn)
{
    bIsOn = bNewIsOn;
}

void AToggleablePillar::TogglePillar()
{
    SetPillarOn(!bIsOn);
}

void AToggleablePillar::SetToggleableEnabled_Implementation(bool bInEnabled)
{
    SetPillarOn(bInEnabled);
}

bool AToggleablePillar::IsToggleableEnabled_Implementation() const
{
    return bIsOn;
}
