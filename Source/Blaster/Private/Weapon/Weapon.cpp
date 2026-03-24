// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include"Components/SkeletalMeshComponent.h"
#include "Weapon/Casing.h"
#include "Engine/SkeletalMeshSocket.h"

AWeapon::AWeapon()
// Sets default values
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);//为了武器掉落时的反弹效果，将武器网格本身对所有碰撞通道的碰撞反馈设置为阻挡，物体在碰撞时会停止移动或反弹。
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);//将武器网格本身对玩家通道的碰撞反馈忽略
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);//武器一开始应处于无碰撞状态，可从地上捡起或穿过

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));//用于检测与玩家是否重叠的区域球体
	AreaSphere->SetupAttachment(RootComponent);

	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);//忽略区域球体的碰撞
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);//默认关闭碰撞检测
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	//在游戏开始时设置PickupWidget的可见性为false
	if (PickupWidget) {
		PickupWidget->SetVisibility(false);
	}

	if (HasAuthority())//检测是否在服务器端
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);//仅在服务器端开启AreaSphere的碰撞检测
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);//仅在服务器端动态绑定重叠响应函数到球体组件的多播委托上，当发生重叠时自动执行
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);//仅在服务器端动态绑定结束重叠处理函数到球体组件的多播委托上，当重叠退出时自动执行
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
}
 
void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//Cast 是 Unreal 中类型安全的动态转换，用于将基类指针或引用转换为派生类指针或引用，以便访问派生类特有的成员。如果转换失败（对象类型不匹配），则返回 nullptr，因此需要先检查再使用。
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	//通过BlasterCharacter类中的SetOverlappingWeapon来设置OverlappingWeapon
	if (BlasterCharacter) {
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
	//Cast 是 Unreal 中类型安全的动态转换，用于将基类指针或引用转换为派生类指针或引用，以便访问派生类特有的成员。如果转换失败（对象类型不匹配），则返回 nullptr，因此需要先检查再使用。
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	//设置OverlappingWeapon为空指针
	if (BlasterCharacter) {
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		break;
	}

}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if(PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);//根据bShowWidget决定PickupWidget的可见性
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);//播放FireAnimation动画，不循环
	}

	if (CasingClass)
	{
		//生成弹壳
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}
} 