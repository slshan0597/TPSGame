// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPSCharacterBase.generated.h"

class UTPSHealthComponent;

/**
 * 플레이어와 적의 공통 부모 클래스
 *  - 체력, 사망
 *  - 무기 생성/장착, 사격 입력 전달
 *  - 사망 시 무기와 루트 드롭
 */
UCLASS(Abstract)	// 레벨에 직접 놓지 않고 상속을 통해서만 사용
class TPSGAME_API ATPSCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ATPSCharacterBase();

	UFUNCTION(BlueprintPure, Category = "Health")
	UTPSHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const;

protected:
	virtual void BeginPlay() override;

	// 체력 컴포넌트의 OnDeath에 연결, 서버와 모든 클라이언트에서 호출됨
	UFUNCTION()
	virtual void HandleDeath(AActor* Victim, AController* KillerController);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTPSHealthComponent> HealthComponent;

	// 시체가 사라지기까지의 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float CorpseLifeSpan = 60.f;
};
