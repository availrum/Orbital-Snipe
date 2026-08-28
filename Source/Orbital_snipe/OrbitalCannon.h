#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "OrbitalCannon.generated.h"

UCLASS()
class ORBITAL_SNIPE_API AOrbitalCannon : public APawn
{
	GENERATED_BODY()

public:
	AOrbitalCannon();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// [부품 조립]
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cannon")
	class UStaticMeshComponent* BaseMesh;   // 회전 받침대 (좌우 회전)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cannon")
	class UStaticMeshComponent* BarrelMesh; // 포신 (상하 회전)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cannon")
	class USceneComponent* ProjectileSpawnPoint; // 발사 위치

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cannon")
	class UCameraComponent* CannonCamera;   // 조준경 카메라

	// [설정 값]
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cannon")
	TSubclassOf<class AGravityBody> PlanetClass;

	float CurrentYaw = 0.0f;
	float CurrentPitch = 45.0f; // 초기 45도 각도
	float CurrentPower = 2000.0f;

	// [입력 함수]
	void RotateCannon(float Val); // 좌우 (A/D)
	void ElevateCannon(float Val); // 상하 (W/S)
	void AdjustPower(float Val);   // 파워 (Q/E)
	void Fire();

	// [기능]
	void DrawTrajectory(); // 궤적 그리기 (항상 실행)

private:
	class AGravityManager* CachedManager;
};