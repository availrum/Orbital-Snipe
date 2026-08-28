// OrbitalGameMode.cpp
#include "OrbitalGameMode.h"
#include "OrbitalPlayerController.h"

AOrbitalGameMode::AOrbitalGameMode()
{
	// 기본 컨트롤러를 우리가 만든 걸로 교체
	PlayerControllerClass = AOrbitalPlayerController::StaticClass();
}