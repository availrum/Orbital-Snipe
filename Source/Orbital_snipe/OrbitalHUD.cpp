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


	// 반투명 검은 배경
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


	// ============================================================
	// 결과 판정이 아직 없으면
	// Status HUD만 그리고 종료
	// ============================================================

	if (!IsValid(GravityManager))
	{
		return;
	}


	if (!GravityManager->bStageCleared &&
		!GravityManager->bStageFailed)
	{
		return;
	}


	// ============================================================
	// Result Panel
	// ============================================================

	const float ResultPanelWidth =
		340.0f *
		ScreenScale;


	// Retry 안내 한 줄을 추가했으므로
	// 기존 80보다 조금 높임
	const float ResultPanelHeight =
		105.0f *
		ScreenScale;


	const float ResultPanelX =
		(
			Canvas->ClipX -
			ResultPanelWidth
			) *
		0.5f;


	const float ResultPanelY =
		Canvas->ClipY *
		0.17f;


	const float ResultTextY =
		ResultPanelY +
		15.0f *
		ScreenScale;


	const float RetryTextY =
		ResultPanelY +
		66.0f *
		ScreenScale;


	const float ResultTextScale =
		1.7f *
		ScreenScale;


	const float RetryTextScale =
		0.62f *
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
	//
	// CLEAR / FAILED 공통
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