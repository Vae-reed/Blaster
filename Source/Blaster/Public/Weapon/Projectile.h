// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, //发生碰撞的自己的组件，在 Projectile 里就是 CollisionBox
		AActor* OtherActor,//被击中的 Actor，也就是子弹撞到了谁
		UPrimitiveComponent* OtherComp,//被击中的 Actor 的具体组件，比如角色的胶囊体、墙壁的静态网格体。
		FVector NormalImpulse,//碰撞时的法线冲量，表示碰撞力的方向和大小
		const FHitResult& Hit); //完整的碰撞信息结构体，包含所有碰撞细节：
private:
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;//击中粒子特效

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;//击中音效
};
