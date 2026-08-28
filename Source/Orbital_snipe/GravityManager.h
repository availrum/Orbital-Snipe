#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityBody.h" // 우리가 만든 행성 클래스를 알아야 함
#include "GravityManager.generated.h"

UCLASS()
class ORBITAL_SNIPE_API AGravityManager : public AActor
{
	GENERATED_BODY()

public:
	AGravityManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// 게임용 중력 상수 (실제 우주는 너무 느리니 게임적 허용값 사용)
	UPROPERTY(EditAnywhere, Category = "Physics")
	float GravitationalConstant = 1000.0f;

	// 시간 배속 (빨리감기 기능용)
	UPROPERTY(EditAnywhere, Category = "Physics")
	float TimeScale = 1.0f;
	void AddPlanet(AGravityBody* NewPlanet);
private:
	// 월드에 있는 모든 행성 리스트
	TArray<AGravityBody*> AllBodies;

	// 중력 계산 함수
	void ApplyGravity(float DeltaTime);
};