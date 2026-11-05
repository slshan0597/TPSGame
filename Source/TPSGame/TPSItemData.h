// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TPSTypes.h"
#include "TPSItemData.generated.h"

// 전방 선언
//  - 이 헤더에서는 두 클래스를 포인터나 클래스 참조로만 쓰기 때문에, "이런 이름의 클래스가 있다"고만 알려 둬도 충분함
//	- 전체 헤더를 include 하지 않기에 빌드가 빠름, 'TPSWeapon.h'가 다시 이 파일을 include 하는 순환 문제도 피할 수 있음
class ATPSWeapon;
class UStaticMesh;

/**
 * 아이템 정의(데이터 에셋)
 *  - 에디터에서 'Data Asset'으로 만들어 아이템 종류마다 하나씩 둠
 *  - ex) 'DA_Rifle', 'DA_Bandage', 'DA_GoldBar'
 */
UCLASS(BlueprintType)		// 'BlueprintType'	: 블루프린트에서 이 타입의 변수를 만들 수 있게 함
class TPSGAME_API UTPSItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// 'FText'		: 사용자에게 보여주는 글자 (다국어 번역 지원)
	// 'FString'	: 코드 안에서 다루는 일반 문자열
	// 'FName'		: 소켓 이름, 본 이름처럼 비교가 빨라야 하는 식별용 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	ETPSItemType ItemType = ETPSItemType::Valuable;

	// 바닥에 놓였을 때 보일 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMesh> PickupMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.01"))
	float PickupScale = 1.f;

	// 탈출 성공 시 정산되는 가치(개당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0"))
	int32 Value = 100;

	// 인벤토리 한 칸에 겹칠 수 있는 최대 개수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	// 생성할 무기 클래스
	//  - 'TSubclassOf<ATPSWeapon>'	: 무기 객체가 아닌 무기 "클래스(설계도)"를 가리킴
	//								: 에디터에서는 'ATPSWeapon'을 상속한 클래스만 고를 수 있으므로, 'BP_Rifle'이나 'BP_SMG' 등을 지정함
	//	- 'EditConditionHides'		: 조건이 맞지 않으면 회색으로 잠그는 대신 아에 숨김 (ex_ 의료품 에셋을 열면 무기 칸이 보이지 않음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "ItemType == ETPSItemType::Weapon", EditConditionHides))
	TSubclassOf<ATPSWeapon> WeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (EditCondition = "ItemType == ETPSItemType::Ammo", EditConditionHides))
	ETPSAmmoType AmmoType = ETPSAmmoType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (EditCondition = "ItemType == ETPSItemType::Medical", EditConditionHides))
	float HealAmount = 50.f;
};

/**
 * 루트 테이블의 한 줄
 *	- 어떤 아이템이 어떤 확률(가중치)로 몇 개가 나올지 저장하는 구조체
 */
USTRUCT(BlueprintType)
struct FTPSLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UTPSItemData> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;
};

/**
 * 인벤토리 한 칸
 */
USTRUCT(BlueprintType)
struct FTPSInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UTPSItemData> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = 0;
};

/**
 * 이름 꺼내기 도우미
 *	- 아이템의 표시 이름을 문자열로 꺼냄, HUD의 가방 화면과 줍기 안내 문구에 씀
 */
inline FString TPSItemName(const UTPSItemData* Item)
{
	if (!Item) return FString();	// 빈 문자열

	return Item->DisplayName.IsEmpty() ? Item->GetName() : Item->DisplayName.ToString();
}

namespace TPSLoot
{
	/**
	 * 가중치 기반 랜덤 뽑기
	 *	- 루트 테이블(배열)을 받아서, 가중치에 따라 한 줄을 골라 그 줄의 주소를 반환
	 *	- 테이블이 비었거나 가중치의 합이 0이면 'nullptr'
	 */
	inline const FTPSLootEntry* PickWeighted(const TArray<FTPSLootEntry>& Table)
	{
		float Total = 0.f;

		// 가중치 합계 - 아이템이 지정된 줄의 가중치만 더함
		for (const FTPSLootEntry& Entry : Table)
			if (Entry.Item) Total += Entry.Weight;

		if (Total <= 0.f) return nullptr;

		// 주사위 굴리기
		float Roll = FMath::FRandRange(0.f, Total);

		// 떨어진 구간 탐색
		//	- 뽑은 숫자에서 각 줄의 가중치를 차례로 빼다가, 0 이하가 되는 줄을 선택
		//	- ex) 스포너의 아이템: 붕대(5) / 구급상자(1), 뽑은 숫자: 5.7
		//		-> 붕대(5)를 뺀 결과		: 0.7  -> 다음 줄
		//		-> 구급상자(1)를 뺀 결과	: -0.3 -> 구급상자 선택
		const FTPSLootEntry* LastValid = nullptr;

		for (const FTPSLootEntry& Entry : Table)
		{
			if (!Entry.Item) continue;

			LastValid =  &Entry;
			Roll	  -= Entry.Weight;

			if (Roll <= 0.f) return &Entry;
		}

		return LastValid;	// 소수점 오차에 대한 방지책
	}
}