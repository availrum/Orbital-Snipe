#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OrbitalCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class ORBITAL_SNIPE_API AOrbitalCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOrbitalCharacter();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// 이동
	void MoveForward(float Value);
	void MoveRight(float Value);

	// 시점
	void Turn(float Value);
	void LookUp(float Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
};