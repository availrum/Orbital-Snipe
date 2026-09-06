#include "GravityManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"


// ============================================================
// Constructor
// ============================================================

AGravityManager::AGravityManager()
{
	PrimaryActorTick.bCanEverTick = true;
}


// ============================================================
// BeginPlay
// ============================================================

void AGravityManager::BeginPlay()
{
	Super::BeginPlay();

	// 월드에 배치되어 있는 모든 GravityBody 검색
	TArray<AActor*> FoundActors;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AGravityBody::StaticClass(),
		FoundActors
	);

	for (AActor* Actor : FoundActors)
	{
		if (AGravityBody* Body = Cast<AGravityBody>(Actor))
		{
			AllBodies.Add(Body);
		}
	}


	// 시작 시 아직 맞지 않은 Target 개수 계산
	RemainingTargets = 0;

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


// ============================================================
// Tick
// ============================================================

void AGravityManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	// ------------------------------------------------------------
	// 1. 물리 계산
	// ------------------------------------------------------------

	ApplyGravity(
		DeltaTime * TimeScale
	);


	// ------------------------------------------------------------
	// 2. 활성 물체의 활동시간 / 범위 검사
	// ------------------------------------------------------------

	UpdateActiveBodies(
		DeltaTime
	);


	// ------------------------------------------------------------
	// 3. Stage Clear
	// ------------------------------------------------------------

	if (!bStageCleared &&
		RemainingTargets == 0)
	{
		bStageCleared = true;
		bStageFailed = false;
	}


	// ------------------------------------------------------------
	// 4. Stage Failed
	//
	// 모든 탄환을 사용했고,
	// 아직 Target이 남아 있으며,
	// 더 이상 연쇄충돌 가능한 활성 물체가 없을 때 실패
	// ------------------------------------------------------------

	if (bShotsExhausted &&
		!bStageCleared &&
		!bStageFailed &&
		RemainingTargets > 0 &&
		!HasActiveChainObjects())
	{
		bStageFailed = true;
	}


	// ------------------------------------------------------------
	// 5. 임시 Debug UI
	// ------------------------------------------------------------

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			0.0f,
			FColor::Yellow,
			FString::Printf(
				TEXT("Score: %d"),
				TotalScore
			)
		);

		GEngine->AddOnScreenDebugMessage(
			3,
			0.0f,
			FColor::Green,
			FString::Printf(
				TEXT("Targets: %d"),
				RemainingTargets
			)
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
		else if (bStageFailed)
		{
			GEngine->AddOnScreenDebugMessage(
				4,
				0.0f,
				FColor::Red,
				TEXT("STAGE FAILED!")
			);
		}
	}
}


// ============================================================
// ApplyGravity
// ============================================================

void AGravityManager::ApplyGravity(float dt)
{
	// ============================================================
	// 0. 프레임 시작 시점의 Hit 상태 저장
	//
	// Target A가 이번 프레임에 맞았다고 해서
	// 같은 프레임 안에서 A → B → C가 한꺼번에 활성화되는 것을 방지
	// ============================================================

	for (AGravityBody* Body : AllBodies)
	{
		if (!IsValid(Body))
			continue;

		Body->bWasHitBeforeThisFrame =
			Body->bHasBeenHit;
	}


	// ============================================================
	// 1. 중력 계산
	//
	// Projectile만 중력 영향을 받는다.
	// 아직 맞지 않은 Target만 고정 중력원으로 사용한다.
	// ============================================================

	for (int32 i = 0; i < AllBodies.Num(); i++)
	{
		AGravityBody* BodyA =
			AllBodies[i];

		if (!IsValid(BodyA))
			continue;


		// Target 자체는 중력에 의해 움직이지 않는다.
		if (BodyA->BodyType !=
			EGravityBodyType::Projectile)
		{
			continue;
		}


		FVector TotalForce =
			FVector::ZeroVector;


		for (int32 j = 0; j < AllBodies.Num(); j++)
		{
			if (i == j)
				continue;


			AGravityBody* BodyB =
				AllBodies[j];

			if (!IsValid(BodyB))
				continue;


			// Target만 중력원
			if (BodyB->BodyType !=
				EGravityBodyType::Target)
			{
				continue;
			}


			// 이미 맞아서 움직이기 시작한 Target은
			// 더 이상 고정 중력원으로 사용하지 않는다.
			if (BodyB->bHasBeenHit)
				continue;


			FVector Direction =
				BodyB->GetActorLocation() -
				BodyA->GetActorLocation();


			float Distance =
				Direction.Size();


			// 이미 충돌할 정도로 가까운 경우
			// 중력 계산 생략
			if (Distance <
				(BodyA->Radius + BodyB->Radius))
			{
				continue;
			}


			if (Distance <= KINDA_SMALL_NUMBER)
				continue;


			float ForceMagnitude =
				GravitationalConstant *
				(BodyA->Mass * BodyB->Mass) /
				(Distance * Distance);


			TotalForce +=
				Direction.GetSafeNormal() *
				ForceMagnitude;
		}


		FVector Acceleration =
			TotalForce /
			BodyA->Mass;


		BodyA->InitialVelocity +=
			Acceleration * dt;
	}


	// ============================================================
	// 2. 위치 업데이트
	//
	// Projectile은 항상 이동
	// Target은 최초 명중 이후 이동
	// ============================================================

	for (AGravityBody* Body : AllBodies)
	{
		if (!IsValid(Body))
			continue;


		const bool bShouldMove =
			Body->BodyType ==
			EGravityBodyType::Projectile ||
			Body->bHasBeenHit;


		if (!bShouldMove)
			continue;


		FVector NewActorPos =
			Body->GetActorLocation() +
			(Body->InitialVelocity * dt);


		Body->SetActorLocation(
			NewActorPos
		);
	}


	// ============================================================
	// 3. 충돌
	//
	// Projectile → Target
	// Hit Target  → Target
	// ============================================================

	for (int32 i = 0; i < AllBodies.Num(); i++)
	{
		for (int32 j = i + 1;
			j < AllBodies.Num();
			j++)
		{
			AGravityBody* BodyA =
				AllBodies[i];

			AGravityBody* BodyB =
				AllBodies[j];


			if (!IsValid(BodyA) ||
				!IsValid(BodyB))
			{
				continue;
			}


			const bool bAWasMovable =
				BodyA->BodyType ==
				EGravityBodyType::Projectile ||
				BodyA->bWasHitBeforeThisFrame;


			const bool bBWasMovable =
				BodyB->BodyType ==
				EGravityBodyType::Projectile ||
				BodyB->bWasHitBeforeThisFrame;


			// 둘 다 아직 움직이지 않은 Target이면
			// 충돌 계산 불필요
			if (!bAWasMovable &&
				!bBWasMovable)
			{
				continue;
			}


			FVector PosA =
				BodyA->GetActorLocation();

			FVector PosB =
				BodyB->GetActorLocation();


			FVector Normal =
				PosA - PosB;


			float Distance =
				Normal.Size();


			float MinDist =
				BodyA->Radius +
				BodyB->Radius;


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
					FVector::DotProduct(
						RelVel,
						Normal
					);


				// 이미 서로 멀어지는 중이면
				// 같은 충돌을 다시 처리하지 않는다.
				if (VelAlongNormal > 0.0f)
					continue;


				// ====================================================
				// 위치 보정
				// ====================================================

				float Overlap =
					MinDist -
					Distance;


				float TotalMass =
					BodyA->Mass +
					BodyB->Mass;


				if (TotalMass <= KINDA_SMALL_NUMBER)
					continue;


				float MoveA =
					Overlap *
					(BodyB->Mass / TotalMass);


				float MoveB =
					Overlap *
					(BodyA->Mass / TotalMass);


				BodyA->SetActorLocation(
					PosA +
					Normal * MoveA
				);


				BodyB->SetActorLocation(
					PosB -
					Normal * MoveB
				);


				// ====================================================
				// 탄성 충돌
				// ====================================================

				if (BodyA->Mass <= KINDA_SMALL_NUMBER ||
					BodyB->Mass <= KINDA_SMALL_NUMBER)
				{
					continue;
				}


				const float Restitution =
					1.0f;


				float ImpulseMagnitude =
					-(1.0f + Restitution) *
					VelAlongNormal;


				ImpulseMagnitude /=
					(1.0f / BodyA->Mass +
						1.0f / BodyB->Mass);


				FVector Impulse =
					ImpulseMagnitude *
					Normal;


				BodyA->InitialVelocity +=
					Impulse /
					BodyA->Mass;


				BodyB->InitialVelocity -=
					Impulse /
					BodyB->Mass;


				// ====================================================
				// BodyA Target 최초 명중
				// ====================================================

				if (BodyA->BodyType ==
					EGravityBodyType::Target &&
					bBWasMovable &&
					!BodyA->bHasBeenHit)
				{
					BodyA->bHasBeenHit =
						true;


					RemainingTargets =
						FMath::Max(
							0,
							RemainingTargets - 1
						);


					BodyA->ShotId =
						BodyB->ShotId;


					BodyA->ChainDepth =
						BodyB->ChainDepth + 1;


					TotalScore +=
						100 *
						BodyA->ChainDepth;


					// 이 순간부터 개별 활동시간 측정 시작
					RegisterActivatedTarget(
						BodyA
					);
				}


				// ====================================================
				// BodyB Target 최초 명중
				// ====================================================

				if (BodyB->BodyType ==
					EGravityBodyType::Target &&
					bAWasMovable &&
					!BodyB->bHasBeenHit)
				{
					BodyB->bHasBeenHit =
						true;


					RemainingTargets =
						FMath::Max(
							0,
							RemainingTargets - 1
						);


					BodyB->ShotId =
						BodyA->ShotId;


					BodyB->ChainDepth =
						BodyA->ChainDepth + 1;


					TotalScore +=
						100 *
						BodyB->ChainDepth;


					RegisterActivatedTarget(
						BodyB
					);
				}
			}
		}
	}
}


// ============================================================
// UpdateActiveBodies
//
// Projectile / Hit Target의:
// 1. 최대 활동시간
// 2. 플레이 영역
//
// 두 조건을 검사해서 제거한다.
// ============================================================

void AGravityManager::UpdateActiveBodies(float DeltaTime)
{
	const float ActiveAreaRadiusSq =
		ActiveAreaRadius *
		ActiveAreaRadius;


	// 배열에서 삭제하면서 순회하므로 반드시 뒤에서부터 검사
	for (int32 i = AllBodies.Num() - 1;
		i >= 0;
		i--)
	{
		AGravityBody* Body =
			AllBodies[i];


		// 이미 외부에서 파괴된 Actor라면
		// AllBodies에서도 제거
		if (!IsValid(Body))
		{
			AllBodies.RemoveAt(i);
			continue;
		}


		const bool bIsProjectile =
			Body->BodyType ==
			EGravityBodyType::Projectile;


		const bool bIsActivatedTarget =
			Body->BodyType ==
			EGravityBodyType::Target &&
			Body->bHasBeenHit;


		// 아직 맞지 않은 고정 Target은
		// 제거 대상이 아님
		if (!bIsProjectile &&
			!bIsActivatedTarget)
		{
			continue;
		}


		TWeakObjectPtr<AGravityBody> BodyKey(
			Body
		);


		float* ActiveTime =
			ActiveBodyTimes.Find(
				BodyKey
			);


		// 혹시 등록이 누락된 활성 물체가 있더라도
		// 여기서 안전하게 등록
		if (!ActiveTime)
		{
			ActiveBodyTimes.Add(
				BodyKey,
				0.0f
			);

			ActiveTime =
				ActiveBodyTimes.Find(
					BodyKey
				);
		}


		if (ActiveTime)
		{
			*ActiveTime +=
				DeltaTime;
		}


		const bool bLifetimeExpired =
			ActiveTime &&
			(*ActiveTime >= ActiveBodyLifetime);


		bool bOutOfBounds =
			false;


		if (bActiveAreaCenterInitialized &&
			ActiveAreaRadius > 0.0f)
		{
			const float DistanceSq =
				FVector::DistSquared(
					Body->GetActorLocation(),
					ActiveAreaCenter
				);


			bOutOfBounds =
				DistanceSq >
				ActiveAreaRadiusSq;
		}


		// 활동시간 초과 또는 범위 밖
		if (bLifetimeExpired ||
			bOutOfBounds)
		{
			// 중요:
			// Actor를 Destroy하기 전에
			// Manager 내부 자료구조에서 먼저 제거
			ActiveBodyTimes.Remove(
				BodyKey
			);

			AllBodies.RemoveAt(
				i
			);

			Body->Destroy();
		}
	}


	// 이미 파괴된 TWeakObjectPtr 키 정리
	for (auto It = ActiveBodyTimes.CreateIterator();
		It;
		++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}


// ============================================================
// RegisterActivatedTarget
// ============================================================

void AGravityManager::RegisterActivatedTarget(
	AGravityBody* Target
)
{
	if (!IsValid(Target))
		return;


	TWeakObjectPtr<AGravityBody> TargetKey(
		Target
	);


	// Target 최초 피격 시 활동시간 0초부터 시작
	ActiveBodyTimes.FindOrAdd(
		TargetKey
	) = 0.0f;
}


// ============================================================
// AddPlanet
// ============================================================

void AGravityManager::AddPlanet(
	AGravityBody* NewPlanet
)
{
	if (!IsValid(NewPlanet))
		return;


	if (NewPlanet->BodyType ==
		EGravityBodyType::Projectile)
	{
		NewPlanet->ShotId =
			NextShotId++;


		NewPlanet->ChainDepth =
			0;


		// 첫 번째 발사체 위치를
		// 플레이 영역의 기준점으로 사용
		if (!bActiveAreaCenterInitialized)
		{
			ActiveAreaCenter =
				NewPlanet->GetActorLocation();


			bActiveAreaCenterInitialized =
				true;
		}


		// Projectile은 발사 순간부터 활동시간 측정
		TWeakObjectPtr<AGravityBody> ProjectileKey(
			NewPlanet
		);


		ActiveBodyTimes.Add(
			ProjectileKey,
			0.0f
		);
	}


	AllBodies.Add(
		NewPlanet
	);
}


// ============================================================
// NotifyShotsExhausted
// ============================================================

void AGravityManager::NotifyShotsExhausted()
{
	bShotsExhausted =
		true;
}


// ============================================================
// HasActiveChainObjects
// ============================================================

bool AGravityManager::HasActiveChainObjects() const
{
	for (AGravityBody* Body : AllBodies)
	{
		if (!IsValid(Body))
			continue;


		// 살아 있는 Projectile
		if (Body->BodyType ==
			EGravityBodyType::Projectile)
		{
			return true;
		}


		// 이미 맞아서 움직이는 Target
		if (Body->BodyType ==
			EGravityBodyType::Target &&
			Body->bHasBeenHit)
		{
			return true;
		}
	}


	return false;
}