// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"

//设置DisplayText的值
void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText) {
		DisplayText->SetText(FText::FromString(TextToDisplay));//FromString: Fstring转FText
	}
}

//显示玩家的网络角色
void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	ENetRole RemoteRole = InPawn->GetRemoteRole(); FString Role;
	switch (RemoteRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority"); 
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy"); 
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy"); 
		break;
	case ENetRole::ROLE_None:
		Role = FString("None"); 
		break;
	}
	FString RemoteRoleString = FString::Printf(TEXT("Remote Role:%s"), * Role);
	SetDisplayText(RemoteRoleString);
}

//在切换地图或关卡时，移除抬头显示
void UOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	RemoveFromParent();//将当前 Widget 从所属的父控件中立即移除并标记为待垃圾回收，确保了 Widget 不再显示在屏幕上，也切断了与 UI 树的引用，防止后续仍被意外访问或渲染。
	//Super::OnLevelRemovedFromWorld(InLevel, InWorld);//调用父类的 OnLevelRemovedFromWorld 实现，执行引擎层面的默认清理逻辑。
}
