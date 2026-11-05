// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSHealthComponent.h"
//#include "GameFramework/Actor.h"
//#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

UTPSHealthComponent::UTPSHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 이 컴포넌트를 복제 대상으로 설정
	//  - 컴포넌트 자체를 네트워크로 전송
	SetIsReplicatedByDefault(true);
}

/**
 * 복제 변수 등록
 */
void UTPSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 'DOREPLIFETIME'	: 복제 대상으로 등록
	DOREPLIFETIME(UTPSHealthComponent, Health);
	DOREPLIFETIME(UTPSHealthComponent, bIsDead);
}

void UTPSHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	// 'HasAuthority()'	: 이 액터에 대한 결정권이 있는지(= 즉 서버인지) 확인, 싱글 플레이 시에는 내 컴퓨터가 서버라서 'true' 반환
	if (Owner && Owner->HasAuthority())
	{
		Health = MaxHealth;

		// 'ApplyDamage', 'ApplyPointDamage'로 들어오는 모든 데미지를 여기서 처리
		//  - 'AddDynamic(this, &함수)'	: 이 객체(this)의 이 함수를 알림 목록에 추가
		//  - 데미지 함수로 이 캐릭터에게 데미지를 주면 엔진이 'OnTakeAnyDamage'를 방송하고, 연결된 'HandleTakeAnyDamage'가 자동으로 호출
		Owner->OnTakeAnyDamage.AddDynamic(this, &UTPSHealthComponent::HandleTakeAnyDamage);
	}
}

/**
 * 체력 회복
 */
void UTPSHealthComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.f || !GetOwner() || !GetOwner()->HasAuthority()) return;

	Health = FMath::Min(Health + Amount, MaxHealth);
}

/**
 * 데미지 처리
 *  - 데미지를 받을 때마다 서버에서 자동으로 호출되는 함수
 *  - 'Damage'와 'InstigatedBy'를 제외한 나머지 매개변수는 엔진이 정한 형식을 맞추기 위해 받음
 */
void UTPSHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || bIsDead) return;

	Health = FMath::Max(Health - Damage, 0.f);

	// 피격 알림을 방송
	OnDamaged.Broadcast(Damage, InstigatedBy);

	if (Health <= 0.f)
	{
		bIsDead = true;		// 클라이언트로 복제 -> 'OnRep_IsDead'

		// 사망 알림을 방송
		OnDeath.Broadcast(GetOwner(), InstigatedBy);
	}
}

/**
 * 클라이언트의 사망 처리
 *  - 클라이언트에서 'bIsDead'가 복제되어 도착했을 때 실행
 */
void UTPSHealthComponent::OnRep_IsDead()
{
	// 다른 플레이어(죽인 쪽)의 컨트롤러는 클라이언트에 존재하지 않아서 일 수 없기 때문에 비워둠
	if (bIsDead) OnDeath.Broadcast(GetOwner(), nullptr);
}