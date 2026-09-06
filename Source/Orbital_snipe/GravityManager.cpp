#include "GravityManager.h"

#include "Kismet/GameplayStatics.h"


AGravityManager::AGravityManager()
{
	PrimaryActorTick.bCanEverTick = true;
}


void AGravityManager::BeginPlay()
{
	Super::BeginPlay();

	// 여기서는 Target을 세지 않는다.
	//
	// OpenLevel 직후 World Partition Actor들이 아직 준비되지 않은
	// 상태에서 GetAllActorsOfClass가 실행될 수 있기 때문이다.
	//
	// 실제 Stage 초기화는 Tick의 TryInitializeStage()에서 수행한다.

	AllBodies.Empty();
	ActiveBodyTimes.Empty();

	TotalScore = 0;
	RemainingTargets = 0;

	bStageInitialized = false;
	bStageCleared = false;
	bStageFailed = false;

	bShotsExhausted = false;

	NextShotId = 1;

	bActiveAreaCenterInitialized = false;
	ActiveAreaCenter = FVector::ZeroVector;

	LastDetectedTargetCount = -1;
	StableTargetScanFrames = 0;
}


void AGravityManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	// ============================================================
	// Stage 초기화
	//
	// Target들이 World Partition에서 실제로 준비된 이후에만
	// 게임 로직을 시작한다.
	// ============================================================

	if (!bStageInitialized)
	{
		TryInitializeStage();

		return;
	}


	// ============================================================
	// 물리
	// ============================================================

	ApplyGravity(
		DeltaTime * TimeScale
	);


	// ============================================================
	// 활동시간 / 범위 밖 Body 제거
	// ============================================================

	UpdateActiveBodies(
		DeltaTime
	);


	// ============================================================
	// Clear
	// ============================================================

	if (!bStageCleared &&
		!bStageFailed &&
		RemainingTargets == 0)
	{
		bStageCleared = true;
	}


	// ============================================================
	// Failed
	// ============================================================

	if (bShotsExhausted &&
		!bStageCleared &&
		!bStageFailed &&
		RemainingTargets > 0 &&
		!HasActiveChainObjects())
	{
		bStageFailed = true;
	}
}


void AGravityManager::TryInitializeStage()
{
	TArray<AActor*> FoundActors;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AGravityBody::StaticClass(),
		FoundActors
	);


	TArray<AGravityBody*> DetectedBodies;

	int32 DetectedTargetCount = 0;


	for (AActor* Actor : FoundActors)
	{
		AGravityBody* Body =
			Cast<AGravityBody>(
				Actor
			);


		if (!IsValid(Body))
		{
			continue;
		}


		DetectedBodies.Add(
			Body
		);


		if (Body->BodyType ==
			EGravityBodyType::Target &&
			!Body->bHasBeenHit)
		{
			DetectedTargetCount++;
		}
	}


	// Target이 아직 하나도 준비되지 않았다면
	// 초기화하지 않는다.
	if (DetectedTargetCount <= 0)
	{
		LastDetectedTargetCount = -1;
		StableTargetScanFrames = 0;

		return;
	}


	// 이전 프레임과 같은 Target 개수가 확인되면
	// 안정적으로 로딩되고 있다고 판단
	if (DetectedTargetCount ==
		LastDetectedTargetCount)
	{
		StableTargetScanFrames++;
	}
	else
	{
		LastDetectedTargetCount =
			DetectedTargetCount;

		StableTargetScanFrames =
			1;
	}


	// 한 프레임만 보고 초기화하지 않고
	// 연속된 몇 프레임 동안 같은 개수인지 확인
	if (StableTargetScanFrames <
		RequiredStableTargetScanFrames)
	{
		return;
	}


	// ============================================================
	// Stage 초기화 완료
	// ============================================================

	AllBodies =
		MoveTemp(
			DetectedBodies
		);


	RemainingTargets =
		DetectedTargetCount;


	TotalScore = 0;

	bStageCleared = false;
	bStageFailed = false;

	bShotsExhausted = false;

	NextShotId = 1;

	ActiveBodyTimes.Empty();

	bActiveAreaCenterInitialized = false;

	ActiveAreaCenter =
		FVector::ZeroVector;


	bStageInitialized = true;
}


bool AGravityManager::IsStageFinished() const
{
	return
		bStageInitialized &&
		(
			bStageCleared ||
			bStageFailed
			);
}


void AGravityManager::ApplyGravity(float dt)
{
	// ============================================================
	// 프레임 시작 시 Hit 상태 저장
	// ============================================================

	for (AGravityBody* Body : AllBodies)
	{
		if (!IsValid(Body))
		{
			continue;
		}


		Body->bWasHitBeforeThisFrame =
			Body->bHasBeenHit;
	}


	// ============================================================
	// 중력
	// ============================================================

	for (int32 i = 0;
		i < AllBodies.Num();
		i++)
	{
		AGravityBody* BodyA =
			AllBodies[i];


		if (!IsValid(BodyA))
		{
			continue;
		}


		if (BodyA->BodyType !=
			EGravityBodyType::Projectile)
		{
			continue;
		}


		FVector TotalForce =
			FVector::ZeroVector;


		for (int32 j = 0;
			j < AllBodies.Num();
			j++)
		{
			if (i == j)
			{
				continue;
			}


			AGravityBody* BodyB =
				AllBodies[j];


			if (!IsValid(BodyB))
			{
				continue;
			}


			if (BodyB->BodyType !=
				EGravityBodyType::Target)
			{
				continue;
			}


			if (BodyB->bHasBeenHit)
			{
				continue;
			}


			FVector Direction =
				BodyB->GetActorLocation() -
				BodyA->GetActorLocation();


			float Distance =
				Direction.Size();


			if (Distance <
				(
					BodyA->Radius +
					BodyB->Radius
					))
			{
				continue;
			}


			if (Distance <=
				KINDA_SMALL_NUMBER)
			{
				continue;
			}


			float ForceMagnitude =
				GravitationalConstant *
				(
					BodyA->Mass *
					BodyB->Mass
					) /
				(
					Distance *
					Distance
					);


			TotalForce +=
				Direction.GetSafeNormal() *
				ForceMagnitude;
		}


		if (BodyA->Mass <=
			KINDA_SMALL_NUMBER)
		{
			continue;
		}


		FVector Acceleration =
			TotalForce /
			BodyA->Mass;


		BodyA->InitialVelocity +=
			Acceleration *
			dt;
	}


	// ============================================================
	// 위치 업데이트
	// ============================================================

	for (AGravityBody* Body : AllBodies)
	{
		if (!IsValid(Body))
		{
			continue;
		}


		const bool bShouldMove =
			Body->BodyType ==
			EGravityBodyType::Projectile ||
			Body->bHasBeenHit;


		if (!bShouldMove)
		{
			continue;
		}


		const FVector NewActorPos =
			Body->GetActorLocation() +
			(
				Body->InitialVelocity *
				dt
				);


		Body->SetActorLocation(
			NewActorPos
		);
	}


	// ============================================================
	// 충돌
	// ============================================================

	for (int32 i = 0;
		i < AllBodies.Num();
		i++)
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
				PosA -
				PosB;


			float Distance =
				Normal.Size();


			float MinDist =
				BodyA->Radius +
				BodyB->Radius;


			if (Distance >=
				MinDist)
			{
				continue;
			}


			if (Distance <=
				KINDA_SMALL_NUMBER)
			{
				continue;
			}


			Normal.Normalize();


			FVector RelVel =
				BodyA->InitialVelocity -
				BodyB->InitialVelocity;


			float VelAlongNormal =
				FVector::DotProduct(
					RelVel,
					Normal
				);


			if (VelAlongNormal >
				0.0f)
			{
				continue;
			}


			// ====================================================
			// 위치 보정
			// ====================================================

			float TotalMass =
				BodyA->Mass +
				BodyB->Mass;


			if (TotalMass <=
				KINDA_SMALL_NUMBER)
			{
				continue;
			}


			float Overlap =
				MinDist -
				Distance;


			float MoveA =
				Overlap *
				(
					BodyB->Mass /
					TotalMass
					);


			float MoveB =
				Overlap *
				(
					BodyA->Mass /
					TotalMass
					);


			BodyA->SetActorLocation(
				PosA +
				Normal *
				MoveA
			);


			BodyB->SetActorLocation(
				PosB -
				Normal *
				MoveB
			);


			// ====================================================
			// 탄성 충돌
			// ====================================================

			if (BodyA->Mass <=
				KINDA_SMALL_NUMBER ||
				BodyB->Mass <=
				KINDA_SMALL_NUMBER)
			{
				continue;
			}


			const float Restitution =
				1.0f;


			float ImpulseMagnitude =
				-(1.0f + Restitution) *
				VelAlongNormal;


			ImpulseMagnitude /=
				(
					1.0f /
					BodyA->Mass +
					1.0f /
					BodyB->Mass
					);


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
					BodyB->ChainDepth +
					1;


				TotalScore +=
					100 *
					BodyA->ChainDepth;


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
					BodyA->ChainDepth +
					1;


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


void AGravityManager::UpdateActiveBodies(
	float DeltaTime
)
{
	const float ActiveAreaRadiusSq =
		ActiveAreaRadius *
		ActiveAreaRadius;


	for (int32 i =
		AllBodies.Num() - 1;
		i >= 0;
		i--)
	{
		AGravityBody* Body =
			AllBodies[i];


		if (!IsValid(Body))
		{
			AllBodies.RemoveAt(
				i
			);

			continue;
		}


		const bool bIsProjectile =
			Body->BodyType ==
			EGravityBodyType::Projectile;


		const bool bIsActivatedTarget =
			Body->BodyType ==
			EGravityBodyType::Target &&
			Body->bHasBeenHit;


		if (!bIsProjectile &&
			!bIsActivatedTarget)
		{
			continue;
		}


		TWeakObjectPtr<AGravityBody>
			BodyKey(
				Body
			);


		float* ActiveTime =
			ActiveBodyTimes.Find(
				BodyKey
			);


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
			(
				*ActiveTime >=
				ActiveBodyLifetime
				);


		bool bOutOfBounds =
			false;


		if (bActiveAreaCenterInitialized &&
			ActiveAreaRadius >
			0.0f)
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


		if (bLifetimeExpired ||
			bOutOfBounds)
		{
			ActiveBodyTimes.Remove(
				BodyKey
			);


			AllBodies.RemoveAt(
				i
			);


			Body->Destroy();
		}
	}


	for (auto It =
		ActiveBodyTimes.CreateIterator();
		It;
		++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}


void AGravityManager::RegisterActivatedTarget(
	AGravityBody* Target
)
{
	if (!IsValid(Target))
	{
		return;
	}


	TWeakObjectPtr<AGravityBody>
		TargetKey(
			Target
		);


	ActiveBodyTimes.FindOrAdd(
		TargetKey
	) = 0.0f;
}


void AGravityManager::AddPlanet(
	AGravityBody* NewPlanet
)
{
	if (!bStageInitialized)
	{
		return;
	}


	if (!IsValid(NewPlanet))
	{
		return;
	}


	if (NewPlanet->BodyType ==
		EGravityBodyType::Projectile)
	{
		NewPlanet->ShotId =
			NextShotId++;


		NewPlanet->ChainDepth =
			0;


		if (!bActiveAreaCenterInitialized)
		{
			ActiveAreaCenter =
				NewPlanet->GetActorLocation();


			bActiveAreaCenterInitialized =
				true;
		}


		TWeakObjectPtr<AGravityBody>
			ProjectileKey(
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


void AGravityManager::NotifyShotsExhausted()
{
	if (!bStageInitialized)
	{
		return;
	}


	bShotsExhausted =
		true;
}


bool AGravityManager::HasActiveChainObjects() const
{
	for (AGravityBody* Body : AllBodies)
	{
		if (!IsValid(Body))
		{
			continue;
		}


		if (Body->BodyType ==
			EGravityBodyType::Projectile)
		{
			return true;
		}


		if (Body->BodyType ==
			EGravityBodyType::Target &&
			Body->bHasBeenHit)
		{
			return true;
		}
	}


	return false;
}