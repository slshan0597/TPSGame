// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPSHealthComponent.generated.h"

/**
 * 델리게이트 타입 선언
 *  - 이벤트 발생 시 등록된 함수들을 호출해 주는 장치
 *  - 'DYNAMIC'	: 블루프린트에도 연결할 수 있음, 연결할 함수는 반드시 'UFUNCTION'이어야 함
 *  - 'MULTICAST'	: 여러 곳에서 동시 등록 가능, 등록된 함수가 모두 호출됨
 *  - 'TwoParams'	: 알림과 함께 값 2개를 전달
 * @param  
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTPSOnDamaged, float, Damage, AController*, InstigatorController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTPSOnDeath, AActor*, Victim, AController*, KillerController);

/**
 * 체력/피격/사망 처리 (복제 지원), 데미지 계산은 서버에서만 실행
 *  - 'ClassGroup = (Custom)'					: 블루프린트에서 컴포넌트를 추가할 때 'Custom' 분류 아래에 표시
 *  - 'meta = (BlueprintSpawnableComponent)'	: 블루프린트의 'Add' 버튼으로 이 컴포넌트에 직접 추가할 수 있게 함
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TPSGAME_API UTPSHealthComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UTPSHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버 전용 - 피격 시 (AI의 반격 판단 등에 사용)
	//  - 'BlueprintAssignable'	: 블루프린트에서도 이 알림에 이벤트를 연결할 수 있게 함
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FTPSOnDamaged OnDamaged;

	// 서버 + 모든 클라이언트에서 방송, 클라이언트에서 'KillerController'는 'nullptr'
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FTPSOnDeath OnDeath;

	// 서버 전용
	void Heal(float Amount);

	// 'UFUNCTION(BlueprintPure)'	: 블루프린트에서도 쓸 수 있고, 값을 바꾸지 않는 함수이기에 실행 핀 없이 값만 꺼내는 노드로 표시됨
	UFUNCTION(BlueprintPure, Category = "Health") float GetHealth()    const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Health") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "Health") bool  IsDead()	   const { return bIsDead; }

protected:
	virtual void BeginPlay() override;

	// 'EditDefaultsOnly'			: 블루프린트의 기본값에서만 수정 가능, 레벨에 배치된 개별 캐릭터마다 다르게 바꿀 수는 ㅇ벗음
	// 'meta = (ClampMin = "1")'	: 에디터에서 1보다 작은 값을 입력하지 못하게 막음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

private:
	// 'Replicated'						: 서버에서 값이 바뀌면 모든 클라이언트로 자동 전송
	// 'ReplicatedUsing = OnRep_IsDead'	: 이 값이 복제되어 클라이언트에 도착하면 'OnRep_IsDead' 함수를 호출
	UPROPERTY(Replicated)					  float Health  = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_IsDead) bool  bIsDead = false;

	// 언리얼 이름으로 찾아서 호출하기 때문에 'UFUNCTION()'를 붙여야 함
	UFUNCTION()	void OnRep_IsDead();
	UFUNCTION() void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
};
