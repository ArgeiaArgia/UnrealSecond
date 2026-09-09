// Fill out your copyright notice in the Description page of Project Settings.

#include "TempPlayer.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#define ECC_Soul ECC_GameTraceChannel1

ATempPlayer::ATempPlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsSoulMode = false;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 400.0f;
    SpringArm->bUsePawnControlRotation = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
}

void ATempPlayer::BeginPlay()
{
    Super::BeginPlay();

    // 로컬 플레이어 서브시스템에 매핑 컨텍스트 추가
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void ATempPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // EnhancedInputComponent로 캐스팅하여 액션 바인딩
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // 이동
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATempPlayer::Move);
        // 회전 (시점)
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATempPlayer::Look);
        // 점프
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
    }
}

void ATempPlayer::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void ATempPlayer::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void ATempPlayer::ToggleSoulState()
{
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (!Capsule) return;

    if (bIsSoulMode)
    {
        Capsule->SetCollisionObjectType(ECC_Pawn);
        bIsSoulMode = false;
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Change: Body"));
    }
    else
    {
        Capsule->SetCollisionObjectType(ECC_Soul);
        bIsSoulMode = true;
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("Change: Soul"));
    }
}