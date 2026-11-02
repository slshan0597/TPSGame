// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPSTypes.generated.h"

/**
 * 무기 전용 트레이스 채널
 * Project Settings > Engine > Collision 에서 "첫 번째"로 추가한 Trace Channel(이름: Weapon)이 ECC_GameTraceChannel1 에 해당함
 */

#define TPS_TRACE_WEAPON ECC_GameTraceChannel1

UENUM(BlueprintType)
enum class ETPSItemType : uint8
{
	Weapon,
	Ammo,
	Medical,
	Valuable
};

UENUM(BlueprintType)
enum class ETPSAmmoType : uint8
{
	None,
	Rifle,
	Pistol,
	Shotgun
};

// 플레이어별 레이드 결과
UENUM(BlueprintType)
enum class ETPSRaidStatus : uint8
{
	InRaid,
	Extracted,
	KilledInAction,
	MissingInAction
};

USTRUCT(BlueprintType)
struct FTPSAmmoStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	ETPSAmmoType AmmoType = ETPSAmmoType::Rifle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	int32 Count = 0;
};

inline const TCHAR* TPSAmmoTypeName(ETPSAmmoType Type)
{
	switch (Type)
	{
	case ETPSAmmoType::Rifle:	return TEXT("Rifle");
	case ETPSAmmoType::Pistol:	return TEXT("Pistol");
	case ETPSAmmoType::Shotgun: return TEXT("Shotgun");
	default:					return TEXT("-");
	}
}