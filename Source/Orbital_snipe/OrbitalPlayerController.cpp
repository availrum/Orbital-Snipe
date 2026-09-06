#include "OrbitalPlayerController.h"

#include "OrbitalCannon.h"
#include "GravityManager.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"


AOrbitalPlayerController::AOrbitalPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
}


void AOrbitalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();


	// F : 대포 탑승 / 하차
	InputComponent->BindAction(
		"Interact",
		IE_Pressed,
		this,
		&AOrbitalPlayerController::Interact
	);


	// Home : Stage 종료 후 Retry
	InputComponent->BindKey(
		EKeys::Home,
		IE_Pressed,
		this,
		&AOrbitalPlayerController::RetryStage
	);
}


void AOrbitalPlayerController::Interact()
{
	APawn* CurrentPawn =
		GetPawn();


	if (!CurrentPawn)
	{
		return;
	}


	// =========================================================
	// 현재 대포 조종 중
	// -> 플레이어 Pawn으로 복귀
	// =========================================================

	if (AOrbitalCannon* Cannon =
		Cast<AOrbitalCannon>(
			CurrentPawn
		))
	{
		if (!IsValid(StoredPlayerPawn))
		{
			return;
		}


		FVector HorizontalForward =
			Cannon->GetActorForwardVector();


		HorizontalForward.Z =
			0.0f;


		if (!HorizontalForward.Normalize())
		{
			HorizontalForward =
				FVector::ForwardVector;
		}


		FVector ExitLocation =
			Cannon->GetActorLocation() -
			HorizontalForward *
			200.0f;


		const FVector TraceStart =
			ExitLocation +
			FVector(
				0.0f,
				0.0f,
				500.0f
			);


		const FVector TraceEnd =
			ExitLocation -
			FVector(
				0.0f,
				0.0f,
				1000.0f
			);


		FHitResult GroundHit;


		FCollisionQueryParams QueryParams;


		QueryParams.AddIgnoredActor(
			Cannon
		);


		QueryParams.AddIgnoredActor(
			StoredPlayerPawn
		);


		if (GetWorld()->LineTraceSingleByChannel(
			GroundHit,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			QueryParams
		))
		{
			ExitLocation.Z =
				GroundHit.ImpactPoint.Z +
				100.0f;
		}
		else
		{
			ExitLocation.Z =
				Cannon->GetActorLocation().Z +
				100.0f;
		}


		StoredPlayerPawn->SetActorLocation(
			ExitLocation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);


		Possess(
			StoredPlayerPawn
		);


		return;
	}


	// =========================================================
	// 일반 플레이어
	// -> 근처 대포 탐색
	// =========================================================

	AOrbitalCannon* Cannon =
		FindNearestCannon();


	if (!Cannon)
	{
		return;
	}


	StoredPlayerPawn =
		CurrentPawn;


	Possess(
		Cannon
	);
}


void AOrbitalPlayerController::RetryStage()
{
	AActor* ManagerActor =
		UGameplayStatics::GetActorOfClass(
			GetWorld(),
			AGravityManager::StaticClass()
		);


	AGravityManager* GravityManager =
		Cast<AGravityManager>(
			ManagerActor
		);


	if (!IsValid(GravityManager))
	{
		return;
	}


	// ============================================================
	// 핵심
	//
	// Stage 초기화 자체가 끝나지 않았으면 Home 무시
	//
	// Stage가 실제로 CLEAR / FAILED 상태가 아니어도 Home 무시
	// ============================================================

	if (!GravityManager->IsStageFinished())
	{
		return;
	}


	const FString CurrentLevelName =
		UGameplayStatics::GetCurrentLevelName(
			this,
			true
		);


	if (CurrentLevelName.IsEmpty())
	{
		return;
	}


	UGameplayStatics::OpenLevel(
		this,
		FName(
			*CurrentLevelName
		)
	);
}


AOrbitalCannon*
AOrbitalPlayerController::FindNearestCannon() const
{
	const APawn* CurrentPawn =
		GetPawn();


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


	const FVector PlayerLocation =
		CurrentPawn->GetActorLocation();


	AOrbitalCannon* NearestCannon =
		nullptr;


	float NearestDistanceSquared =
		FMath::Square(
			CannonInteractionDistance
		);


	for (AActor* Actor : Cannons)
	{
		AOrbitalCannon* Cannon =
			Cast<AOrbitalCannon>(
				Actor
			);


		if (!Cannon)
		{
			continue;
		}


		const float DistanceSquared =
			FVector::DistSquared(
				PlayerLocation,
				Cannon->GetActorLocation()
			);


		if (DistanceSquared <=
			NearestDistanceSquared)
		{
			NearestDistanceSquared =
				DistanceSquared;


			NearestCannon =
				Cannon;
		}
	}


	return NearestCannon;
}