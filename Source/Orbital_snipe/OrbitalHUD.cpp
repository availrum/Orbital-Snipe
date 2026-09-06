#include "OrbitalHUD.h"

#include "GravityManager.h"
#include "OrbitalCannon.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"


AOrbitalHUD::AOrbitalHUD()
{
}


void AOrbitalHUD::BeginPlay()
{
	Super::BeginPlay();

	FindGameplayActors();
}


void AOrbitalHUD::FindGameplayActors()
{
	// GravityManager 찾기
	if (!IsValid(GravityManager))
	{
		AActor* ManagerActor =
			UGameplayStatics::GetActorOfClass(
				GetWorld(),
				AGravityManager::StaticClass()
			);

		GravityManager =
			Cast<AGravityManager>(
				ManagerActor
			);
	}


	// Cannon 찾기
	if (!IsValid(OrbitalCannon))
	{
		AActor* CannonActor =
			UGameplayStatics::GetActorOfClass(
				GetWorld(),
				AOrbitalCannon::StaticClass()
			);

		OrbitalCannon =
			Cast<AOrbitalCannon>(
				CannonActor
			);
	}
}


void AOrbitalHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
		return;


	// Actor 참조가 없어진 경우 다시 검색
	if (!IsValid(GravityManager) ||
		!IsValid(OrbitalCannon))
	{
		FindGameplayActors();
	}


	// ============================================================
	// 왼쪽 상단 Gameplay HUD
	// ============================================================

	const float StartX = 40.0f;
	const float StartY = 40.0f;
	const float LineSpacing = 28.0f;


	// ------------------------------------------------------------
	// Shots
	// ------------------------------------------------------------

	if (IsValid(OrbitalCannon))
	{
		DrawHUDText(
			FString::Printf(
				TEXT("SHOTS   %d / %d"),
				OrbitalCannon->RemainingShots,
				OrbitalCannon->MaxShots
			),
			StartX,
			StartY,
			FLinearColor::White
		);
	}


	// ------------------------------------------------------------
	// Targets / Score
	// ------------------------------------------------------------

	if (IsValid(GravityManager))
	{
		DrawHUDText(
			FString::Printf(
				TEXT("TARGETS   %d"),
				GravityManager->RemainingTargets
			),
			StartX,
			StartY + LineSpacing,
			FLinearColor(
				0.2f,
				1.0f,
				0.3f,
				1.0f
			)
		);


		DrawHUDText(
			FString::Printf(
				TEXT("SCORE   %d"),
				GravityManager->TotalScore
			),
			StartX,
			StartY + LineSpacing * 2.0f,
			FLinearColor(
				1.0f,
				0.85f,
				0.1f,
				1.0f
			)
		);
	}


	// ============================================================
	// Clear / Failed
	// ============================================================

	if (IsValid(GravityManager))
	{
		if (GravityManager->bStageCleared)
		{
			const FString ClearText =
				TEXT("STAGE CLEAR");


			const float X =
				Canvas->ClipX * 0.5f -
				110.0f;


			const float Y =
				Canvas->ClipY * 0.18f;


			DrawHUDText(
				ClearText,
				X,
				Y,
				FLinearColor(
					0.0f,
					1.0f,
					1.0f,
					1.0f
				),
				true
			);
		}
		else if (GravityManager->bStageFailed)
		{
			const FString FailedText =
				TEXT("STAGE FAILED");


			const float X =
				Canvas->ClipX * 0.5f -
				120.0f;


			const float Y =
				Canvas->ClipY * 0.18f;


			DrawHUDText(
				FailedText,
				X,
				Y,
				FLinearColor(
					1.0f,
					0.1f,
					0.1f,
					1.0f
				),
				true
			);
		}
	}
}


void AOrbitalHUD::DrawHUDText(
	const FString& Text,
	float X,
	float Y,
	const FLinearColor& Color,
	bool bLarge
)
{
	if (!Canvas ||
		!GEngine)
	{
		return;
	}


	UFont* Font = nullptr;


	if (bLarge)
	{
		Font =
			GEngine->GetLargeFont();
	}
	else
	{
		Font =
			GEngine->GetMediumFont();
	}


	if (!Font)
		return;


	FCanvasTextItem TextItem(
		FVector2D(
			X,
			Y
		),
		FText::FromString(
			Text
		),
		Font,
		Color
	);


	// 배경에 관계없이 글자가 잘 보이도록 그림자
	TextItem.EnableShadow(
		FLinearColor::Black
	);


	Canvas->DrawItem(
		TextItem
	);
}