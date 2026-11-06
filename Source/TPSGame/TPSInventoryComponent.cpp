// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSInventoryComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UTPSInventoryComponent::UTPSInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UTPSInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 다른 플레이어의 가방 내용은 보낼 필요가 없음
	//	- 'COND_OwnerOnly'	: 본인만 전달 받음
	DOREPLIFETIME_CONDITION(UTPSInventoryComponent, Slots,		COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UTPSInventoryComponent, AmmoStacks, COND_OwnerOnly);
}

void UTPSInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// 시작 시 탄약 지급
		for (const FTPSAmmoStack& Stack : StartingAmmo) AddAmmo(Stack.AmmoType, Stack.Count);
	}
}

/**
 * 아이템 획득
 */
bool UTPSInventoryComponent::AddItem(UTPSItemData* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0) return false;

	if (Item->ItemType == ETPSItemType::Ammo)
	{
		AddAmmo(Item->AmmoType, Quantity);

		return true;
	}

	// 무기(Weapon)는 장착으로 처리
	if (Item->ItemType == ETPSItemType::Weapon) return false;

	// 아이템이 들어갈 공간이 충분한지 먼저 계산
	//	- 빈 칸에 넣을 수 있는 개수를 구함
	//	- 'MaxSlots - Slots.Num()'	: 남은 빈 칸 수 -> 여기에 한 칸당 최대 개수를 곱함
	const int32 MaxStack = FMath::Max(1, Item->MaxStack);
	int32		Capacity = FMath::Max(0, MaxSlots - Slots.Num()) * MaxStack;

	// 이미 같은 아이템이 든 칸의 남은 자리를 더함
	//	- ex) 붕대가 3개 든 칸이 있고 최대 5개면, 해당 칸에 2개를 더 넣을 수 있음
	for (const FTPSInventorySlot& Slot : Slots)
		if (Slot.Item == Item)
			Capacity += MaxStack - Slot.Quantity;

	// 넣으려는 개수보다 공간이 적으면 아무것도 넣지 않고 실패
	if (Capacity < Quantity) return false;

	// 기존 칸에 채우고, 남으면 새 칸 사용
	int32 Remaining = Quantity;

	for (FTPSInventorySlot& Slot : Slots)
	{
		if (Remaining <= 0)    break;
		if (Slot.Item != Item) continue;

		// 칸의 남은 자리와 넣어야 할 개수 중 작은 만큼만 넣음
		const int32 Added = FMath::Min(MaxStack - Slot.Quantity, Remaining);

		Slot.Quantity += Added;
		Remaining	  -= Added;
	}

	while (Remaining > 0)
	{
		FTPSInventorySlot NewSlot;

		NewSlot.Item	 =  Item;
		NewSlot.Quantity =  FMath::Min(MaxStack, Remaining);
		Remaining		 -= NewSlot.Quantity;

		Slots.Add(NewSlot);
	}

	return true;
}

/**
 * 아이템 제거
 */
bool UTPSInventoryComponent::RemoveItem(UTPSItemData* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0) return false;

	// 아이템의 총 갯수(모든 칸의 수량의 합)
	int32 Owned = 0;

	for (const FTPSInventorySlot& Slot : Slots)
		if (Slot.Item == Item)
			Owned += Slot.Quantity;

	if (Owned < Quantity) return false;

	// 배열의 뒤부터 거꾸로 돌며 아이템을 뺌
	//	- 앞에서부터 지우면, 뒤에 있던 칸들이 한 칸씩 앞으로 당겨저서 다음 칸을 건너뛰는 실수가 발생하므로 뒤에서부터 뺌
	int32 Remaining = Quantity;

	for (int32 i = Slots.Num() - 1; i >= 0 && Remaining > 0; --i)
	{
		// 같은 아이템 칸이면, 칸에 든 개수와 빼야 할 개수 중 작은 만큼 뺌
		if (Slots[i].Item != Item) continue;

		const int32 Removed = FMath::Min(Slots[i].Quantity, Remaining);

		Slots[i].Quantity -= Removed;
		Remaining		  -= Removed;

		// 칸이 비면 그 칸을 배열에서 지움
		if (Slots[i].Quantity <= 0) Slots.RemoveAt(i);
	}

	return true;
}

/**
 * 탄약 늘리기
 */
void UTPSInventoryComponent::AddAmmo(ETPSAmmoType Type, int32 Count)
{
	if (Type == ETPSAmmoType::None || Count <= 0) return;

	for (FTPSAmmoStack& Stack : AmmoStacks)
	{
		// 같은 종류의 탄약이 이미 존재하면 개수만 늘리고 종료
		if (Stack.AmmoType == Type)
		{
			Stack.Count += Count;

			return;
		}
	}

	// 처음 얻는 종류면 새 항목을 만들어 목록에 추가
	FTPSAmmoStack NewStack;

	NewStack.AmmoType = Type;
	NewStack.Count	  = Count;

	AmmoStacks.Add(NewStack);
}

/**
 * 탄약 꺼내기
 */
int32 UTPSInventoryComponent::ConsumeAmmo(ETPSAmmoType Type, int32 Wanted)
{
	for (FTPSAmmoStack& Stack : AmmoStacks)
	{
		// 탄약을 꺼낸 만큼 줄이고, 꺼낸 개수를 반환
		if (Stack.AmmoType == Type)
		{
			const int32 Taken = FMath::Clamp(Wanted, 0, Stack.Count);

			Stack.Count -= Taken;

			return Taken;
		}
	}

	// 해당 종류의 탄약이 아예 없으면 0을 반환
	return 0;
}

/**
 * 조회
 */
// 탄약 조회
int32 UTPSInventoryComponent::GetAmmo(ETPSAmmoType Type) const
{
	for (const FTPSAmmoStack& Stack : AmmoStacks)
		if (Stack.AmmoType == Type)
			return Stack.Count;

	return 0;
}

// 아이템 조회
//	- 가방 앞쪽부터 돌면서, 원하는 종류의 첫 아이템을 찾아서 반환
UTPSItemData* UTPSInventoryComponent::FindFirstItemOfType(ETPSItemType Type) const
{
	for (const FTPSInventorySlot& Slot : Slots)
		if (Slot.Item && Slot.Item->ItemType == Type)
			return Slot.Item;

	return nullptr;
}


// 모든 칸의 "가치 x 개수"를 모두 합한 값
int32 UTPSInventoryComponent::GetTotalValue() const
{
	int32 Total = 0;

	for (const FTPSInventorySlot& Slot : Slots)
		if (Slot.Item)
			Total += Slot.Item->Value * Slot.Quantity;

	return Total;
}