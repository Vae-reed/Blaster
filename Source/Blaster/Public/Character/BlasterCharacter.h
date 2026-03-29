#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "BlasterTypes\TurningInPlace.h"
#include "Blaster/Public/Interfaces/InteractWithCrosshairs.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter,public IInteractWithCrosshairs
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;// 注册网络复制属性：指定哪些成员变量需要从服务器同步到客户端
	virtual void PostInitializeComponents() override; //在该 Actor 挂载的所有组件（Components）都已经完成初始化之后，执行自定义的后续设置逻辑
	void PlayFireMontage(bool bAiming);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHit();

	virtual void OnRep_ReplicatedMovement() override;
protected:
	virtual void BeginPlay() override;

	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void LookUp(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void EquipButtonPressed(const FInputActionValue& Value);
	void CrouchButtonPressed(const FInputActionValue& Value);
	void AimButtonPressed(const FInputActionValue& Value);
	void AimButtonReleased(const FInputActionValue& Value);
	void FireButtonPressed(const FInputActionValue& Value);
	void FireButtonReleased(const FInputActionValue& Value);
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();
	virtual void Jump() override;
	void PlayHitReactMontage();

	//TObjectPtr: 增加在SHIPPING发布之前的安全性
	//想处理什么Action，就响应什么动作，不太关心动作是由什么按键触发的

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> IMC_Move;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> IMC_Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_MoveForward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_MoveRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_LookUp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Turn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Equip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Crouch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Aim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnhancedInput|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Fire;
private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true")/*允许私有变量被蓝图访问*/)
	class UWidgetComponent* OverheadWidget;
	/*
	当属性被标记为 Replicated 时，引擎会在服务器上检测其值的变化，并将最新值发送给所有连接的客户端，确保所有机器上的该属性保持一致。
	属性修改为ReplicatedUsing = OnRep_OverlappingWeapon后，该属性仍然会从服务器自动复制到客户端，但在每次客户端接收到属性更新后，引擎会额外自动调用指定的 OnRep_OverlappingWeapon 函数。
	*/
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;//正在重叠的武器

	/*
	当 OverlappingWeapon 属性从服务器更新到客户端时，引擎会自动调用该函数。它允许客户端在属性值变化后执行本地逻辑，而无需等待服务器额外通知。
	UE 允许 RepNotify 函数拥有一个与属性类型相同的参数——即将要被复制的属性OverlappingWeapon，其类型为AWeapon*。
	当函数被调用时，引擎会将属性更新前的旧值作为实参传递给这个参数。这个旧值是在复制数据到达客户端、但尚未写入成员变量之前保存下来的。
	*/
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;//战斗组件

	/*
	宏定义Server主要用于实现远程过程调用，作用是告诉引擎这个函数由客户端发起调用，但实际逻辑在服务器上执行
	Reliable RPC：可靠的RPC，确保在客户端和服务器之间可靠地传输数据。如果由于网络问题导致数据丢失，会尝试重新发送。
	*/
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UPROPERTY(Replicated)
	float AO_Yaw;

	UPROPERTY(Replicated)
	float InterpAO_Yaw;

	float AO_Pitch;

	UPROPERTY(Replicated)
	FRotator StartingAimRotation;//角色开始瞄准时的旋转角度

	UPROPERTY(Replicated)
	ETurningInPlace TurningInPlace;

	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat) 
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;//摄像机阈值，是摄像机与角色的安全距离

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

	/*
	角色血量
	*/

	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere,Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	class ABlasterPlayerController* BlasterPlayerController;
public:
	void SetOverlappingWeapon(AWeapon* Weapon);//设置正在重叠的武器
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw()const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch()const { return AO_Pitch; }

	AWeapon* GetEquippedWeapon();
	
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }

	FVector GetHitTarget() const;

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
};
