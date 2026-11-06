// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSWeapon.h"
#include "TPSInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"

ATPSWeapon::ATPSWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates					  = true;

	// 무기의 위치/이동은 따로 복제하지 않음
	// 캐릭터의 손에 붙어 있으므로, 'Attachment' 복제로 충분함
	SetReplicatingMovement(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RootComponent = Mesh;
}

void ATPSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATPSWeapon, CurrentAmmo, COND_OwnerOnly);	// 탄창 수는 본인에게만 보냄
	DOREPLIFETIME(ATPSWeapon, bReloading);								// 재장전 중은 모두에게 보냄
}

void ATPSWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 발사 간격
	//	- 'FMath::Max(..., 1.f)'	: 실수로 0을 넣었을 때 0으로 나누는 오류 방지
	TimeBetweenShots = 60.f / FMath::Max(RoundPerMinute, 1.f);

	// 서버에서만 탄창을 가득 채움
	if (HasAuthority()) CurrentAmmo = MagazineSize;
}

void ATPSWeapon::SetCurrentAmmo(int32 NewAmmo)
{
	if (HasAuthority()) CurrentAmmo = FMath::Clamp(NewAmmo, 0, MagazineSize);
}

/**
 * 로컬 클라이언트 (쏘는 쪽)
 */
void ATPSWeapon::StartFire()
{
	// 연타로 연사 속도를 넘지 못하도록 마지막 발사 시각 기준으로 첫 발 지연 계산
	//	- 'static_cast'	: C++에서 "의도적으로 타입을 바꾼다"고 명시하는 방법
	const float Now		   = static_cast<float>(GetWorld()->GetTimeSeconds());
	const float FirstDelay = FMath::Max(LastLocalFireTime + TimeBetweenShots - Now, 0.f);

	// 연사 타이머를 예약
	//	- 'FireTimerHandle'					: 나중에 취소할 수 있게 핸들에 저장
	//	- 'this, &ATPSWeapon::LocalFire'	: 이 무기의 'LocalFire' 함수를 실행
	//	- 'TimeBetweenShots'				: 0.1 초마다
	//	- 'true'							: 반복 실행
	//	- 'FirstDelay'						: 첫 실행의 대기 시간
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ATPSWeapon::LocalFire, TimeBetweenShots, true, FirstDelay);
}

void ATPSWeapon::StopFire()
{
	// 마우스를 떼면 연사 타이머를 취소
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void ATPSWeapon::Reload()
{
	StopFire();
	ServerReload();		// 서버에서 호출하면 바로 실행됨
}

void ATPSWeapon::LocalFire()
{
	if (bReloading) return;

	if (CurrentAmmo <= 0)
	{
		StopFire();
		Reload();		// 탄창이 비면 자동 재장전

		return;
	}

	AController* Controller = GetOwnerController();

	if (!Controller) return;

	// 플레이어	: 카메라 위치 + 방향
	// AI		: 눈 위치 + 포커스 방향
	FVector  ViewLocation;
	FRotator ViewRotation;

	// 어디서 어느 방향을 보고 있는지 가져옴 (각 매개변수에 넣어서 반환)
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	// 탄퍼짐 적용
	//	'ViewRotation.Vector()'	: 회전값을 길이 1짜리 방향으로 바꿈
	//	'VRandCone(방향, 각도)'	: 그 방향을 중심으로 한 원뿔 안에서 랜덤한 방향을 고름
	//	'SpreadDegrees * 0.5f'	: 퍼짐 각도의 절반 (원뿔의 반각)
	//	'DegreesToRadians'		: 각도 -> 라디안으로 변환
	const FVector Direction = FMath::VRandCone(ViewRotation.Vector(), FMath::DegreesToRadians(SpreadDegrees * 0.5f));

	// 쏜 시각을 기록
	LastLocalFireTime = static_cast<float>(GetWorld()->GetTimeSeconds());

	// 위치와 방향을 서버로 보냄
	// 명중 판정은 여기서 하지 않고, 서버에 맏김
	ServerFire(ViewLocation, Direction);
}

/**
 * 서버
 */
// 사격 판정
//	- 클라이언트의 요청을 받은 서버가 실행하는 함수, 실제 처리는 'HandleShot'에 맏김
void ATPSWeapon::ServerFire_Implementation(FVector_NetQuantize ViewLocation, FVector_NetQuantizeNormal ViewDirection)
{
	HandleShot(ViewLocation, ViewDirection);
}

// 요청 검증
void ATPSWeapon::HandleShot(const FVector& ViewLocation, const FVector& ViewDIrection)
{
	if (bReloading || CurrentAmmo <= 0) return;

	// 무기를 든 캐릭터와 게임 월드를 가져옴
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (!OwnerPawn) return;

	UWorld* World = GetWorld();

	// 연사 속도 검증 (네트워크 지연을 감안하여 약간의 여유를 둠)
	const float Now = static_cast<float>(World->GetTimeSeconds());

	if (Now - LastServerFireTime < TimeBetweenShots * 0.75f) return;	// 네트워크 지연에 따라 100%가 아닌, 75% 정도만 비교

	// 시점 위치 검증 (캐릭터에서 너무 먼 위치에서 발사했다고 하면 거부)
	if (FVector::DistSquared(ViewLocation, OwnerPawn->GetActorLocation()) > FMath::Square(1000.f)) return;

	// 갱신
	LastServerFireTime = Now;
	--CurrentAmmo;

	// 트레이스 준비
	//	- 라인 트레이스(광선 검사) 설정
	//	- 'SCENE_QUERY_STAT(TPSWeaponTrace)'	: 성능 분석 도구에서 이 트레이스를 이름으로 구분하기 위한 표시
	//	- 'false'								: 복잡한 충돌 모양 대신 단순한 충돌 모양으로 검사, 속도가 빠름
	//	- 'AddIgnoredActor'						: 해당 액터(무기 자신 + 쏜 사람)를 검사에서 제외함, 그렇지 않으면 총알이 자기 몸에 충돌할 수 있음
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TPSWeaponTrace), false);

	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerPawn);

	// 카메라에서 그 지점까지 광선을 쏴서 처음 맞는 물체를 찾음
	//	- 실제 사격이 아닌, 조준점 탐색
	//	- 'LineTraceSingleByChannel'	: 가장 먼저 맞는 하나만 찾음
	//	- 'TPS_TRACE_WEAPON'			: 이전에 만든 'Weapon 트레이스 채널'로 검색
	FVector    AimPoint = ViewLocation + ViewDIrection * Range;		// 카메라에서 조준 방향으로 거리만큼 트레이스 -> 화면 중앙이 가리키는 조준점
	FHitResult ViewHit;

	if (World->LineTraceSingleByChannel(ViewHit, ViewLocation, AimPoint, TPS_TRACE_WEAPON, Params))
	{
		// 맞으면 'FHitResult'에 결과가 채워지고, 그 위치('ImpactPoint')가 조준점이 됨
		AimPoint = ViewHit.ImpactPoint;
	}

	// 총구에서 조준점으로 실제 사격 (총구 앞 엄폐물은 여기서 막힘)
	//	- 'AimPoint - MuzzleLocation'	: 총구에서 조준점을 향하는 방향과 거리
	//	- 'GetSafeNormal()'				: 정규화, 길이가 0이면 오류 대신 0벡터를 반환("Safe")
	//	- 'TraceEnd'					: 그 방향으로 사거리만큼 간 지점
	const FVector MuzzleLocation  = GetMuzzleLocation();
	const FVector BulletDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	const FVector TraceEnd		  = MuzzleLocation + BulletDirection * Range;
	FHitResult	  Hit;
	const bool	  bHit			  = World->LineTraceSingleByChannel(Hit, MuzzleLocation, TraceEnd, TPS_TRACE_WEAPON, Params);

	// 데미지를 입힘
	if (bHit && Hit.GetActor())
	{
		// AI끼리는 서로 피해를 주지 않음 (PvP와 PvE는 허용)
		//	- 맞은 쪽('HitPawn')과 쏜 쪽('OwnerPawn') 둘 다 플레이어가 아닌 경우
		const APawn* HitPawn		 = Cast<APawn>(Hit.GetActor());
		const bool	 bAIFriendlyFire = HitPawn && !HitPawn->IsPlayerControlled() && !OwnerPawn->IsPlayerControlled();

		if (!bAIFriendlyFire)
		{
			// 언리얼의 기본 데미지 함수로 피해를 줌
			//	- 이 함수가 맞은 액터의 'OnTakeAnyDamage'를 방송 -> 체력 컴포넌트가 그걸 받아 체력을 감소
			//	- 'Hit.GetActor()'				: 맞은 액터
			//	- 'BulletDirection'				: 총알이 날아온 방향
			//	- 'OwnerPawn->GetController()'	: 쏜 사람 (킬 수 집계, AI의 반격 판단 등에 씀)
			//	- 'this'						: 데미지를 준 물체 (해당 무기)
			//	- 'UDamageType::StaticClass()'	: 데미지 종류 (기본값)
			UGameplayStatics::ApplyPointDamage(
				Hit.GetActor(),
				Damage,
				BulletDirection,
				Hit,
				OwnerPawn->GetController(),
				this,
				UDamageType::StaticClass()
			);
		}
	}

	// 이펙트 요청
	//	- 모든 화면에 탄도선과 소리를 표시하라고 방송
	MulticastFireEffects(MuzzleLocation, bHit ? FVector(Hit.ImpactPoint) : TraceEnd, bHit);
}

// 재장전
void ATPSWeapon::ServerReload_Implementation()
{
	StartReloadOnServer();
}

void ATPSWeapon::StartReloadOnServer()
{
	// 'HasReserveAmmo()'	: 가방에 예비 탄약 검색
	if (bReloading || CurrentAmmo >= MagazineSize || !HasReserveAmmo()) return;

	bReloading = true;

	// 재장전 시간 뒤에 한 번 'FinishReload'를 실행하도록 예약
	//	- 'false'	: 반복하지 않음
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ATPSWeapon::FinishReload, ReloadTime, false);
}

void ATPSWeapon::FinishReload()
{
	const int32				Needed	  = MagazineSize - CurrentAmmo;
	UTPSInventoryComponent* Inventory = GetOwnerInventory();

	// 인벤토리가 없는 적(AI)는 예비 탄약 무한
	const int32 Taken = Inventory ? Inventory->ConsumeAmmo(AmmoType, Needed) : Needed;

	CurrentAmmo += Taken;
	bReloading  =  false;
}

/**
 * 공통
 */
void ATPSWeapon::MulticastFireEffects_Implementation(FVector_NetQuantize MuzzleLocation, FVector_NetQuantize EndPoint, bool bHit)
{
	// 테스트용
	if (bDrawDebugTrace)
	{
		// 총구에서 끝 지점까지 노란 선을 그림
		//	'false'	: 계속 표시 여부
		//	'0.1f'	: 표시 시간
		//	'0'		: 우선순위
		//	'1.f'	: 선 두께
		DrawDebugLine(GetWorld(), MuzzleLocation, EndPoint, FColor::Yellow, false, 0.1f, 0, 1.f);

		if (bHit)
		{
			// 히트 시 탄착 지점에 빨간 점을 표시
			DrawDebugPoint(GetWorld(), EndPoint, 10.f, FColor::Red, false, 1.f);
		}
	}

	if (FireSound) UGameplayStatics::PlaySoundAtLocation(this, FireSound, MuzzleLocation);
}

bool ATPSWeapon::HasReserveAmmo() const
{
	const UTPSInventoryComponent* Inventory = GetOwnerInventory();

	return !Inventory || Inventory->GetAmmo(AmmoType) > 0;		// 가방이 없는 경우: 적 AI -> 무한 탄약
}

UTPSInventoryComponent* ATPSWeapon::GetOwnerInventory() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UTPSInventoryComponent>() : nullptr;
}

AController* ATPSWeapon::GetOwnerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());

	return OwnerPawn ? OwnerPawn->GetController() : nullptr;
}

FVector ATPSWeapon::GetMuzzleLocation() const
{
	// 메시에 'Muzzle' 소켓이 있으면 그 위치를, 없으면 무기 액터의 위치(손 위치)를 반환
	return Mesh->DoesSocketExist(MuzzleSocketName) ? Mesh->GetSocketLocation(MuzzleSocketName) : GetActorLocation();
}