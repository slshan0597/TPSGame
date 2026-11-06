// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPSTypes.h"
#include "TPSWeapon.generated.h"

class UStaticMeshComponent;
class USoundBase;
class UTPSItemData;
class UTPSInventoryComponent;

/**
 * 히트스캔 무기 (서버 권한 방식)
 *	- 연사 타이밍과 조준 방향은 발사하는 쪽(로컬 플레이어 / 서버 AI)에서 계산
 *	- 명중 판정, 데미지, 탄약 차감은 서버에서만 처리
 *	- 총구 이펙트는 Multicast로 모든 화면에 표시
 */
UCLASS()
class TPSGAME_API ATPSWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	ATPSWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void StartFire();
	void StopFire();
	void Reload();

	// 바닥의 무기를 주웠을 때 장전된 탄 수 설정 (서버 전용)
	void SetCurrentAmmo(int32 NewAmmo);

	UFUNCTION(BlueprintPure, Category = "Weapon") int32			GetCurrentAmmo()  const { return CurrentAmmo; }
	UFUNCTION(BlueprintPure, Category = "Weapon") int32			GetMagazineSize() const { return MagazineSize; }
	UFUNCTION(BlueprintPure, Category = "Weapon") bool			IsReloading()	  const { return bReloading; }
	UFUNCTION(BlueprintPure, Category = "Weapon") ETPSAmmoType  GetAmmoType()	  const { return AmmoType; }
	UFUNCTION(BlueprintPure, Category = "Weapon") UTPSItemData* GetItemData()	  const { return ItemData; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 총구 소켓 이름 (메시에 소켓이 없으면 액터 위치에서 발사)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// 이 무기를 나타내는 아이템 (드롭, 가치 계산용)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UTPSItemData> ItemData;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	ETPSAmmoType AmmoType = ETPSAmmoType::Rifle;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats") float Damage			= 20.f;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats") float Range			= 10000.f;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats") float RoundPerMinute = 600.f;		// 분당 발사 수
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats") float SpreadDegrees	= 1.f;			// 탄퍼짐 각도 (원뿔)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats") int32 MagazineSize	= 30;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats") float ReloadTime		= 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX") TObjectPtr<USoundBase> FireSound;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX") bool				   bDrawDebugTrace = true;

private:
	// 조준 방향을 계산해 서버로 보냄 (로컬 연사 타이머에서 호출)
	void LocalFire();

	// 실제 명중 판정 (서버)
	void HandleShot(const FVector& ViewLocation, const FVector& ViewDirection);
	void StartReloadOnServer();
	void FinishReload();

	bool					HasReserveAmmo()	 const;		// 재정비할 예비 탄약이 있는지 확인
	UTPSInventoryComponent* GetOwnerInventory()  const;
	AController*			GetOwnerController() const;
	FVector					GetMuzzleLocation()  const;

	/**
	 * 네트워크 함수 (RPC - Remote Procedure Call)
	 *	- 클라이언트가 호출하면 서버에서 실행되는 함수
	 *	- 호출은 자신의 컴퓨터(클라이언트)에서 하지만, 실제 실행은 서버나 다른 클라이언트에서 일어남
	 */
	// 'Reliable'	: 네트워크가 불안정해도 반드시 도착을 보장
	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantize ViewLocation, FVector_NetQuantizeNormal ViewDirection);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	// 'NetMulticast'	: 서버에서 호출하면 서버 자신과 접속한 모든 클라이언트에서 실행
	// 'Unreliable'		: 도착을 보장하지 않음, 대신 빠르고 네트워크 부담이 적음
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireEffects(FVector_NetQuantize MuzzleLocation, FVector_NetQuantize EndPoint, bool bHit);

	UPROPERTY(Replicated) int32 CurrentAmmo = 0;
	UPROPERTY(Replicated) bool  bReloading  = false;

	// 타이머 핸들
	//	- 언리얼의 타이머는 "몇 초 뒤에, 또는 몇 초마다 함수를 실행"하라는 예약 기능
	//	- 핸들은 그 예약을 가리키는 "번호표", 후에 이 번호표로 예약을 취소할 수 있음
	FTimerHandle FireTimerHandle;		// 연사 예약
	FTimerHandle ReloadTimerHandle;		// 재장전 완료 예약

	float TimeBetweenShots   = 0.1f;
	float LastLocalFireTime  = -1000.f;
	float LastServerFireTime = -1000.f;
};
