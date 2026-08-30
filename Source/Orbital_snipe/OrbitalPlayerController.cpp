#include "OrbitalPlayerController.h"
#include "OrbitalCannon.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AOrbitalPlayerController::AOrbitalPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
}

void AOrbitalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// F: 대포 탑승 / 하차
	InputComponent->BindAction(
		"Interact",
		IE_Pressed,
		this,
		&AOrbitalPlayerController::Interact
	);
}

void AOrbitalPlayerController::Interact()
{
	APawn* CurrentPawn = GetPawn();

	if (!CurrentPawn)
	{
		return;
	}

	// =========================================================
	// 1. 현재 대포를 조종하고 있다면 -> 기존 플레이어 Pawn으로 복귀
	// =========================================================
	if (AOrbitalCannon* Cannon = Cast<AOrbitalCannon>(CurrentPawn))
	{
		if (!IsValid(StoredPlayerPawn))
		{
			return;
		}

		// 대포가 위/아래를 보고 있어도
		// 하차 방향은 수평 방향만 사용한다.
		FVector HorizontalForward = Cannon->GetActorForwardVector();
		HorizontalForward.Z = 0.0f;

		if (!HorizontalForward.Normalize())
		{
			HorizontalForward = FVector::ForwardVector;
		}

		// 대포 뒤쪽 200cm 지점
		FVector ExitLocation =
			Cannon->GetActorLocation()
			- HorizontalForward * 200.0f;

		// 해당 위치의 실제 지면 탐색
		const FVector TraceStart =
			ExitLocation + FVector(0.0f, 0.0f, 500.0f);

		const FVector TraceEnd =
			ExitLocation - FVector(0.0f, 0.0f, 1000.0f);

		FHitResult GroundHit;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Cannon);
		QueryParams.AddIgnoredActor(StoredPlayerPawn);

		if (GetWorld()->LineTraceSingleByChannel(
			GroundHit,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			QueryParams))
		{
			// 지면보다 100cm 위에 플레이어 배치
			ExitLocation.Z = GroundHit.ImpactPoint.Z + 100.0f;
		}
		else
		{
			// 지면을 찾지 못했을 경우 fallback
			ExitLocation.Z = Cannon->GetActorLocation().Z + 100.0f;
		}

		StoredPlayerPawn->SetActorLocation(
			ExitLocation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);

		Possess(StoredPlayerPawn);

		return;
	}

	// =========================================================
	// 2. 일반 플레이어 상태라면 -> 근처 대포 탐색
	// =========================================================
	AOrbitalCannon* Cannon = FindNearestCannon();

	if (!Cannon)
	{
		return;
	}

	// 현재 플레이어를 기억해 두고 대포로 조종권 이동
	StoredPlayerPawn = CurrentPawn;

	Possess(Cannon);
}

AOrbitalCannon* AOrbitalPlayerController::FindNearestCannon() const
{
	const APawn* CurrentPawn = GetPawn();

	if (!CurrentPawn)
	{
		return nullptr;
	}

	TArray<AActor*> Cannons;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AOrbitalCannon::StaticClass(),
		Cannons
	);

	const FVector PlayerLocation = CurrentPawn->GetActorLocation();

	AOrbitalCannon* NearestCannon = nullptr;

	float NearestDistanceSquared =
		FMath::Square(CannonInteractionDistance);

	for (AActor* Actor : Cannons)
	{
		AOrbitalCannon* Cannon = Cast<AOrbitalCannon>(Actor);

		if (!Cannon)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(
			PlayerLocation,
			Cannon->GetActorLocation()
		);

		if (DistanceSquared <= NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestCannon = Cannon;
		}
	}

	return NearestCannon;
}