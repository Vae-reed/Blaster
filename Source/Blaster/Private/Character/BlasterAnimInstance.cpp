#include "Character/BlasterAnimInstance.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Public/Weapon/Weapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation() {
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime) {
	Super::NativeUpdateAnimation(DeltaTime);

	if (BlasterCharacter == nullptr) {
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	if (BlasterCharacter == nullptr)return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0;
	//Velocity.Size() = √(X² + Y² + Z²)
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();

	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();

	bIsCrouched = BlasterCharacter->bIsCrouched;

	bIsAiming = BlasterCharacter->IsAiming();

	TurningInPlace = BlasterCharacter->GetTurningInPlace();

	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();

	/*** 横移 ***/
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());

	/*
	*NormalizedDeltaRotator计算 MovementRotation 和 AimRotation 之间的差值，并且强制将结果限制在[-180, 180]度的范围内
	*这里必须借助标准化函数计算差值，不能直接将两者在 Yaw 方向上的值进行相减，防止在-180 和 180 的过渡点时 “YawOffset” 的值会发生突变
	*/
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);

	/*
	*插值函数 RInterpTo 的特性：它专门为旋转量设计，内部已经处理了角度在 180 和 -180 之间的跳转逻辑
	*DeltaRotation默认值为(0,0,0)，RInterpTo参数中的DeltaRotation的相当于是上一帧的值，而DeltaRot是当前帧的值
	*每一帧，RInterpTo 都会让 DeltaRotation 向着 DeltaRot 移动一小步。计算出的新值再次赋值给 DeltaRotation，作为下一帧的“当前值”
	*/
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f); 
	YawOffset = DeltaRotation.Yaw;
	
	/*** 动态侧倾 ***/
	//通过 NormalizedDeltaRotator 比较了当前帧和上一帧的旋转。这实际上是在求角色在这一帧转了多少度
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);

	/*
	*把旋转量除以时间，得到了角速度。DeltaTime：当前帧与上一帧的时间差
	*如果没有除以 DeltaTime，倾斜度只取决于“帧的旋转量”。
	*如果游戏帧率很高，每一帧旋转量很小，角色就会几乎不倾斜。如果游戏掉帧了，每一帧旋转量巨大，角色就会瞬间倾斜得非常厉害。
	*/
	const float Target = Delta.Yaw / DeltaTime;

	/*
	*平滑插值，防止角色动作出现“一帧到位”的机械感，Lean 变量会平滑地追随 Target 速度
	*float FMath::FInterpTo(float Current, float Target, float DeltaTime, float InterpSpeed);
	*InterpSpeed (插值速度)：移动的快慢。数值越大，移动越快；数值越小，移动越平滑、越迟缓。
	*/	
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);

	//将倾斜度限定在合理的视觉范围内
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	/*** 瞄准偏移 ***/
	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	/*** 手部IK ***/
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh()) 
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		/*
		*把 LeftHandSocket 的世界空间坐标，转换成相对于 hand_r（右手骨骼）的本地坐标。
		*武器是附着在右手骨骼上的，武器上的 `LeftHandSocket` 位置天然就是"相对右手的左手应该在哪里"。
		*以右手骨骼为参考系做转换，结果最稳定——右手怎么动，左手目标跟着以相同的相对关系运动，不会因为角色整体移动或旋转而漂移。
		*/
		BlasterCharacter->GetMesh()->TransformToBoneSpace(
			FName("hand_r"), // 参考骨骼：右手
			LeftHandTransform.GetLocation(), // 输入：左手的世界空间位置
			FRotator::ZeroRotator, //输入：零旋转，IK不关注旋转只关注位置
			OutPosition, // 输出：相对右手骨骼空间的位置
			OutRotation); // 输出：相对右手骨骼空间的旋转
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (BlasterCharacter->IsLocallyControlled()) 
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			//FindLookAtRotation(Start, Target) 内部计算的是 Target - Start
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}


		////枪口指向方向和准星指向方向的调试线
		//FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);//获取枪口 Socket 的 Transform
		//FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));//提取枪口朝向的 X 轴方向
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);//画一条枪口指向的调试线
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Orange);//画一条准星方向的调试线
	}
}
