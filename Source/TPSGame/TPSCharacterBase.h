// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPSCharacterBase.generated.h"

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
	// Sets default values for this character's properties
	ATPSCharacterBase();

//protected:
//	// Called when the game starts or when spawned
//	virtual void BeginPlay() override;
//
//public:	
//	// Called every frame
//	virtual void Tick(float DeltaTime) override;
//
//	// Called to bind functionality to input
//	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
