// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TPSHUD.generated.h"

class UFont;
class ATPSPlayerCharacter;

/**
 * 'Canvas'로 그리는 간단한 HUD ('UMG'로 교체하기 전짜기 사용)
 */
UCLASS()
class TPSGAME_API ATPSHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override;

private:
	void DrawCrosshair(float CenterX, float CenterY, bool bAiming);
	void DrawPlayerStatus(ATPSPlayerCharacter* Player, UFont* Font);
	void DrawCenteredText(const FString& Text, const FLinearColor& Color, float Y, UFont* Font, float Scale);
};
