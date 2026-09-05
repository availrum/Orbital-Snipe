#include "GravityManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h" // 궤적 그리기용
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

// 1. 생성자 (Constructor) - 이게 없어서 에러 났었음
AGravityManager::AGravityManager()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick을 돌리겠다고 선언
}

// 2. 게임 시작 (BeginPlay) - 이것도 없어서 에러 났었음
void AGravityManager::BeginPlay()
{
    Super::BeginPlay();

    // 월드의 모든 GravityBody 찾기
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGravityBody::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (AGravityBody* Body = Cast<AGravityBody>(Actor))
        {
            AllBodies.Add(Body);
        }
    }
    for (AGravityBody* Body : AllBodies)
    {
        if (!IsValid(Body))
            continue;

        if (Body->BodyType == EGravityBodyType::Target &&
            !Body->bHasBeenHit)
        {
            RemainingTargets++;
        }
    }

}

// 3. 매 프레임 실행 (Tick) - 이것도 없어서 에러 났었음

void AGravityManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ApplyGravity(DeltaTime * TimeScale);

    // 모든 Target을 최초 1회 이상 맞히면 Stage Clear
    if (!bStageCleared && RemainingTargets == 0)
    {
        bStageCleared = true;
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            1,
            0.0f,
            FColor::Yellow,
            FString::Printf(TEXT("Score: %d"), TotalScore)
        );

        GEngine->AddOnScreenDebugMessage(
            3,
            0.0f,
            FColor::Green,
            FString::Printf(TEXT("Targets: %d"), RemainingTargets)
        );

        if (bStageCleared)
        {
            GEngine->AddOnScreenDebugMessage(
                4,
                0.0f,
                FColor::Cyan,
                TEXT("STAGE CLEAR!")
            );
        }
    }
}

// 4. 물리 연산 핵심 로직
void AGravityManager::ApplyGravity(float dt)
{
    // ============================================================
    // 1. 중력 계산
    // Projectile만 중력 영향을 받는다.
    // 아직 맞지 않은 Target만 고정된 중력원으로 사용한다.
    // ============================================================
    for (AGravityBody* Body : AllBodies)
    {
        if (!IsValid(Body))
            continue;

        Body->bWasHitBeforeThisFrame = Body->bHasBeenHit;
    }

    for (int32 i = 0; i < AllBodies.Num(); i++)
    {
        AGravityBody* BodyA = AllBodies[i];

        if (!IsValid(BodyA))
            continue;

        // Target은 중력 때문에 스스로 움직이지 않는다.
        if (BodyA->BodyType != EGravityBodyType::Projectile)
            continue;

        FVector TotalForce = FVector::ZeroVector;

        for (int32 j = 0; j < AllBodies.Num(); j++)
        {
            if (i == j)
                continue;

            AGravityBody* BodyB = AllBodies[j];

            if (!IsValid(BodyB))
                continue;

            // 중력원은 아직 맞지 않은 Target만 사용
            if (BodyB->BodyType != EGravityBodyType::Target)
                continue;

            if (BodyB->bHasBeenHit)
                continue;

            FVector Direction =
                BodyB->GetActorLocation() -
                BodyA->GetActorLocation();

            float Distance = Direction.Size();

            // 이미 충돌할 정도로 가까우면 중력 계산 생략
            if (Distance < (BodyA->Radius + BodyB->Radius))
                continue;

            float ForceMagnitude =
                GravitationalConstant *
                (BodyA->Mass * BodyB->Mass) /
                (Distance * Distance);

            TotalForce +=
                Direction.GetSafeNormal() * ForceMagnitude;
        }

        FVector Acceleration =
            TotalForce / BodyA->Mass;

        BodyA->InitialVelocity +=
            Acceleration * dt;
    }


    // ============================================================
    // 2. 위치 업데이트
    //
    // Projectile은 항상 이동
    // Target은 한 번 맞은 뒤에만 이동
    // ============================================================
    for (AGravityBody* Body : AllBodies)
    {
        if (!IsValid(Body))
            continue;

        const bool bShouldMove =
            Body->BodyType == EGravityBodyType::Projectile ||
            Body->bHasBeenHit;

        if (!bShouldMove)
            continue;

        FVector OldCenter =
            Body->MeshComponent->Bounds.Origin;

        FVector NewActorPos =
            Body->GetActorLocation() +
            (Body->InitialVelocity * dt);

        Body->SetActorLocation(NewActorPos);

        Body->MeshComponent->UpdateBounds();

        FVector NewCenter =
            Body->MeshComponent->Bounds.Origin;

        //DrawDebugLine(
        //    GetWorld(),
        //    OldCenter,
        //    NewCenter,
        //    FColor::Red,
        //    false,
        //    2.0f,
        //    0,
        //    2.0f
        //);
    }


    // ============================================================
    // 3. 충돌
    //
    // Projectile → Target
    // Hit Target → Target
    //
    // 맞은 Target은 활성화되어 이후부터 움직인다.
    // ============================================================
    for (int32 i = 0; i < AllBodies.Num(); i++)
    {
        for (int32 j = i + 1; j < AllBodies.Num(); j++)
        {
            AGravityBody* BodyA = AllBodies[i];
            AGravityBody* BodyB = AllBodies[j];

            if (!IsValid(BodyA) || !IsValid(BodyB))
                continue;

            const bool bAWasMovable =
                BodyA->BodyType == EGravityBodyType::Projectile ||
                BodyA->bWasHitBeforeThisFrame;

            const bool bBWasMovable =
                BodyB->BodyType == EGravityBodyType::Projectile ||
                BodyB->bWasHitBeforeThisFrame;

            // 둘 다 아직 움직이지 않는 Target이면
            // 서로 충돌 계산할 필요 없음
            if (!bAWasMovable && !bBWasMovable)
                continue;

            FVector PosA = BodyA->GetActorLocation();
            FVector PosB = BodyB->GetActorLocation();

            FVector Normal = PosA - PosB;

            float Distance = Normal.Size();
            float MinDist =
                BodyA->Radius + BodyB->Radius;

            if (Distance < MinDist)
            {
                // 중심이 완전히 같은 특수 상황 방지
                if (Distance <= KINDA_SMALL_NUMBER)
                    continue;

                Normal.Normalize();

                FVector RelVel =
                    BodyA->InitialVelocity -
                    BodyB->InitialVelocity;

                float VelAlongNormal =
                    FVector::DotProduct(RelVel, Normal);

                // 이미 서로 멀어지는 중
                if (VelAlongNormal > 0.0f)
                    continue;


                // ----------------------------------------
                // 위치 보정
                // ----------------------------------------
                float Overlap =
                    MinDist - Distance;

                float TotalMass =
                    BodyA->Mass + BodyB->Mass;

                float MoveA =
                    Overlap *
                    (BodyB->Mass / TotalMass);

                float MoveB =
                    Overlap *
                    (BodyA->Mass / TotalMass);

                BodyA->SetActorLocation(
                    PosA + Normal * MoveA
                );

                BodyB->SetActorLocation(
                    PosB - Normal * MoveB
                );


                // ----------------------------------------
                // 탄성 충돌
                // ----------------------------------------
                float Restitution = 1.0f;

                float j_impulse =
                    -(1.0f + Restitution) *
                    VelAlongNormal;

                j_impulse /=
                    (1.0f / BodyA->Mass +
                        1.0f / BodyB->Mass);

                FVector Impulse =
                    j_impulse * Normal;

                BodyA->InitialVelocity +=
                    Impulse / BodyA->Mass;

                BodyB->InitialVelocity -=
                    Impulse / BodyB->Mass;


                // ----------------------------------------
                // Target 활성화
                //
                // 움직이던 물체가 Target을 치면
                // 그 Target도 이후부터 움직일 수 있다.
                // ----------------------------------------
                if (BodyA->BodyType == EGravityBodyType::Target &&
                    bBWasMovable &&
                    !BodyA->bHasBeenHit)
                {
                    BodyA->bHasBeenHit = true;
                    RemainingTargets = FMath::Max(0, RemainingTargets - 1);

                    BodyA->ShotId = BodyB->ShotId;
                    BodyA->ChainDepth = BodyB->ChainDepth + 1;

                    TotalScore += 100 * BodyA->ChainDepth;

                    BodyA->SetLifeSpan(30.0f);
                }

                if (BodyB->BodyType == EGravityBodyType::Target &&
                    bAWasMovable &&
                    !BodyB->bHasBeenHit)
                {
                    BodyB->bHasBeenHit = true;
                    RemainingTargets = FMath::Max(0, RemainingTargets - 1);

                    BodyB->ShotId = BodyA->ShotId;
                    BodyB->ChainDepth = BodyA->ChainDepth + 1;

                    TotalScore += 100 * BodyB->ChainDepth;

                    BodyB->SetLifeSpan(30.0f);
                }

            }
        }
    }
}
void AGravityManager::AddPlanet(AGravityBody* NewPlanet)
{
    if (!NewPlanet)
        return;

    if (NewPlanet->BodyType == EGravityBodyType::Projectile)
    {
        NewPlanet->ShotId = NextShotId++;
        NewPlanet->ChainDepth = 0;
    }

    AllBodies.Add(NewPlanet);
}