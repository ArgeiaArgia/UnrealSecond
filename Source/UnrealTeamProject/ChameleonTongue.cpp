#include "ChameleonTongue.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AChameleonTongue::AChameleonTongue()
{
    PrimaryActorTick.bCanEverTick = false;

    // 1. 충돌체 설정
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(15.0f);

    // Tongue 채널 (이전에 세팅한 채널 번호가 2번이라고 가정)
    CollisionComp->SetCollisionObjectType(ECC_GameTraceChannel2);
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    // 기본적으로 모두 무시하되, 스위치(WorldDynamic)와는 겹치고 벽(WorldStatic)에는 부딪히게 설정
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

    RootComponent = CollisionComp;

    // 2. 메쉬 설정 (눈에 보이는 형태)
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 메쉬는 물리 간섭 안 함

    // 3. 투사체 이동 컴포넌트 설정
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = 3000.0f; // 날아가는 속도
    ProjectileMovement->MaxSpeed = 3000.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->ProjectileGravityScale = 0.0f; // 중력 무시 (직선으로 날아감)

    // 4. 태그 추가 (스위치가 이 태그를 확인하고 작동함)
    Tags.Add(FName("Tongue"));

    // 5. 수명 설정 (허공으로 날아가면 3초 뒤 자동 메모리 해제)
    InitialLifeSpan = 3.0f;

    // 벽 등에 부딪혔을 때 호출될 이벤트 바인딩
    CollisionComp->OnComponentHit.AddDynamic(this, &AChameleonTongue::OnHit);
}

void AChameleonTongue::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 벽에 부딪히면 혀(액터) 파괴
    Destroy();
}