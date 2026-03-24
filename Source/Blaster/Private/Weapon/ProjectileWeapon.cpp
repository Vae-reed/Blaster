// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (!HasAuthority()) return;

	//获取持有这把武器的 Pawn（角色）。Instigator 在 UE 里特指"造成这件事的发起者"，用于伤害系统判断是谁开的枪
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	//枪口火焰插槽，同样是子弹生成的位置
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));

	if (MuzzleFlashSocket)
	{
		//获取枪口位置
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		//计算子弹朝向
		//ToTarget = 目标点 - 枪口位置  →  得到从枪口指向目标的方向向量
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		//把方向向量转换成旋转角度，子弹生成时会朝着这个旋转方向飞，确保从枪口飞向准星指向的位置，而不是朝着固定方向
		FRotator TargetRotation = ToTarget.Rotation();
		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;//Instigator 在 UE 里特指"造成这件事的发起者"，用于伤害系统判断是谁开的枪
			UWorld* World = GetWorld();
			if (World)
			{
				//在枪口位置，朝着目标方向，生成一个 ProjectileClass 指定类型的子弹 Actor。
				World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
				);
			}
		}
	}
}
