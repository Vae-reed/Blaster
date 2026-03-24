// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	//让弹壳不与摄像机产生碰撞
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectionImpulse = 5.f;//Impulse冲量，表示瞬间施加的力。
}


void ACasing::BeginPlay()
{
	Super::BeginPlay();
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	// 解绑碰撞事件，防止落地后反复弹跳触发多次 OnHit
	CasingMesh->OnComponentHit.RemoveAll(this);

	FTimerHandle DestroyTimer;
	// 1.5 秒后销毁
	GetWorldTimerManager().SetTimer(
		DestroyTimer, //计时器的句柄，相当于这个计时器的身份证。
		this,//回调函数的执行对象
		&ACasing::DestroyCasing,//计时器到期后调用的函数，必须满足特定的签名格式——无参数、无返回值
		1.5f,//延迟时间
		false//是否循环执行
	);
}

void ACasing::DestroyCasing()
{
	Destroy();
}