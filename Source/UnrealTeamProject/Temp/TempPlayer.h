#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h" // 향상된 입력 값 헤더
#include "TempPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class UNREALTEAMPROJECT_API ATempPlayer : public ACharacter
{
    GENERATED_BODY()

public:
    ATempPlayer();

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "Prototype")
    void ToggleSoulState();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* Camera;

    // 향상된 입력 에셋 참조 변수들
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* JumpAction;

    bool bIsSoulMode;

    // 향상된 입력용 이동/회전 함수 (파라미터가 FInputActionValue로 변경됨)
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
};