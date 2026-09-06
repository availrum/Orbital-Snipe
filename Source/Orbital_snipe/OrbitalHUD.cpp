#include "OrbitalHUD.h"

#include "GravityManager.h"
#include "OrbitalCannon.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"


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
	// ============================================================
	// GravityManager
	// ============================================================

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


	// ============================================================
	// OrbitalCannon
	// ============================================================

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
	{
		return;
	}


	// Actor 참조가 유효하지 않으면 다시 검색
	if (!IsValid(GravityManager) ||
		!IsValid(OrbitalCannon))
	{
		FindGameplayActors();
	}


	// ============================================================
	// 화면 크기 기반 HUD Scale
	// ============================================================

	const float ScreenScale =
		FMath::Clamp(
			Canvas->ClipX / 1920.0f,
			0.75f,
			1.25f
		);


	// ============================================================
	// 좌측 Status Panel
	// ============================================================

	const float PanelX =
		40.0f *
		ScreenScale;


	const float PanelY =
		40.0f *
		ScreenScale;


	const float PanelWidth =
		300.0f *
		ScreenScale;


	const float PanelHeight =
		145.0f *
		ScreenScale;


	DrawRect(
		FLinearColor(
			0.0f,
			0.0f,
			0.0f,
			0.55f
		),
		PanelX,
		PanelY,
		PanelWidth,
		PanelHeight
	);


	const float LabelX =
		PanelX +
		22.0f *
		ScreenScale;


	const float ValueX =
		PanelX +
		190.0f *
		ScreenScale;


	const float FirstLineY =
		PanelY +
		24.0f *
		ScreenScale;


	const float LineSpacing =
		38.0f *
		ScreenScale;


	const float TextScale =
		0.95f *
		ScreenScale;


	// ============================================================
	// SHOTS
	// ============================================================

	DrawHUDText(
		TEXT("SHOTS"),
		LabelX,
		FirstLineY,
		FLinearColor::White,
		TextScale
	);


	if (IsValid(OrbitalCannon))
	{
		DrawHUDText(
			FString::Printf(
				TEXT("%d / %d"),
				OrbitalCannon->RemainingShots,
				OrbitalCannon->MaxShots
			),
			ValueX,
			FirstLineY,
			FLinearColor::White,
			TextScale
		);
	}


	// ============================================================
	// TARGETS
	// ============================================================

	DrawHUDText(
		TEXT("TARGETS"),
		LabelX,
		FirstLineY +
		LineSpacing,
		FLinearColor(
			0.25f,
			1.0f,
			0.35f,
			1.0f
		),
		TextScale
	);


	if (IsValid(GravityManager))
	{
		DrawHUDText(
			FString::Printf(
				TEXT("%d"),
				GravityManager->RemainingTargets
			),
			ValueX,
			FirstLineY +
			LineSpacing,
			FLinearColor(
				0.25f,
				1.0f,
				0.35f,
				1.0f
			),
			TextScale
		);
	}


	// ============================================================
	// SCORE
	// ============================================================

	DrawHUDText(
		TEXT("SCORE"),
		LabelX,
		FirstLineY +
		LineSpacing *
		2.0f,
		FLinearColor(
			1.0f,
			0.85f,
			0.15f,
			1.0f
		),
		TextScale
	);


	if (IsValid(GravityManager))
	{
		DrawHUDText(
			FormatScore(
				GravityManager->TotalScore
			),
			ValueX,
			FirstLineY +
			LineSpacing *
			2.0f,
			FLinearColor(
				1.0f,
				0.85f,
				0.15f,
				1.0f
			),
			TextScale
		);
	}


	// GravityManager가 없으면 여기서 종료
	if (!IsValid(GravityManager))
	{
		return;
	}


	// ============================================================
	// 게임 진행 중 조작 안내
	// ============================================================

	if (!GravityManager->bStageCleared &&
		!GravityManager->bStageFailed)
	{
		bool bIsControllingCannon =
			false;


		if (IsValid(PlayerOwner))
		{
			APawn* ControlledPawn =
				PlayerOwner->GetPawn();


			if (Cast<AOrbitalCannon>(ControlledPawn))
			{
				bIsControllingCannon =
					true;
			}
		}


		// ========================================================
		// Control Panel 위치
		// ========================================================

		const float ControlPanelWidth =
			360.0f *
			ScreenScale;


		const float ControlPanelHeight =
			(
				bIsControllingCannon
				? 245.0f
				: 135.0f
				) *
			ScreenScale;


		const float ControlPanelX =
			Canvas->ClipX -
			ControlPanelWidth -
			40.0f *
			ScreenScale;


		const float ControlPanelY =
			Canvas->ClipY -
			ControlPanelHeight -
			80.0f *
			ScreenScale;


		DrawRect(
			FLinearColor(
				0.0f,
				0.0f,
				0.0f,
				0.62f
			),
			ControlPanelX,
			ControlPanelY,
			ControlPanelWidth,
			ControlPanelHeight
		);


		const float ControlKeyX =
			ControlPanelX +
			22.0f *
			ScreenScale;


		const float ControlActionX =
			ControlPanelX +
			145.0f *
			ScreenScale;


		const float ControlHeaderY =
			ControlPanelY +
			18.0f *
			ScreenScale;


		const float ControlFirstLineY =
			ControlPanelY +
			52.0f *
			ScreenScale;


		const float ControlLineSpacing =
			34.0f *
			ScreenScale;


		const float ControlHeaderScale =
			1.00f *
			ScreenScale;


		const float ControlTextScale =
			0.98f *
			ScreenScale;


		const FLinearColor KeyColor =
			FLinearColor::White;


		const FLinearColor ActionColor =
			FLinearColor(
				0.75f,
				0.75f,
				0.75f,
				1.0f
			);


		// ========================================================
		// CANNON CONTROLS
		// ========================================================

		if (bIsControllingCannon)
		{
			DrawHUDText(
				TEXT("CANNON CONTROLS"),
				ControlKeyX,
				ControlHeaderY,
				FLinearColor(
					0.20f,
					0.90f,
					1.0f,
					1.0f
				),
				ControlHeaderScale
			);


			// A / D
			DrawHUDText(
				TEXT("A / D"),
				ControlKeyX,
				ControlFirstLineY,
				KeyColor,
				ControlTextScale
			);

			DrawHUDText(
				TEXT("ROTATE"),
				ControlActionX,
				ControlFirstLineY,
				ActionColor,
				ControlTextScale
			);


			// W / S
			DrawHUDText(
				TEXT("W / S"),
				ControlKeyX,
				ControlFirstLineY +
				ControlLineSpacing,
				KeyColor,
				ControlTextScale
			);

			DrawHUDText(
				TEXT("ELEVATE"),
				ControlActionX,
				ControlFirstLineY +
				ControlLineSpacing,
				ActionColor,
				ControlTextScale
			);


			// Q / E
			DrawHUDText(
				TEXT("Q / E"),
				ControlKeyX,
				ControlFirstLineY +
				ControlLineSpacing *
				2.0f,
				KeyColor,
				ControlTextScale
			);

			DrawHUDText(
				TEXT("POWER"),
				ControlActionX,
				ControlFirstLineY +
				ControlLineSpacing *
				2.0f,
				ActionColor,
				ControlTextScale
			);


			// Left Mouse Button
			DrawHUDText(
				TEXT("LMB"),
				ControlKeyX,
				ControlFirstLineY +
				ControlLineSpacing *
				3.0f,
				KeyColor,
				ControlTextScale
			);

			DrawHUDText(
				TEXT("FIRE"),
				ControlActionX,
				ControlFirstLineY +
				ControlLineSpacing *
				3.0f,
				ActionColor,
				ControlTextScale
			);


			// F
			DrawHUDText(
				TEXT("F"),
				ControlKeyX,
				ControlFirstLineY +
				ControlLineSpacing *
				4.0f,
				KeyColor,
				ControlTextScale
			);

			DrawHUDText(
				TEXT("EXIT CANNON"),
				ControlActionX,
				ControlFirstLineY +
				ControlLineSpacing *
				4.0f,
				ActionColor,
				ControlTextScale
			);
		}


		// ========================================================
		// PLAYER CONTROLS
		// ========================================================

		else
		{
			DrawHUDText(
				TEXT("PLAYER CONTROLS"),
				ControlKeyX,
				ControlHeaderY,
				FLinearColor(
					0.20f,
					0.90f,
					1.0f,
					1.0f
				),
				ControlHeaderScale
			);


			// WASD
			DrawHUDText(
				TEXT("WASD"),
				ControlKeyX,
				ControlFirstLineY,
				KeyColor,
				ControlTextScale
			);

			DrawHUDText(
				TEXT("MOVE"),
				ControlActionX,
				ControlFirstLineY,
				ActionColor,
				ControlTextScale
			);


			// F
			DrawHUDText(
				TEXT("F"),
				ControlKeyX,
				ControlFirstLineY +
				ControlLineSpacing,
				KeyColor,
				ControlTextScale
			);

			DrawHUDText(
				TEXT("ENTER CANNON"),
				ControlActionX,
				ControlFirstLineY +
				ControlLineSpacing,
				ActionColor,
				ControlTextScale
			);
		}


		// 플레이 중에는 Result Panel 없음
		return;
	}


	// ============================================================
	// Result Panel
	// ============================================================

	const float ResultPanelWidth =
		500.0f *
		ScreenScale;

	const float ResultPanelHeight =
		150.0f *
		ScreenScale;

	const float ResultPanelX =
		(
			Canvas->ClipX -
			ResultPanelWidth
			) *
		0.5f;

	const float ResultPanelY =
		Canvas->ClipY *
		0.15f;

	const float ResultTextY =
		ResultPanelY +
		24.0f *
		ScreenScale;

	const float RetryTextY =
		ResultPanelY +
		98.0f *
		ScreenScale;

	const float ResultTextScale =
		2.3f *
		ScreenScale;

	const float RetryTextScale =
		1.0f *
		ScreenScale;

	// ============================================================
	// STAGE CLEAR
	// ============================================================

	if (GravityManager->bStageCleared)
	{
		DrawRect(
			FLinearColor(
				0.0f,
				0.07f,
				0.08f,
				0.92f
			),
			ResultPanelX,
			ResultPanelY,
			ResultPanelWidth,
			ResultPanelHeight
		);


		DrawCenteredHUDText(
			TEXT("STAGE CLEAR"),
			Canvas->ClipX *
			0.5f,
			ResultTextY,
			FLinearColor(
				0.1f,
				1.0f,
				1.0f,
				1.0f
			),
			ResultTextScale
		);
	}


	// ============================================================
	// STAGE FAILED
	// ============================================================

	else if (GravityManager->bStageFailed)
	{
		DrawRect(
			FLinearColor(
				0.10f,
				0.0f,
				0.0f,
				0.92f
			),
			ResultPanelX,
			ResultPanelY,
			ResultPanelWidth,
			ResultPanelHeight
		);


		DrawCenteredHUDText(
			TEXT("STAGE FAILED"),
			Canvas->ClipX *
			0.5f,
			ResultTextY,
			FLinearColor(
				1.0f,
				0.15f,
				0.15f,
				1.0f
			),
			ResultTextScale
		);
	}


	// ============================================================
	// RETRY 안내
	// ============================================================

	DrawCenteredHUDText(
		TEXT("HOME : RETRY"),
		Canvas->ClipX *
		0.5f,
		RetryTextY,
		FLinearColor(
			0.8f,
			0.8f,
			0.8f,
			1.0f
		),
		RetryTextScale
	);
}


void AOrbitalHUD::DrawHUDText(
	const FString& Text,
	float X,
	float Y,
	const FLinearColor& Color,
	float Scale
)
{
	if (!Canvas ||
		!GEngine)
	{
		return;
	}


	UFont* Font =
		GEngine->GetMediumFont();


	if (!Font)
	{
		return;
	}


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


	TextItem.Scale =
		FVector2D(
			Scale,
			Scale
		);


	TextItem.EnableShadow(
		FLinearColor::Black
	);


	Canvas->DrawItem(
		TextItem
	);
}


void AOrbitalHUD::DrawCenteredHUDText(
	const FString& Text,
	float CenterX,
	float Y,
	const FLinearColor& Color,
	float Scale
)
{
	if (!Canvas ||
		!GEngine)
	{
		return;
	}


	UFont* Font =
		GEngine->GetLargeFont();


	if (!Font)
	{
		return;
	}


	float TextWidth =
		0.0f;


	float TextHeight =
		0.0f;


	Canvas->StrLen(
		Font,
		Text,
		TextWidth,
		TextHeight
	);


	TextWidth *=
		Scale;


	const float X =
		CenterX -
		TextWidth *
		0.5f;


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


	TextItem.Scale =
		FVector2D(
			Scale,
			Scale
		);


	TextItem.EnableShadow(
		FLinearColor::Black
	);


	Canvas->DrawItem(
		TextItem
	);
}


FString AOrbitalHUD::FormatScore(
	int32 Score
) const
{
	FString Number =
		FString::FromInt(
			Score
		);


	for (int32 i =
		Number.Len() - 3;
		i > 0;
		i -= 3)
	{
		Number.InsertAt(
			i,
			TEXT(",")
		);
	}


	return Number;
}