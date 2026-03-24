// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

private:
	//TSubclassOf<AProjectile> 是一个可以在编辑器里配置、类型安全的"蓝图类选择器"，告诉武器开火时该生成哪种子弹
	//TSubclassOf<AProjectile> 只能存 AProjectile 或其子类，类型安全
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;
};
