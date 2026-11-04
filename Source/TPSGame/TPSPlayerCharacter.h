// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPSCharacterBase.h"
#include "TPSPlayerCharacter.generated.h"

class  USpringArmComponent;
class  UCameraComponent;
class  UInputMappingContext;
class  UInputAction;
struct FInputActionValue;

/**
 * 3인칭 슬더뷰 플레이어 캐릭터
 */
UCLASS()
class TPSGAME_API ATPSPlayerCharacter : public ATPSCharacterBase
{
	GENERATED_BODY()

public:
	ATPSPlayerCharacter();

	virtual void Tick(float DeltaSeconds)												 override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)		 override;
	virtual void PawnClientRestart()													 override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Aim")
	bool IsAiming() const { return bIsAiming; }

protected:
	// 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Camera") float DefaultArmLength = 300.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float AimArmLength		= 150.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float DefaultFOV		= 90.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float AimFOV			= 65.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera") float AimInterpSpeed	= 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement") float DefaultWalkSpeed = 450.f;
	UPROPERTY(EditDefaultsOnly, Category = "Movement") float AimWalkSpeed	  = 220.f;

	// 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction>		 MoveAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction>		 LookAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction>		 JumpAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction>		 AimAction;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnAimPressed();
	void OnAimReleased();

	/**
	 * 조준 동기화
	 *  - 조준/사격 상태를 로컬에 적용하고 서버로 전송
	 */
	void SetCombatState(bool bAiming, bool bFiring);

	// 조준 또는 사격 중에 카메라 방향을 바라보는 스트레이프 모드
	void UpdateRotationMode();

	UFUNCTION(Server, Reliable)
	void ServerSetCombatState(bool bAiming, bool bFiring);

	UFUNCTION()
	void OnRep_CombatState();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	bool bIsAiming = false;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	bool bIsFiring = false;
};
