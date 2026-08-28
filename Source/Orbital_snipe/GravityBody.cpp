// GravityBody.cpp
#include "GravityBody.h"

AGravityBody::AGravityBody()
{
	// 매 프레임 갱신할 필요 없음 (물리 계산은 나중에 컴포넌트가 함)
	PrimaryActorTick.bCanEverTick = false;

	// 1. 메쉬 컴포넌트 생성 및 루트로 설정
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	// 2. 언리얼 기본 물리 엔진 끄기 (우리가 직접 만들 거니까!)
	MeshComponent->SetSimulatePhysics(false);
}

void AGravityBody::BeginPlay()
{
	Super::BeginPlay();// 시작할 때 "이전 위치"를 "현재 위치"로 세팅
	PreviousLocation = GetActorLocation();

	// 메쉬 크기에 맞춰 반지름 자동 설정 (편의 기능)
	if (MeshComponent && MeshComponent->GetStaticMesh())
	{
		FBoxSphereBounds Bounds = MeshComponent->GetStaticMesh()->GetBounds();
		// 스케일까지 고려한 반지름 계산
		Radius = Bounds.SphereRadius * GetActorScale3D().X;
	}
}