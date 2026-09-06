#include "OrbitalCannon.h"
#include "GravityBody.h"
#include "GravityManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

AOrbitalCannon::AOrbitalCannon()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 받침대 (좌우 회전의 중심)
	BaseMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("BaseMesh")
		);

	RootComponent = BaseMesh;

	// 2. 포신 (상하 회전)
	BarrelMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("BarrelMesh")
		);

	BarrelMesh->SetupAttachment(BaseMesh);

	// 3. 발사구
	ProjectileSpawnPoint =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("SpawnPoint")
		);

	ProjectileSpawnPoint->SetupAttachment(
		BarrelMesh
	);

	// 4. 대포 카메라
	CannonCamera =
		CreateDefaultSubobject<UCameraComponent>(
			TEXT("CannonCamera")
		);

	CannonCamera->SetupAttachment(
		BarrelMesh
	);
}

void AOrbitalCannon::BeginPlay()
{
	Super::BeginPlay();

	AActor* ManagerActor =
		UGameplayStatics::GetActorOfClass(
			GetWorld(),
			AGravityManager::StaticClass()
		);

	CachedManager =
		Cast<AGravityManager>(
			ManagerActor
		);

	RemainingShots =
		MaxShots;
}

void AOrbitalCannon::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent
)
{
	Super::SetupPlayerInputComponent(
		PlayerInputComponent
	);

	PlayerInputComponent->BindAxis(
		"MoveRight",
		this,
		&AOrbitalCannon::RotateCannon
	);

	PlayerInputComponent->BindAxis(
		"MoveForward",
		this,
		&AOrbitalCannon::ElevateCannon
	);

	PlayerInputComponent->BindAxis(
		"PowerAdj",
		this,
		&AOrbitalCannon::AdjustPower
	);

	PlayerInputComponent->BindAction(
		"Fire",
		IE_Pressed,
		this,
		&AOrbitalCannon::Fire
	);
}

void AOrbitalCannon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	BaseMesh->SetRelativeRotation(
		FRotator(
			0.0f,
			CurrentYaw,
			0.0f
		)
	);

	BarrelMesh->SetRelativeRotation(
		FRotator(
			0.0f,
			0.0f,
			CurrentPitch
		)
	);

	DrawTrajectory();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			2,
			0.0f,
			FColor::White,
			FString::Printf(
				TEXT("Shots: %d / %d"),
				RemainingShots,
				MaxShots
			)
		);
	}
}

void AOrbitalCannon::RotateCannon(
	float Val
)
{
	if (Val != 0.0f)
	{
		CurrentYaw += Val;
	}
}

void AOrbitalCannon::ElevateCannon(
	float Val
)
{
	if (Val != 0.0f)
	{
		CurrentPitch =
			FMath::Clamp(
				CurrentPitch + Val,
				-10.0f,
				80.0f
			);
	}
}

void AOrbitalCannon::AdjustPower(
	float Val
)
{
	if (Val != 0.0f)
	{
		CurrentPower =
			FMath::Clamp(
				CurrentPower +
				(Val * 10.0f),
				100.0f,
				5000.0f
			);
	}
}

void AOrbitalCannon::DrawTrajectory()
{
	if (!CachedManager)
		return;

	// 시작점: 포구 끝
	FVector StartPos =
		ProjectileSpawnPoint
		->GetComponentLocation();

	// 실제 발사 방향
	FVector LaunchDir =
		-BarrelMesh->GetRightVector();

	FVector CurrentPos =
		StartPos;

	FVector CurrentVel =
		LaunchDir *
		CurrentPower;

	const int32 MaxSteps =
		300;

	const float StepTime =
		0.016f;

	TArray<AActor*> Planets;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AGravityBody::StaticClass(),
		Planets
	);

	for (int32 i = 0;
		i < MaxSteps;
		i++)
	{
		FVector TotalForce =
			FVector::ZeroVector;

		bool bHitTarget =
			false;

		for (AActor* P : Planets)
		{
			AGravityBody* Planet =
				Cast<AGravityBody>(P);

			if (!Planet)
				continue;

			// Projectile은 중력원이 아님
			if (Planet->BodyType !=
				EGravityBodyType::Target)
			{
				continue;
			}

			FVector Dir =
				Planet->GetActorLocation() -
				CurrentPos;

			float DistSq =
				Dir.SizeSquared();

			// Target과 충돌할 위치라면
			// 예측선 종료
			if (DistSq <
				FMath::Square(
					Planet->Radius
				))
			{
				bHitTarget =
					true;

				break;
			}

			// 이미 맞은 Target은
			// 중력원에서 제외
			if (Planet->bHasBeenHit)
				continue;

			// 0으로 나누는 상황 방지
			if (DistSq <=
				KINDA_SMALL_NUMBER)
			{
				continue;
			}

			float Force =
				CachedManager
				->GravitationalConstant *
				Planet->Mass /
				DistSq;

			TotalForce +=
				Dir.GetSafeNormal() *
				Force;
		}

		if (bHitTarget)
			break;

		CurrentVel +=
			TotalForce *
			StepTime;

		FVector NextPos =
			CurrentPos +
			(CurrentVel *
				StepTime);

		if (i % 3 == 0)
		{
			DrawDebugPoint(
				GetWorld(),
				NextPos,
				3.0f,
				FColor::Cyan,
				false,
				-1.0f
			);
		}

		CurrentPos =
			NextPos;
	}
}

void AOrbitalCannon::Fire()
{
	if (RemainingShots <= 0)
		return;

	if (!PlanetClass)
		return;

	FVector SpawnLoc =
		ProjectileSpawnPoint
		->GetComponentLocation();

	FRotator SpawnRot =
		ProjectileSpawnPoint
		->GetComponentRotation();

	FActorSpawnParameters P;

	P.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod
		::AlwaysSpawn;

	AGravityBody* NewPlanet =
		GetWorld()
		->SpawnActor<AGravityBody>(
			PlanetClass,
			SpawnLoc,
			SpawnRot,
			P
		);

	if (!NewPlanet)
		return;

	NewPlanet->BodyType =
		EGravityBodyType::Projectile;

	NewPlanet->InitialVelocity =
		-BarrelMesh->GetRightVector() *
		CurrentPower;

	if (CachedManager)
	{
		CachedManager->AddPlanet(
			NewPlanet
		);
	}

	RemainingShots--;

	// 마지막 탄환 사용
	if (RemainingShots == 0 &&
		CachedManager)
	{
		CachedManager
			->NotifyShotsExhausted();
	}
}