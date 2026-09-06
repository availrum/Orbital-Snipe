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
	// 게임 상태를 읽기 위해 참조
	AGravityManager* GravityManager = nullptr;
	AOrbitalCannon* OrbitalCannon = nullptr;

	// 필요한 Actor가 아직 연결되지 않았을 경우 다시 검색
	void FindGameplayActors();

	// 공통 텍스트 그리기 함수
	void DrawHUDText(
		const FString& Text,
		float X,
		float Y,
		const FLinearColor& Color,
		bool bLarge = false
	);
};