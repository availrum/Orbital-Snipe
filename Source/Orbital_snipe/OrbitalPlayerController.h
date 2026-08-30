#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OrbitalPlayerController.generated.h"

class AOrbitalCannon;

UCLASS()
class ORBITAL_SNIPE_API AOrbitalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AOrbitalPlayerController();

	virtual void SetupInputComponent() override;

protected:
	// 상호작용 (대포 타기/내리기)
	void Interact();

	UPROPERTY()
	TObjectPtr<APawn> StoredPlayerPawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float CannonInteractionDistance = 300.0f;

private:
	AOrbitalCannon* FindNearestCannon() const;
};