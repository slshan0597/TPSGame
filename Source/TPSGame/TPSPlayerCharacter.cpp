// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ATPSPlayerCharacter::ATPSPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);		// 캐릭터 충돌 캡슐의 크기 (반지름: 42cm, 절반 높이: 96cm (전체 약 1.9m))

	// 컨트롤러 회전은 카메라에만 적용
	bUseControllerRotationPitch = false;	// 위/아래 기울기
	bUseControllerRotationYaw   = false;	// 좌/우 회전
	bUseControllerRotationRoll  = false;	// 옆으로 기울기

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	MoveComp->bOrientRotationToMovement = true;							// 캐릭터가 움직이는 방향을 바라보도록 자동으로 회전
	MoveComp->RotationRate				= FRotator(0.f, 600.f, 0.f);	// 회전 속도
	MoveComp->MaxWalkSpeed				= DefaultWalkSpeed;
	MoveComp->JumpZVelocity				= 600.f;

	// CreateDefaultSubobject	: 생성자에서 컴포넌트를 만들 때 쓰는 언리얼 전용 함수
	// TEXT("CameraBoom")		: 블루프린트의 Component 패널에 표시될 이름
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);						// 스프링암을 캐릭터의 루트 컴포넌트(캡슐)에 부착
	CameraBoom->TargetArmLength			= DefaultArmLength;
	CameraBoom->bUsePawnControlRotation = true;						// 마우스 방향(컨트롤러 회전)을 따라 회전
	CameraBoom->SocketOffset			= FVector(0.f, 60.f, 60.f);	// 스프링암 끝(카메라 위치, 오른쪽 어깨 너머 시점)
	CameraBoom->bEnableCameraLag		= true;						// 카메라 Follow 보간(캐릭터를 부드럽게 따라감)
	CameraBoom->CameraLagSpeed			= 20.f;						// 보간 속도

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));

	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);		// 카메라를 스프링 암의 끝부분(소켓)에 부착
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(DefaultFOV);
}

/**
 * 멀티 플레이 복제 설정
 *  - "이 클래스의 어떤 변수를 다른 플레이어에게 전송할 지" 엔진에 알려주는 함수
 *  - 헤더에서 'Replicated'를 붙인 변수는 여기에 꼭 등록해야 함
 */
void ATPSPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 본인은 이미 로컬에서 적용했으므로 다른 사람에게만 전송
	// 'COND_SkipOwner' : "본인에게는 보내지 않는다"는 조건
	DOREPLIFETIME_CONDITION(ATPSPlayerCharacter, bIsAiming, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ATPSPlayerCharacter, bIsFiring, COND_SkipOwner);
}

/**
 * 입력 매핑 등록
 *  - 플레이어가 이 캐릭터를 "조종하기 시작할 때(빙의할 때)" 호출되는 함수
 */
void ATPSPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// 로컬 플레이어 입력 매핑 등록
	// 이 캐릭터를 조종하는 컨트롤러를 가져와, 플레이어 컨트롤러가 맞는지 확인
	// 'Cast<>'	: 불러온 객체의 타입이 맞으면 해당 타입으로 형변환하여 반환, 아니면 'nullptr' 반환
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// 내 컴퓨터의 플레이어('LocalPlayer')가 가진 '입력 관리자'를 호출
		if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			SubSystem->ClearAllMappings();

			// 블루프린트에서 지정한 'IMC_Default (어떤 키가 어떤 액션인지 정해둔 표)'를 등록
			if (DefaultMappingContext) SubSystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

/**
 * 키와 함수 연결
 *  - 엔진이 캐릭터를 조종하기 시작할 때 자동으로 호출
 */
void ATPSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EIC) return;

	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPSPlayerCharacter::Move);
	EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPSPlayerCharacter::Look);
	EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &ACharacter::Jump);
	EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EIC->BindAction(AimAction,  ETriggerEvent::Started,   this, &ATPSPlayerCharacter::OnAimPressed);
	EIC->BindAction(AimAction,  ETriggerEvent::Completed, this, &ATPSPlayerCharacter::OnAimReleased);
}

void ATPSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocallyControlled()) return;

	// 조준 시 카메라 줌 보간 (본인 화면에서만)
	const float TargetArm = bIsAiming ? AimArmLength : DefaultArmLength;
	const float TargetFOV = bIsAiming ? AimFOV		 : DefaultFOV;

	// 현재 스프링암 길이 보간
	// 'FMath::FInterpTo(현재값, 목표값, 지난 시간, 속도)	: 보간 함수
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm, DeltaSeconds, AimInterpSpeed);

	// 시야각 보간
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaSeconds, AimInterpSpeed));
}

/**
 * 이동
 */
void ATPSPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();	// 마우스의 이동량

	if (!Controller) return;

	// 컨트롤러(카메라가 보는 방향)의 회전에서 "좌우 방향(Yaw)만" 추출
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	// 'FRotationMatrix(...).GetUnitAxis(Exis::X)'	: 이 회전에서 앞쪽을 가리키는 길이 1짜리 방향을 구하는 식
	// 'AddMovementInput'							: 실제로 위치를 바꾸지 않고 요청만 쌓아둠, 실제 이동은 'CharacterMovement'에서 충돌 계산 후 처리
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);	// 카메라 기준 "앞 방향(X축)"으로 'Axis.Y'만큼 이동 입력
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);	// 카메라 기준 "오른쪽 방향(Y축)"으로 'Axis.X'만큼 이동 입력
}

/**
 * 시점 회전
 */
void ATPSPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();

	AddControllerYawInput(Axis.X);		// "좌우(X)"는 컨트롤러의 "Yaw"에 저장
	AddControllerPitchInput(Axis.Y);	// "상하(Y)"는 컨트롤러의 "Pitch"에 저장
}

/**
 * 조준 입력 처리
 *  - 사격 상태('bIsFiring')는 현재 상태를 그대로 전달
 */
void ATPSPlayerCharacter::OnAimPressed()
{
	SetCombatState(true, bIsFiring);
}

void ATPSPlayerCharacter::OnAimReleased()
{
	SetCombatState(false, bIsFiring);
}

void ATPSPlayerCharacter::SetCombatState(bool bAiming, bool bFiring)
{
	bIsAiming = bAiming;
	bIsFiring = bFiring;

	UpdateRotationMode();

	// 이동 속도/회전 방식이 서버와 다르면 위치 보정으로 끊김이 생기므로 서버에도 적용
	// 'HasAuthority()'	: "이 컴퓨터가 서버인지" 묻는 함수
	// 클라이언트일 경우 서버에도 같은 상태로 전달
	if (!HasAuthority()) ServerSetCombatState(bAiming, bFiring);
}

// 클라이언트가 보낸 요청을 서버에서 실행하는 함수
// 구현에는 이름 뒤에 '_Implementation'을 붙이느 게 언리얼의 규칙
// 서버에서 해당 변수들이 바뀌면, 앞에서 등록한 복제 설정에 따라 다른 플레이어에게 자동으로 전송
void ATPSPlayerCharacter::ServerSetCombatState_Implementation(bool bAiming, bool bFiring)
{
	bIsAiming = bAiming;
	bIsFiring = bFiring;

	UpdateRotationMode();
}

// 다른 플레이어의 컴퓨터에서 조준 상태가 복제되어 도착했을 때 자동으로 호출되는 함수
// 헤더의 'ReplicatedUsing = OnRep_CombatState'가 해당 연결을 생성
void ATPSPlayerCharacter::OnRep_CombatState()
{
	UpdateRotationMode();
}

/**
 * 회전 방식과 속도 갱신
 */
void ATPSPlayerCharacter::UpdateRotationMode()
{
	const bool bStrafe = bIsAiming || bIsFiring;

	bUseControllerRotationYaw = bStrafe;

	// 스트레이프 모드일 때는 캐릭터가 "카메라의 방향을 바라보고" A, D를 누르면 옆걸음으로 이동
	// 평소에는 "반대로 이동하는 방향을 바라봄"
	// 두 설정은 항상 반대 값이어야 함
	GetCharacterMovement()->bOrientRotationToMovement = !bStrafe;
	GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : DefaultWalkSpeed;
}