#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityBody.generated.h"

UENUM(BlueprintType)
enum class EGravityBodyType : uint8
{
	Target      UMETA(DisplayName = "Target"),
	Projectile  UMETA(DisplayName = "Projectile")
};

UCLASS()
class ORBITAL_SNIPE_API AGravityBody : public AActor
{
	GENERATED_BODY()

public:
	AGravityBody();

protected:
	virtual void BeginPlay() override; // 이 줄이 하나만 있어야 합니다!

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	EGravityBodyType BodyType = EGravityBodyType::Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bHasBeenHit = false;
	
	// 질량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float Mass = 1000.0f;

	// 초기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	FVector InitialVelocity = FVector::ZeroVector;

	// 껍데기(메쉬)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	// [새로 추가한 변수들]
	// 충돌 판정용 반지름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float Radius = 50.0f;

	// 궤적 그리기용 직전 위치 기억 변수
	FVector PreviousLocation;
};