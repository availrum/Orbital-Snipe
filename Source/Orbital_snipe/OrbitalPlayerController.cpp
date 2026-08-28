#include "OrbitalPlayerController.h"
#include "OrbitalCannon.h"            
#include "GameFramework/GameModeBase.h" 
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AOrbitalPlayerController::AOrbitalPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = true;
}

void AOrbitalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// "Interact" 키(F) 바인딩
	InputComponent->BindAction("Interact", IE_Pressed, this, &AOrbitalPlayerController::Interact);
}

void AOrbitalPlayerController::Interact()
{
	APawn* CurrentPawn = GetPawn();

	// 1. [하차 로직] 대포에 타고 있는가?
	if (CurrentPawn && CurrentPawn->IsA(AOrbitalCannon::StaticClass()))
	{
		UnPossess();

		// 자유 카메라로 돌아오기
		AGameModeBase* GM = UGameplayStatics::GetGameMode(GetWorld());
		if (GM && GM->DefaultPawnClass)
		{
			// 대포 뒤쪽(-200cm) + 위쪽(+100cm)에 하차
			FVector ExitLoc = CurrentPawn->GetActorLocation() - (CurrentPawn->GetActorForwardVector() * 200.0f) + FVector(0, 0, 100);
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			APawn* NewPawn = GetWorld()->SpawnActor<APawn>(GM->DefaultPawnClass, ExitLoc, FRotator::ZeroRotator, P);

			if (NewPawn)
			{
				Possess(NewPawn);
			}
		}
		return;
	}

	// 2. [탑승 로직] 자유 상태라면 대포를 찾자
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (Hit.GetActor() && Hit.GetActor()->IsA(AOrbitalCannon::StaticClass()))
	{
		Possess(Cast<APawn>(Hit.GetActor()));
	}
}