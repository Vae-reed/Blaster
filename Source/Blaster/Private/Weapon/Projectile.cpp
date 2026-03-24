// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Blaster.h"
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);// 设定自己的碰撞类型为 WorldDynamic，用于动态物体的碰撞，通常是玩家角色、移动的物体等
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);// 设定自己的物理碰撞启用类型为 QueryAndPhysics，可以用于空间查询（射线投射、扫描、覆盖）和模拟（刚体、约束）。
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);// 先暂时关闭所有通道的碰撞检测和碰撞响应，然后再开启需要的通道
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);// 与可视物体发生碰撞的响应为阻挡，在碰撞时会停止移动或反弹。
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);// 与静态物体发生碰撞的响应为阻挡，在碰撞时会停止移动或反弹。
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);// 与自定义碰撞体发生碰撞的响应为阻挡，在碰撞时会停止移动或反弹

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;//保证旋转方向跟随速度方向
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,//要播放的粒子特效资产
			CollisionBox,//特效挂载到的组件
			FName(),//挂载的 Socket 名称，传空名称表示直接挂载到组件根部
			GetActorLocation(),//特效生成时的初始位置，用子弹当前的世界坐标作为起点。
			GetActorRotation(),//特效生成时的初始旋转，用子弹当前的朝向，保证曳光方向和子弹飞行方向一致。
			EAttachLocation::KeepWorldPosition//挂载后保持世界空间位置不变。
		);
	}

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor); 
	if (BlasterCharacter)
	{
		BlasterCharacter->MulticastHit();
	}
		
	//Destroy() 销毁 Actor → 引擎自动调用 Destroyed() 
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

