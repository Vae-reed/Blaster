// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget))//绑定DisplayText变量与widget蓝图中的文本块
	class UTextBlock* DisplayText;

	void SetDisplayText(FString TextToDisplay);//设置DisplayText的值

	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);//显示玩家的网络角色

protected:
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld);//在切换地图或关卡时，移除抬头显示
};
