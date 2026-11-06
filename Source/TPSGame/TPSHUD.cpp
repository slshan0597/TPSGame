// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSHUD.h"
#include "TPSTypes.h"
#include "TPSPlayerCharacter.h"
#include "TPSHealthComponent.h"
#include "TPSInventoryComponent.h"
#include "TPSWeapon.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"

/**
 * 그리기 (매 프레임마다 엔진에 의해 호출됨)
 */
void ATPSHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	UFont*				 Font   = GEngine->GetMediumFont();
	ATPSPlayerCharacter* Player = Cast<ATPSPlayerCharacter>(GetOwningPawn());

	if (!Player || Player->IsDead()) return;

	// 크로스헤어와 상태 정보를 그림
	//	'Canvas->ClipX, ClipY'	: 화면의 가로/세로 크기(픽셀) -> 절반을 곱하면 화면 정중앙 좌표
	//							: 화면 좌표는 왼쪽 위가 (0,0), 아래로 갈 수록 Y가 증가
	DrawCrosshair(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f, Player->IsAiming());
	DrawPlayerStatus(Player, Font);
}

void ATPSHUD::DrawCrosshair(float CenterX, float CenterY, bool bAiming)
{
	// 'Gap'			: 중심에서 선이 시작하는 거리
	// 'Len'			: 선 하나의 길이
	// 'FLinearColor'	: 정규화된 RGBA
	const float		   Gap   = bAiming ? 4.f : 8.f;		// 조준 시 크로스헤어가 좁아짐
	const float		   Len   = 10.f;
	const FLinearColor Color = FLinearColor::White;

	// 가운데를 비워 둔('Gap' 만큼) 십자가 선('Len' 길이)을 그림
	//		|
	//	-		-
	//		|
	DrawLine(CenterX - Gap - Len, CenterY,			   CenterX - Gap,		CenterY,			 Color, 2.f);	// 왼쪽
	DrawLine(CenterX + Gap,		  CenterY,			   CenterX + Gap + Len, CenterY,			 Color, 2.f);	// 오른쪽
	DrawLine(CenterX,			  CenterY - Gap - Len, CenterX,				CenterY - Gap,		 Color, 2.f);	// 위
	DrawLine(CenterX,			  CenterY + Gap,	   CenterX,				CenterY + Gap + Len, Color, 2.f);	// 아래
}

// 글자를 화면 "가로 가운데"에 그리는 도우미 함수
void ATPSHUD::DrawCenteredText(const FString& Text, const FLinearColor& Color, float Y, UFont* Font, float Scale)
{
	float Width = 0.f, Height = 0.f;

	// 해당 폰트와 스케일로 글자를 그렸을 때의 너비와 높이를 미리 잼
	GetTextSize(Text, Width, Height, Font, Scale);

	// 화면 중앙에서 글자 너비의 절반만큼 왼쪽에서 시작 -> 글자의 가운데가 화면 중앙에 옴
	DrawText(Text, Color, Canvas->ClipX * 0.5f - Width * 0.5f, Y, Font, Scale);
}

// 체력과 탄약
void ATPSHUD::DrawPlayerStatus(ATPSPlayerCharacter* Player, UFont* Font)
{
	const float Bottom = Canvas->ClipY - 80.f;

	// 체력 표시
	if (const UTPSHealthComponent* Health = Player->GetHealthComponent())
	{
		// 현재 체력 비율
		const float Ratio = (Health->GetMaxHealth() > 0.f) ? (Health->GetHealth() / Health->GetMaxHealth()) : 0.f;

		DrawText(
			FString::Printf(TEXT("HP  %.0f / %.0f"), Health->GetHealth(), Health->GetMaxHealth()),	// 글자, '%.0f' : 소수점을 제거하여 출력
			FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Green, Ratio),				// 색, 체력 비율에 따른 그라데이션 효과
			40.f,		// X
			Bottom,		// Y
			Font,		// 폰트
			1.5f		// 크기 배율
		);
	}

	// 탄약 표시
	if (const ATPSWeapon* Weapon = Player->GetWeapon())
	{
		const UTPSInventoryComponent* Inventory = Player->GetInventory();
		const int32					  Reserve   = Inventory ? Inventory->GetAmmo(Weapon->GetAmmoType()) : 0;
		const FString				  AmmoText  = Weapon->IsReloading()
			? FString(TEXT("RELOADING..."))
			: FString::Printf(TEXT("%d / %d  %s"), Weapon->GetCurrentAmmo(), Reserve, TPSAmmoTypeName(Weapon->GetAmmoType()));

		// 화면 오른쪽 아래에 노란색으로 그림
		// X를 "화면 너비 - 340"으로 정해서, 화면 크기가 달라도 오른쪽 가장자리에서 일정한 위치에 표시
		DrawText(AmmoText, FLinearColor::Yellow, Canvas->ClipX - 340.f, Bottom, Font, 1.5f);
	}
}