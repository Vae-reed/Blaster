#include "Character/BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Public/Weapon/Weapon.h"
#include "Blaster/Public/BlasterComponents/CombatComponent.h"
#include "Blaster/Public/Character/BlasterAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Blaster.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera")); 
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); 
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent")); 
	Combat->SetIsReplicated(true);//CombatComponent是一个特殊组件，不需要注册和GetLifetimeReplicatedProps，只需要将它的复制属性设置为true

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	//让角色的胶囊体碰撞忽略摄像机通道
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	//设置角色骨骼网格体的碰撞对象类型设置为自定义的 ECC_SkeletalMesh 通道
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	//让角色的骨骼网格体碰撞忽略摄像机通道
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	//让角色的骨骼网格体响应 Visibility 通道的射线检测
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	GetCharacterMovement()->RotationRate = FRotator(0.f, 850.f, 0.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;// 设置网络更新频率
	MinNetUpdateFrequency = 33.f;// 设置最小网络更新频率
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);// 调用父类函数，确保父类中的复制属性也被注册
	/*
	DOREPLIFETIME 是 Unreal Engine 中用于在 GetLifetimeReplicatedProps 函数内注册需要网络复制的属性的宏。
	它告诉引擎：指定的属性（第二个参数）应该从服务器同步到所有客户端（或按条件同步），
	引擎会自动处理该属性的网络序列化、变化检测和分发，确保多玩家游戏中所有机器上的该属性保持一致。
	DOREPLIFETIME_CONDITION在DOREPLIFETIME的前提上允许为网络复制指定一个条件
	*/

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);	// 仅复制给拥有者客户端：当前重叠的武器只对本地玩家可见，用于拾取提示
	DOREPLIFETIME_CONDITION(ABlasterCharacter, AO_Yaw, COND_SimulatedOnly);	// AO_Yaw 的计算只在权威端或本地控制端进行，复制只在模拟端进行
	DOREPLIFETIME_CONDITION(ABlasterCharacter, InterpAO_Yaw, COND_SimulatedOnly); // InterpAO_Yaw 的计算只在权威端或本地控制端进行，复制只在模拟端进行
	DOREPLIFETIME_CONDITION(ABlasterCharacter, StartingAimRotation, COND_SimulatedOnly); // StartingAimRotation 的计算只在权威端或本地控制端进行，复制只在模拟端进行
	DOREPLIFETIME_CONDITION(ABlasterCharacter, TurningInPlace, COND_SimulatedOnly);	// TurningInPlace 的复制只在模拟端进行
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	//在这里添加映射，确保角色已经完全由控制器拥有，避免在复杂网络环境下GetController() 有时会返回空
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(IMC_Move, 100);
			Subsystem->AddMappingContext(IMC_Weapon, 100);
		}
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if(TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
		

	HideCameraIfCharacterClose();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_MoveForward)
		{
			EnhancedInputComponent->BindAction(IA_MoveForward, ETriggerEvent::Triggered, this, &ABlasterCharacter::MoveForward);
		}

		if (IA_MoveRight)
		{
			EnhancedInputComponent->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this, &ABlasterCharacter::MoveRight);
		}

		if (IA_Turn)
		{
			EnhancedInputComponent->BindAction(IA_Turn, ETriggerEvent::Triggered, this, &ABlasterCharacter::Turn);
		}

		if (IA_LookUp)
		{
			EnhancedInputComponent->BindAction(IA_LookUp, ETriggerEvent::Triggered, this, &ABlasterCharacter::LookUp);
		}

		if (IA_Jump)
		{
			EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Started, this, &ABlasterCharacter::Jump);
		}

		if (IA_Equip)
		{
			EnhancedInputComponent->BindAction(IA_Equip, ETriggerEvent::Started, this, &ABlasterCharacter::EquipButtonPressed);
		}

		if (IA_Crouch) {
			EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Started, this, &ABlasterCharacter::CrouchButtonPressed);
		}

		if (IA_Aim) {
			EnhancedInputComponent->BindAction(IA_Aim, ETriggerEvent::Started, this, &ABlasterCharacter::AimButtonPressed);
			EnhancedInputComponent->BindAction(IA_Aim, ETriggerEvent::Completed, this, &ABlasterCharacter::AimButtonReleased);
		}

		if (IA_Fire) {
			EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Started, this, &ABlasterCharacter::FireButtonPressed);
			EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Completed, this, &ABlasterCharacter::FireButtonReleased);
		}
	}
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat) {
		Combat->Character = this;
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);//先把 Montage 激活，让它处于"运行中"状态
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);//再跳到正确的 Section
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);//先把 Montage 激活，让它处于"运行中"状态
		FName SectionName;
		SectionName = ("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);//再跳到正确的 Section
	}
}

void ABlasterCharacter::MoveForward(const FInputActionValue& Value)
{
	if (Controller != nullptr && Value.Get<float>() != 0.f) {
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);//(Pitch,Yaw,Roll)
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));//从YawRotatoin中得到一个矩阵，并从中获得平行于地面的X轴，指向角色前进方向
		AddMovementInput(Direction, Value.Get<float>());
	}
}

void ABlasterCharacter::MoveRight(const FInputActionValue& Value)
{
	if (Controller != nullptr && Value.Get<float>() != 0.f) {
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);//(Pitch,Yaw,Roll)
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));//从YawRotatoin中得到一个矩阵，并从中获得平行于地面的Y轴，指向角色右手方向
		AddMovementInput(Direction, Value.Get<float>());
	}
}

void ABlasterCharacter::LookUp(const FInputActionValue& Value)
{
	AddControllerPitchInput(Value.Get<float>());
}

void ABlasterCharacter::Turn(const FInputActionValue& Value)
{
	AddControllerYawInput(Value.Get<float>());
}

void ABlasterCharacter::EquipButtonPressed(const FInputActionValue& Value)
{
	if (Combat) {
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else 
		{
			ServerEquipButtonPressed();
		}
	}
}

/*
ServerEquipButtonPressed 的定义是由编译器自动生成的，它会检查当前环境。如果它发现自己在客户端运行，它就会把这个函数调用打包成一个网络数据包，发送给服务器
而_Implementation 才是才是真正的业务逻辑，它只会在服务器接收到数据包后被触发
*/
void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat ) {
		Combat->EquipWeapon(OverlappingWeapon);
	}
}


void ABlasterCharacter::CrouchButtonPressed(const FInputActionValue& Value)
{
	if (bIsCrouched) 
	{
		UnCrouch();
	}
	else 
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed(const FInputActionValue& Value)
{
	if (Combat) {
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased(const FInputActionValue& Value)
{
	if (Combat) {
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::FireButtonPressed(const FInputActionValue& Value)
{
	if (Combat) Combat->FireButtonPressed(true);
}

void ABlasterCharacter::FireButtonReleased(const FInputActionValue& Value)
{
	if (Combat) Combat->FireButtonPressed(false);
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	//Velocity.Size() = √(X² + Y² + Z²)
	return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if(Combat && Combat->EquippedWeapon == nullptr)return;
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (HasAuthority() || IsLocallyControlled())
	{
		if (Speed == 0.f && !bIsInAir) {//此时我们处于静止状态
			bRotateRootBone = true;
			FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
			FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
			AO_Yaw = DeltaAimRotation.Yaw;

			if (TurningInPlace == ETurningInPlace::ETIP_NotTurning) {	// 如果不转身
				InterpAO_Yaw = AO_Yaw;									// InterpAO_Yaw 始终和 AO_Yaw 保持一致
			}

			bUseControllerRotationYaw = true;//听从鼠标指挥转身
			TurnInPlace(DeltaTime); //转身动画在偏转角过大时被触发
		}

		if (Speed > 0.f || bIsInAir) {//此时我们处于行走状态或者跳跃状态
			bRotateRootBone = false;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
			AO_Yaw = 0.f;
			bUseControllerRotationYaw = true;//行走状态或者跳跃状态时，角色的身体朝向必须完全听从鼠标的指挥
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;//行走状态或者跳跃状态时，转身动画不应该被触发
		}
	}
	
	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		//把一个值从一个数值范围线性映射到另一个数值范围，并且在超出边界时夹紧（Clamp）不外插
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0.f) 
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched) {	//如果在蹲伏
		UnCrouch();		//起身
	}
	else {				// 如果不在蹲伏，说明在站立状态
		Super::Jump();	// 跳跃
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f) {
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f) {
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning) {					// 如果开始向左或向右转身
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);		// 由于未开始转身前 InterpAO_Yaw 与 AO_Yaw 相等，因此开始转身时这里相对于从 0.0 到 AO_Yaw 插值
		AO_Yaw = InterpAO_Yaw;													// 将每一帧的插值赋给 AO_Yaw，以便在蓝图中获取 AO_Yaw，它的相反数将作为旋转根骨骼节点的入参
		if (FMath::Abs(AO_Yaw) < 15.f) {										// 当 AO_Yaw 小于一定值，这里设为 15.0
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;					// 停止转身
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // 重置起始瞄准旋转
		}
	}

}

void ABlasterCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;//仅让拥有者对该武器不可见
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}

}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	//无论是OnSphereEndOverlap还是OnSphereOverlap调用这个方法，都先隐藏上一把重叠的武器（也就是还未赋值的OverlappingWeapon）的拾取组件，再对OverlappingWeapon进行新的赋值
	if (OverlappingWeapon) {
		OverlappingWeapon->ShowPickupWidget(false);
	}

	//设置当前重叠的武器
	OverlappingWeapon = Weapon;

	//判断角色是否由本地服务器控制（不是网络服务器），若是，显示拾取组件
	if (IsLocallyControlled()) {
		if (OverlappingWeapon) {
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return(Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return(Combat && Combat->bIsAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if(Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon) {
		OverlappingWeapon->ShowPickupWidget(true);
	}

	//当重叠退出时，OverlappingWeapon被设置为nullptr或下一把重叠的武器，而LastWeapon是OverlappingWeapon改变之前的值，即为应该被隐藏拾取组件的武器
	if (LastWeapon) {
		LastWeapon->ShowPickupWidget(false);
	}
}