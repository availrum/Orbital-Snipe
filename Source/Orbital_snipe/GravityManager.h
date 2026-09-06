#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityBody.h"
#include "GravityManager.generated.h"

UCLASS()
class ORBITAL_SNIPE_API AGravityManager : public AActor
{
	GENERATED_BODY()

public:
	AGravityManager();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	int32 TotalScore = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	int32 RemainingTargets = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bStageInitialized = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bStageCleared = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bStageFailed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float ActiveBodyLifetime = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float ActiveAreaRadius = 10000.0f;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Physics")
	float GravitationalConstant = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Physics")
	float TimeScale = 1.0f;

	void AddPlanet(AGravityBody* NewPlanet);

	void NotifyShotsExhausted();

	bool IsStageFinished() const;

private:
	TArray<AGravityBody*> AllBodies;

	TMap<TWeakObjectPtr<AGravityBody>, float> ActiveBodyTimes;

	int32 NextShotId = 1;

	bool bShotsExhausted = false;

	FVector ActiveAreaCenter = FVector::ZeroVector;

	bool bActiveAreaCenterInitialized = false;

	// World Partition 로딩과 BeginPlay 순서 차이를 해결하기 위한
	// 초기 Target 검색 상태
	int32 LastDetectedTargetCount = -1;

	int32 StableTargetScanFrames = 0;

	int32 RequiredStableTargetScanFrames = 3;

	void TryInitializeStage();

	void ApplyGravity(float DeltaTime);

	void UpdateActiveBodies(float DeltaTime);

	bool HasActiveChainObjects() const;

	void RegisterActivatedTarget(AGravityBody* Target);
};