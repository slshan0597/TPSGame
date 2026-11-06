// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSCharacterBase.h"
#include "TPSTypes.h"
#include "TPSHealthComponent.h"
#include "TPSWeapon.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

ATPSCharacterBase::ATPSCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	HealthComponent = CreateDefaultSubobject<UTPSHealthComponent>(TEXT("HealthComponent"));

	// 총알 판정은 캡슐로만 받음
	// 메시는 무시해서 한 발이 두번 맞는 상황을 방지
	GetCapsuleComponent()->SetCollisionResponseToChannel(TPS_TRACE_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(TPS_TRACE_WEAPON, ECR_Ignore);
}

void ATPSCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 체력 컴포넌트의 사망 알림('OnDeath')에 이 캐릭터의 'HandleDeath' 함수를 연결, 체력이 0이 되면 자동으로 연결된 함수가 호출됨
	// 조건 없이 서버와 모든 클라이언트에 연결, 래그돌은 모든 화면에서 보여야 하기 때문
	HealthComponent->OnDeath.AddDynamic(this, &ATPSCharacterBase::HandleDeath);

	// 무기는 서버에서만 생성하고, 복제를 통해 클라이언트에 나타남
	if (HasAuthority() && DefaultWeaponClass) SpawnAndEquipWeapon(DefaultWeaponClass);
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
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));						// 캐릭터 메시의 충돌 설정을 엔진 기본 프로필인 'Ragdoll'로 변경
	GetMesh()->SetCollisionResponseToChannel(TPS_TRACE_WEAPON, ECR_Ignore);
	GetMesh()->SetSimulatePhysics(true);										// 메시에 물리 시뮬레이션을 활성화

	// 게임 로직	: 서버에서만 실행
	if (HasAuthority())
	{
		// 이 캐릭터가 'CorpseLifeSpan'초(기본 60초) 뒤에 자동으로 삭제되도록 예약
		SetLifeSpan(CorpseLifeSpan);
	}
}

void ATPSCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSCharacterBase, CurrentWeapon);
}

void ATPSCharacterBase::Destroyed()
{
	if (HasAuthority() && CurrentWeapon) CurrentWeapon->Destroy();

	Super::Destroyed();
}

void ATPSCharacterBase::StartFire()
{
	if (!IsDead() && CurrentWeapon) CurrentWeapon->StartFire();
}

void ATPSCharacterBase::StopFire()
{
	if (CurrentWeapon) CurrentWeapon->StopFire();
}

void ATPSCharacterBase::Reload()
{
	if (!IsDead() && CurrentWeapon) CurrentWeapon->Reload();
}

ATPSWeapon* ATPSCharacterBase::SpawnAndEquipWeapon(TSubclassOf<ATPSWeapon> WeaponClass)
{
	if (!HasAuthority() || !WeaponClass) return nullptr;

	// 생성 설정
	//	- 액터를 생성할 때 함꼐 넘길 설정 묶음
	FActorSpawnParameters Params;

	Params.Owner						  = this;		// 무기의 'Server RPC'가 이 캐릭터(플레이어 컨트롤러)를 통해 전달됨
	Params.Instigator					  = this;		// 무기를 사용한 캐릭터, "이 무기가 일으킨 일은 이 캐릭터의 책임"이라는 표시
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;	// 생성 위치에 다른 물체가 겹처 있어도 "무조건 생성"

	// 무기 생성
	ATPSWeapon* Weapon = GetWorld()->SpawnActor<ATPSWeapon>(WeaponClass, GetActorTransform(), Params);

	if (Weapon)
	{
		// 무기를 캐릭터 메시의 소캣(손)에 붙임
		//	- 'GetMesh()'						: 캐릭터의 스켈레탈 메시
		//	- 'SnapToTargetNotIncludingScale'	: 붙이는 규칙 (위치/회전은 소켓에 맞춤, 무기 자신의 크기는 유지)
		//	- 'WeaponSocketName'				: 붙일 위치 (기본 값은 오른손 뼈 'hand_r')
		Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);

		CurrentWeapon = Weapon;
	}

	return Weapon;
}