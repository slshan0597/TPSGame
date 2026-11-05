// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPSTypes.h"
#include "TPSItemData.h"
#include "TPSInventoryComponent.generated.h"

/**
 * 플레이어 인벤토리 - 칸(슬롯) 단위 아이템 + 탄약 종류별 수량
 *	- 수정은 서버에서만 이루어짐, 내용은 소유자(본인)에게만 복제됨
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPSGAME_API UTPSInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTPSInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 서버 전용 함수
	 */
	// 수정
	bool AddItem(UTPSItemData* Item, int32 Quantity);
	bool RemoveItem(UTPSItemData* Item, int32 Quantity);
	void AddAmmo(ETPSAmmoType Type, int32 Count);

	// 요청한 만큼 탄약을 꺼냄, 실제로 꺼낸 수를 반환
	int32 ConsumeAmmo(ETPSAmmoType Type, int32 Wanted);

	// 조회
	int32							 GetAmmo(ETPSAmmoType Type)				const;
	UTPSItemData*					 FindFirstItemOfType(ETPSItemType Type) const;
	int32							 GetTotalValue()						const;
	const TArray<FTPSInventorySlot>& GetSlots()								const { return Slots; }
	int32							 GetMaxSlots()							const { return MaxSlots; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxSlots = 12;

	// 레이드 시작 시 지급할 탄약
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<FTPSAmmoStack> StartingAmmo;

private:
	UPROPERTY(Replicated) TArray<FTPSInventorySlot> Slots;
	UPROPERTY(Replicated) TArray<FTPSAmmoStack>		AmmoStacks;
};
