#include "OrbitalCannon.h"
#include "GravityBody.h"
#include "GravityManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"

AOrbitalCannon::AOrbitalCannon()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 받침대 (좌우 회전의 중심)
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	// 2. 포신 (상하 회전) - 받침대 자식으로 붙임
	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMesh->SetupAttachment(BaseMesh);

	// 3. 발사구 - 포신 끝에 붙임
	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(BarrelMesh);

	// 4. 대포 카메라 - 포신 위에 달아서 같이 움직이게 함
	CannonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CannonCamera"));
	CannonCamera->SetupAttachment(BarrelMesh);
}

void AOrbitalCannon::BeginPlay()
{
	Super::BeginPlay();

	// 매니저 찾아두기
	AActor* ManagerActor =
		UGameplayStatics::GetActorOfClass(
			GetWorld(),
			AGravityManager::StaticClass()
		);

	CachedManager = Cast<AGravityManager>(ManagerActor);
}

void AOrbitalCannon::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Input Mapping (Project Settings에서 이름 맞춰야 함)
	PlayerInputComponent->BindAxis("MoveRight", this, &AOrbitalCannon::RotateCannon); // A/D
	PlayerInputComponent->BindAxis("MoveForward", this, &AOrbitalCannon::ElevateCannon); // W/S
	PlayerInputComponent->BindAxis("PowerAdj", this, &AOrbitalCannon::AdjustPower); // Q/E (Axis로 설정 필요)
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AOrbitalCannon::Fire);
}

void AOrbitalCannon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. 회전 적용 (현재 변수 값대로 메쉬를 돌림)
	BaseMesh->SetRelativeRotation(
		FRotator(0.0f, CurrentYaw, 0.0f)
	);

	BarrelMesh->SetRelativeRotation(
		FRotator(0.0f, 0.0f, CurrentPitch)
	);
	// 2. 궤적 그리기 (탑승 여부와 상관없이 항상 그림!)
	DrawTrajectory();
}

void AOrbitalCannon::RotateCannon(float Val)
{
	if (Val != 0.f)
	{
		CurrentYaw += Val; // 회전 속도 조절 가능
	}
}

void AOrbitalCannon::ElevateCannon(float Val)
{
	if (Val != 0.f)
	{
		// 각도 제한 (-10도 ~ 80도)
		CurrentPitch = FMath::Clamp(CurrentPitch + Val, -10.0f, 80.0f);
	}
}

void AOrbitalCannon::AdjustPower(float Val)
{
	if (Val != 0.f)
	{
		CurrentPower = FMath::Clamp(CurrentPower + (Val * 10.0f), 100.0f, 5000.0f);
	}
}

void AOrbitalCannon::DrawTrajectory()
{
	if (!CachedManager) return;

	// 시작점: 포구 끝
	FVector StartPos = ProjectileSpawnPoint->GetComponentLocation();
	// 발사 방향: 포구의 전방 벡터
	FVector LaunchDir = -BarrelMesh->GetRightVector();

	FVector CurrentPos = StartPos;
	FVector CurrentVel = LaunchDir * CurrentPower;

	int32 MaxSteps = 300;
	float StepTime = 0.016f;

	TArray<AActor*> Planets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGravityBody::StaticClass(), Planets);

	for (int32 i = 0; i < MaxSteps; i++)
	{
		FVector TotalForce = FVector::ZeroVector;

		for (AActor* P : Planets)
		{
			AGravityBody* Planet = Cast<AGravityBody>(P);
			if (!Planet) continue;


			// Projectile은 중력원이 아니다.
			if (Planet->BodyType != EGravityBodyType::Target)
				continue;

			FVector Dir =
				Planet->GetActorLocation() - CurrentPos;

			float DistSq = Dir.SizeSquared();

			if (DistSq < FMath::Square(Planet->Radius))
				break;

			// 이미 맞아서 움직이기 시작한 Target은
			// 더 이상 고정 중력원으로 사용하지 않는다.
			if (Planet->bHasBeenHit)
				continue;

			float Force =
				CachedManager->GravitationalConstant *
				Planet->Mass /
				DistSq;

			TotalForce +=
				Dir.GetSafeNormal() * Force;
		}

		CurrentVel += TotalForce * StepTime;
		FVector NextPos = CurrentPos + (CurrentVel * StepTime);

		if (i % 3 == 0) // 점선 간격
		{
			DrawDebugPoint(GetWorld(), NextPos, 3.0f, FColor::Cyan, false, -1.0f);
		}
		CurrentPos = NextPos;
	}
}

void AOrbitalCannon::Fire()
{
	if (PlanetClass)
	{
		FVector SpawnLoc = ProjectileSpawnPoint->GetComponentLocation();
		FRotator SpawnRot = ProjectileSpawnPoint->GetComponentRotation();

		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AGravityBody* NewPlanet = GetWorld()->SpawnActor<AGravityBody>(PlanetClass, SpawnLoc, SpawnRot, P);
		if (NewPlanet)
		{
			NewPlanet->BodyType = EGravityBodyType::Projectile;
			NewPlanet->InitialVelocity = -BarrelMesh->GetRightVector() * CurrentPower;
			if (CachedManager) CachedManager->AddPlanet(NewPlanet);
		}
	}
}