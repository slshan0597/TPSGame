// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSCharacterBase.h"
#include "TPSHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

ATPSCharacterBase::ATPSCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	HealthComponent = CreateDefaultSubobject<UTPSHealthComponent>(TEXT("HealthComponent"));
}

void ATPSCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 체력 컴포넌트의 사망 알림('OnDeath')에 이 캐릭터의 'HandleDeath' 함수를 연결, 체력이 0이 되면 자동으로 연결된 함수가 호출됨
	// 조건 없이 서버와 모든 클라이언트에 연결, 래그돌은 모든 화면에서 보여야 하기 때문
	HealthComponent->OnDeath.AddDynamic(this, &ATPSCharacterBase::HandleDeath);
}

/**
 * 사망 여부 확인
 */
bool ATPSCharacterBase::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

/**
 * 사망 처리
 *  - 사망 알림이 오면 호출되는 함수
 *  - 매개 변수 형식은 'FTPSOnDeath' 델리게이트 형식에 맞춤 (나중에 사용)
 */
void ATPSCharacterBase::HandleDeath(AActor* Victim, AController* KillerController)
{
	// 래그돌 연출	: 서버와 모든 클라이언트에서 실행
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));		// 캐릭터 메시의 충돌 설정을 엔진 기본 프로필인 'Ragdoll'로 변경
	GetMesh()->SetSimulatePhysics(true);						// 메시에 물리 시뮬레이션을 활성화

	// 게임 로직	: 서버에서만 실행
	if (HasAuthority())
	{
		// 이 캐릭터가 'CorpseLifeSpan'초(기본 60초) 뒤에 자동으로 삭제되도록 예약
		SetLifeSpan(CorpseLifeSpan);
	}
}
