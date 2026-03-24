// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ShowPickupWidget(bool bShowWidget);//显示拾取组件
	virtual void Fire(const FVector& HitTarget);

protected:

	virtual void BeginPlay() override;

	/*这些参数是UE中的重叠事件签名，你绑定的函数必须严格匹配这个参数列表。可以给函数起任何名字，但参数的类型、顺序和数量必须完全一致。*/
	/*事件系统在触发时会向所有绑定的函数传递这些参数，如果函数签名不匹配，编译器将无法生成正确的调用代码*/
	UFUNCTION()//UFUNCTION() 宏的作用是将 C++ 函数标记为 UObject 反射系统可识别的函数，为了让 OnSphereOverlap 能够被 Unreal 的委托系统正确绑定和调用，这是事件驱动编程在 UE 中的标准做法
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent, //发生重叠的组件
		AActor* OtherActor, //与当前组件重叠的另一个 Actor						  
		UPrimitiveComponent* OtherComp, //另一个 Actor 中具体发生重叠的组件
		int32 OtherBodyIndex, //对方组件的身体索引
		bool bFromSweep, //指示这次重叠是否由 sweep 运动引起
		const FHitResult& SweepResult //sweep检测的详细结果，包含碰撞点、法线、穿透深度等物理数据
	);

	/*这些参数是UE中的退出重叠事件签名*/
	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
		);

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;//武器网格

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;//区域检测球体

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;//武器状态

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;//拾取组件

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }

	//准星材质
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;

	//瞄准缩放FOV
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	/*
	*自动开火
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;
};
