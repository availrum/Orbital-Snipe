#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OrbitalHUD.generated.h"

class AGravityManager;
class AOrbitalCannon;

UCLASS()
class ORBITAL_SNIPE_API AOrbitalHUD : public AHUD
{
	GENERATED_BODY()

public:
	AOrbitalHUD();

protected:
	virtual void BeginPlay() override;

public:
	virtual void DrawHUD() override;

private:
	// 게임 상태를 읽기 위한 Actor 참조
	AGravityManager* GravityManager = nullptr;
	AOrbitalCannon* OrbitalCannon = nullptr;

	// Actor 참조 검색
	void FindGameplayActors();

	// 일반 HUD 텍스트 출력
	void DrawHUDText(
		const FString& Text,
		float X,
		float Y,
		const FLinearColor& Color,
		float Scale = 1.0f
	);

	// 화면 중앙 기준 텍스트 출력
	void DrawCenteredHUDText(
		const FString& Text,
		float CenterX,
		float Y,
		const FLinearColor& Color,
		float Scale = 1.0f
	);

	// 점수에 천 단위 쉼표 추가
	FString FormatScore(int32 Score) const;
};