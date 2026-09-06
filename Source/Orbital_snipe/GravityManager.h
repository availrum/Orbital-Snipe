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

	// ============================================================
	// Gameplay State
	// ============================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	int32 TotalScore = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	int32 RemainingTargets = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bStageCleared = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bStageFailed = false;


	// ============================================================
	// Active Object Lifetime
	//
	// Projectile 또는 맞은 Target이 이 시간 이상 활동하면 제거된다.
	// Stage 타이머가 아니라 개별 물체의 최대 활동시간이다.
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float ActiveBodyLifetime = 8.0f;


	// ============================================================
	// Active Area
	//
	// 첫 발사 위치를 중심으로 이 반경 밖으로 나간
	// Projectile / Hit Target은 즉시 제거한다.
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float ActiveAreaRadius = 10000.0f;


protected:
	virtual void BeginPlay() override;


public:
	virtual void Tick(float DeltaTime) override;


	// ============================================================
	// Physics
	// ============================================================

	// 게임용 중력 상수
	UPROPERTY(EditAnywhere, Category = "Physics")
	float GravitationalConstant = 1000.0f;

	// 시간 배속
	UPROPERTY(EditAnywhere, Category = "Physics")
	float TimeScale = 1.0f;


	// ============================================================
	// Public Gameplay Functions
	// ============================================================

	// 새 Projectile을 GravityManager에 등록
	void AddPlanet(AGravityBody* NewPlanet);

	// Cannon에서 마지막 탄환까지 사용했을 때 호출
	void NotifyShotsExhausted();


private:
	// ============================================================
	// Body Management
	// ============================================================

	// 현재 게임 물리 계산에 참여하는 GravityBody
	TArray<AGravityBody*> AllBodies;

	// 각 활성 물체가 얼마나 오래 활동했는지 기록
	//
	// TWeakObjectPtr을 사용해서 Actor가 파괴되어도
	// 유효하지 않은 객체를 안전하게 구분할 수 있게 한다.
	TMap<TWeakObjectPtr<AGravityBody>, float> ActiveBodyTimes;


	// ============================================================
	// Shot / Stage State
	// ============================================================

	int32 NextShotId = 1;

	bool bShotsExhausted = false;


	// ============================================================
	// Active Area
	// ============================================================

	// 첫 번째 Projectile이 생성된 위치를 기준점으로 사용
	FVector ActiveAreaCenter = FVector::ZeroVector;

	bool bActiveAreaCenterInitialized = false;


	// ============================================================
	// Internal Functions
	// ============================================================

	// 중력 / 이동 / 충돌 계산
	void ApplyGravity(float DeltaTime);

	// Projectile / Hit Target의 활동시간과 범위를 검사하고
	// 조건을 넘은 물체를 안전하게 제거
	void UpdateActiveBodies(float DeltaTime);

	// 아직 연쇄충돌을 일으킬 수 있는 활성 물체가 있는지 검사
	bool HasActiveChainObjects() const;

	// Target이 처음 맞았을 때 활동시간 등록
	void RegisterActivatedTarget(AGravityBody* Target);
};