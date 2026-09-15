#include "UTPTurtleCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AUTPTurtleCharacter::AUTPTurtleCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(60.0f, 48.0f);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -48.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 160.0f;
		Movement->MaxAcceleration = 900.0f;
		Movement->BrakingDecelerationWalking = 900.0f;
	}

	bDisableMovementWhenUnpossessed = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> TurtleMeshAsset(
		TEXT("/Game/_Art/QuirkyMinimal/SnappingTurtle/Models/SnappingTurtle_LOD0.SnappingTurtle_LOD0"));
	if (TurtleMeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(TurtleMeshAsset.Object);
	}
}

void AUTPTurtleCharacter::BeginPlay()
{
	Super::BeginPlay();
	bIsBracing = !IsSoulPossessed();
}

void AUTPTurtleCharacter::OnPossessedBySoul_Implementation(APawn* SoulPawn)
{
	Super::OnPossessedBySoul_Implementation(SoulPawn);
	bIsBracing = false;
}

void AUTPTurtleCharacter::OnReleasedFromSoul_Implementation(APawn* SoulPawn)
{
	Super::OnReleasedFromSoul_Implementation(SoulPawn);
	bIsBracing = true;
}

float AUTPTurtleCharacter::GetWeight_Implementation() const
{
	return BraceMass;
}

bool AUTPTurtleCharacter::CanActivateWeightPlate_Implementation() const
{
	return bIsBracing;
}
