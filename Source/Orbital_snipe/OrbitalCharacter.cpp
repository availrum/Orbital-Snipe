#include "OrbitalCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

AOrbitalCharacter::AOrbitalCharacter()
{
	// 매 프레임 별도의 Tick 로직이 필요하지 않음
	PrimaryActorTick.bCanEverTick = false;

	// 캐릭터 자체가 Controller의 Pitch/Roll을 따라 기울어지지 않도록 함
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	// 이동 방향을 바라보도록 회전
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// 기본 걷기 속도
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;

	// =========================
	// 3인칭 카메라 Arm
	// =========================
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = 350.0f;
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));

	// 마우스로 카메라 방향 회전
	CameraBoom->bUsePawnControlRotation = true;

	// =========================
	// Camera
	// =========================
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(
		CameraBoom,
		USpringArmComponent::SocketName
	);

	// Camera 자체는 SpringArm 회전을 따라감
	FollowCamera->bUsePawnControlRotation = false;
}

void AOrbitalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 이동
	PlayerInputComponent->BindAxis(
		"MoveForward",
		this,
		&AOrbitalCharacter::MoveForward
	);

	PlayerInputComponent->BindAxis(
		"MoveRight",
		this,
		&AOrbitalCharacter::MoveRight
	);

	// 카메라
	PlayerInputComponent->BindAxis(
		"Turn",
		this,
		&AOrbitalCharacter::Turn
	);

	PlayerInputComponent->BindAxis(
		"LookUp",
		this,
		&AOrbitalCharacter::LookUp
	);
}

void AOrbitalCharacter::MoveForward(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	// 카메라가 바라보는 수평 방향 기준
	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	const FVector ForwardDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	AddMovementInput(ForwardDirection, Value);
}

void AOrbitalCharacter::MoveRight(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	const FVector RightDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(RightDirection, Value);
}

void AOrbitalCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void AOrbitalCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}