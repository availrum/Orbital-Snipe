#include "OrbitalGameMode.h"

#include "OrbitalPlayerController.h"
#include "OrbitalHUD.h"


AOrbitalGameMode::AOrbitalGameMode()
{
	// 직접 만든 PlayerController 사용
	PlayerControllerClass =
		AOrbitalPlayerController::StaticClass();


	// 실제 게임 HUD 사용
	HUDClass =
		AOrbitalHUD::StaticClass();
}