#include "GravityManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h" // 궤적 그리기용
#include "Components/StaticMeshComponent.h"

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
}

// 3. 매 프레임 실행 (Tick) - 이것도 없어서 에러 났었음
void AGravityManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 배속 적용해서 물리 계산 실행
    ApplyGravity(DeltaTime * TimeScale);
}

// 4. 물리 연산 핵심 로직
void AGravityManager::ApplyGravity(float dt)
{
    // 1. [중력 단계] 모든 행성 간의 중력 가속도 계산 (속도만 변경)
    for (int32 i = 0; i < AllBodies.Num(); i++)
    {
        AGravityBody* BodyA = AllBodies[i];
        FVector TotalForce = FVector::ZeroVector;

        for (int32 j = 0; j < AllBodies.Num(); j++)
        {
            if (i == j) continue;
            AGravityBody* BodyB = AllBodies[j];

            FVector Direction = BodyB->GetActorLocation() - BodyA->GetActorLocation();
            float Distance = Direction.Size();

            // 너무 가까우면 중력 계산 스킵 (발산 방지)
            if (Distance < (BodyA->Radius + BodyB->Radius)) continue;

            // F = G * m1 * m2 / r^2
            float ForceMagnitude = GravitationalConstant * (BodyA->Mass * BodyB->Mass) / (Distance * Distance);
            TotalForce += Direction.GetSafeNormal() * ForceMagnitude;
        }

        FVector Acceleration = TotalForce / BodyA->Mass;
        BodyA->InitialVelocity += Acceleration * dt;
    }

 // [2] 위치 업데이트 및 궤적 그리기 (중심점 기준 수정 버전)
    for (AGravityBody* Body : AllBodies)
    {
        // 1. 이동하기 전의 "진짜 중심(Center)" 좌표를 가져옴
        // (Bounds.Origin은 피벗 상관없이 무조건 도형의 정중앙을 줍니다)
        FVector OldCenter = Body->MeshComponent->Bounds.Origin;

        // 2. 위치 이동 (피벗 기준 이동)
        FVector NewActorPos = Body->GetActorLocation() + (Body->InitialVelocity * dt);
        Body->SetActorLocation(NewActorPos);

        // [중요] 위치를 옮겼으니, 메쉬의 중심점 정보(Bounds)도 강제로 새로고침 해줘야 함
        Body->MeshComponent->UpdateBounds();

        // 3. 이동한 후의 "진짜 중심(Center)" 좌표 가져옴
        FVector NewCenter = Body->MeshComponent->Bounds.Origin;

        // 4. 이제 중심에서 중심으로 선을 긋습니다
        DrawDebugLine(
            GetWorld(),
            OldCenter,   // 이전 중심
            NewCenter,   // 현재 중심
            FColor::Red,
            false, 5.0f, 0, 2.0f
        );
    }

    // 3. [충돌 단계] 운동량 보존 법칙 적용 (Elastic Collision)
    // 이중 루프를 돌되, 중복 검사(A-B, B-A)를 피하기 위해 j = i + 1 부터 시작
    for (int32 i = 0; i < AllBodies.Num(); i++)
    {
        for (int32 j = i + 1; j < AllBodies.Num(); j++)
        {
            AGravityBody* BodyA = AllBodies[i];
            AGravityBody* BodyB = AllBodies[j];

            FVector PosA = BodyA->GetActorLocation();
            FVector PosB = BodyB->GetActorLocation();
            FVector Normal = PosA - PosB; // 충돌 면의 법선 벡터
            float Distance = Normal.Size();
            float MinDist = BodyA->Radius + BodyB->Radius;

            // 충돌 감지
            if (Distance < MinDist)
            {
                Normal.Normalize();

                // (1) 위치 보정 (겹친 만큼 서로 밀어내기 - 질량 반비례)
                float Overlap = MinDist - Distance;
                float TotalMass = BodyA->Mass + BodyB->Mass;

                // 무거운 놈은 조금 밀리고, 가벼운 놈은 많이 밀림
                float MoveA = Overlap * (BodyB->Mass / TotalMass);
                float MoveB = Overlap * (BodyA->Mass / TotalMass);

                BodyA->SetActorLocation(PosA + Normal * MoveA);
                BodyB->SetActorLocation(PosB - Normal * MoveB);

                // (2) 속도 반응 (운동량 보존 법칙 공식)
                // 상대 속도 계산
                FVector RelVel = BodyA->InitialVelocity - BodyB->InitialVelocity;
                float VelAlongNormal = FVector::DotProduct(RelVel, Normal);

                // 이미 멀어지고 있는 중이면 계산 안 함
                if (VelAlongNormal > 0) continue;

                // 반발 계수 (1.0 = 완전 탄성 충돌, 탱탱볼 / 0.5 = 약간의 에너지 손실)
                float Restitution = 1.0f;

                // 충격량(Impulse) 스칼라 계산
                float j_impulse = -(1 + Restitution) * VelAlongNormal;
                j_impulse /= (1 / BodyA->Mass + 1 / BodyB->Mass);

                // 충격량 벡터
                FVector Impulse = j_impulse * Normal;

                // 속도에 적용
                BodyA->InitialVelocity += Impulse / BodyA->Mass;
                BodyB->InitialVelocity -= Impulse / BodyB->Mass;
            }
        }
    }
}
void AGravityManager::AddPlanet(AGravityBody* NewPlanet)
{
    if (NewPlanet){
        AllBodies.Add(NewPlanet);
    }
}